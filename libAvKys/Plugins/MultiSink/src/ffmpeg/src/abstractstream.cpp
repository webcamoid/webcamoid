/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#include <QQueue>
#include <QAbstractEventDispatcher>
#include <QtConcurrent>
#include <QThread>
#include <QThreadPool>
#include <QFuture>
#include <QWaitCondition>
#include <akpacket.h>

extern "C"
{
    #include <libavutil/mathematics.h>
}

#include "abstractstream.h"
#include "mediawriterffmpeg.h"

template <typename T>
inline void waitLoop(const QFuture<T> &loop)
{
    while (!loop.isFinished()) {
        auto eventDispatcher = QThread::currentThread()->eventDispatcher();

        if (eventDispatcher)
            eventDispatcher->processEvents(QEventLoop::AllEvents);
    }
}

class AbstractStreamPrivate
{
    public:
        AbstractStream *self;
        uint m_index {0};
        int m_streamIndex {-1};
        AVMediaType m_mediaType {AVMEDIA_TYPE_UNKNOWN};
        AVFormatContext *m_formatContext {nullptr};
        AVCodecContext *m_codecContext {nullptr};
        AVStream *m_stream {nullptr};
        QThreadPool m_threadPool;
        AVDictionary *m_codecOptions {nullptr};

        // Packet queue and convert loop.
        QQueue<AkPacket> m_packetQueue;
        QMutex m_convertMutex;
        QWaitCondition m_packetQueueNotFull;
        QWaitCondition m_packetQueueNotEmpty;
        QFuture<void> m_convertLoopResult;
        bool m_runConvertLoop {false};

        // Frame queue and encoding loop.
        QFuture<void> m_encodeLoopResult;
        bool m_runEncodeLoop {false};

        explicit AbstractStreamPrivate(AbstractStream *self):
            self(self)
        {
        }

        void convertLoop();
        void encodeLoop();
};

AbstractStream::AbstractStream(const AVFormatContext *formatContext,
                               uint index,
                               int streamIndex,
                               const QVariantMap &configs,
                               const QMap<QString, QVariantMap> &codecOptions,
                               MediaWriterFFmpeg *mediaWriter,
                               QObject *parent):
    QObject(parent)
{
    Q_UNUSED(mediaWriter)

    this->d = new AbstractStreamPrivate(this);
    this->m_maxPacketQueueSize = 9;
    this->d->m_index = index;
    this->d->m_streamIndex = streamIndex;
    this->d->m_formatContext = const_cast<AVFormatContext *>(formatContext);

    this->d->m_stream =
            (formatContext && index < formatContext->nb_streams)?
                formatContext->streams[index]: nullptr;

    QString codecName = configs["codec"].toString();
    auto codec = avcodec_find_encoder_by_name(codecName.toStdString().c_str());
    this->d->m_codecContext = avcodec_alloc_context3(codec);

    // Some formats want stream headers to be separate.
    if (formatContext->oformat->flags & AVFMT_GLOBALHEADER)
        this->d->m_codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    this->d->m_codecContext->strict_std_compliance = CODEC_COMPLIANCE;

    // Set codec options.
    auto optKey = QString("%1/%2/%3").arg(formatContext->oformat->name)
                                     .arg(streamIndex)
                                     .arg(codecName);
    auto options = codecOptions.value(optKey);

    if (codecName == "libvpx") {
        if (!options.contains("deadline"))
            options["deadline"] = "realtime";

        if (!options.contains("quality"))
            options["quality"] = "realtime";
    } else if (codecName == "libx265") {
        if (!options.contains("preset"))
            options["preset"] = "ultrafast";
    }

    for (auto it = options.begin(); it != options.end(); it++) {
        QString value = it.value().toString();

        if (!value.isEmpty())
            av_dict_set(&this->d->m_codecOptions,
                        it.key().toStdString().c_str(),
                        value.toStdString().c_str(),
                        0);
    }

    if (this->d->m_threadPool.maxThreadCount() < 2)
        this->d->m_threadPool.setMaxThreadCount(2);
}

AbstractStream::~AbstractStream()
{
    this->uninit();

    if (this->d->m_codecContext)
        avcodec_free_context(&this->d->m_codecContext);

    delete this->d;
}

uint AbstractStream::index() const
{
    return this->d->m_index;
}

int AbstractStream::streamIndex() const
{
    return this->d->m_streamIndex;
}

AVMediaType AbstractStream::mediaType() const
{
    return this->d->m_mediaType;
}

AVStream *AbstractStream::stream() const
{
    return this->d->m_stream;
}

AVFormatContext *AbstractStream::formatContext() const
{
    return this->d->m_formatContext;
}

AVCodecContext *AbstractStream::codecContext() const
{
    return this->d->m_codecContext;
}

void AbstractStream::packetEnqueue(const AkPacket &packet)
{
    if (!this->d->m_runConvertLoop)
        return;

    this->d->m_convertMutex.lock();
    bool enqueue = true;

    if (this->d->m_packetQueue.size() >= this->m_maxPacketQueueSize)
        enqueue = this->d->m_packetQueueNotFull.wait(&this->d->m_convertMutex,
                                                     THREAD_WAIT_LIMIT);

    if (enqueue) {
        this->d->m_packetQueue << packet;
        this->d->m_packetQueueNotEmpty.wakeAll();
    }

    this->d->m_convertMutex.unlock();
}

void AbstractStream::convertPacket(const AkPacket &packet)
{
    Q_UNUSED(packet)
}

int AbstractStream::encodeData(AVFrame *frame)
{
    Q_UNUSED(frame)

    return AVERROR_EOF;
}

AVFrame *AbstractStream::dequeueFrame()
{
    return nullptr;
}

void AbstractStream::rescaleTS(AVPacket *pkt, AVRational src, AVRational dst)
{
    av_packet_rescale_ts(pkt, src, dst);
}

void AbstractStream::deleteFrame(AVFrame **frame)
{
    if (!frame || !*frame)
        return;

    av_frame_unref(*frame);
    av_frame_free(frame);
    *frame = nullptr;
}

void AbstractStreamPrivate::convertLoop()
{
    while (this->m_runConvertLoop) {
        this->m_convertMutex.lock();
        bool gotPacket = true;

        if (this->m_packetQueue.isEmpty())
            gotPacket = this->m_packetQueueNotEmpty.wait(&this->m_convertMutex,
                                                         THREAD_WAIT_LIMIT);

        AkPacket packet;

        if (gotPacket) {
            packet = this->m_packetQueue.dequeue();
            this->m_packetQueueNotFull.wakeAll();
        }

        this->m_convertMutex.unlock();

        if (packet)
            self->convertPacket(packet);
    }
}

void AbstractStreamPrivate::encodeLoop()
{
    while (this->m_runEncodeLoop) {
        if (auto frame = self->dequeueFrame()) {
            self->encodeData(frame);
            self->deleteFrame(&frame);
        }
    }

    // Flush encoders
    while (self->encodeData(nullptr) == AVERROR(EAGAIN)) {
    }
}

bool AbstractStream::init()
{
    if (!this->d->m_codecContext)
        return false;

    int result = avcodec_open2(this->d->m_codecContext,
                               this->d->m_codecContext->codec,
                               &this->d->m_codecOptions);

    if (result < 0) {
        char error[1024];
        av_strerror(result, error, 1024);
        qDebug() << "Error: " << error;

        return false;
    }

    avcodec_parameters_from_context(this->d->m_stream->codecpar,
                                    this->d->m_codecContext);
    this->d->m_runEncodeLoop = true;
    this->d->m_encodeLoopResult =
            QtConcurrent::run(&this->d->m_threadPool,
                              this->d,
                              &AbstractStreamPrivate::encodeLoop);

    this->d->m_runConvertLoop = true;

    this->d->m_convertLoopResult =
            QtConcurrent::run(&this->d->m_threadPool,
                              this->d,
                              &AbstractStreamPrivate::convertLoop);

    return true;
}

void AbstractStream::uninit()
{
    this->d->m_runConvertLoop = false;
    waitLoop(this->d->m_convertLoopResult);

    this->d->m_runEncodeLoop = false;
    waitLoop(this->d->m_encodeLoopResult);

    avcodec_close(this->d->m_codecContext);

    if (this->d->m_codecOptions)
        av_dict_free(&this->d->m_codecOptions);

    this->d->m_packetQueue.clear();
}

#include "moc_abstractstream.cpp"
