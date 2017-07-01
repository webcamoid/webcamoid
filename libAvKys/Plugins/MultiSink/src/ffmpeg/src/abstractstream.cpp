/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include <QAbstractEventDispatcher>

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

AbstractStream::AbstractStream(const AVFormatContext *formatContext,
                               uint index,
                               int streamIndex,
                               const QVariantMap &configs,
                               const QMap<QString, QVariantMap> &codecOptions,
                               QObject *parent):
    QObject(parent)
{
    this->m_maxPacketQueueSize = 9;
    this->m_maxFrameQueueSize = 1;
    this->m_runConvertLoop = false;
    this->m_runEncodeLoop = false;
    this->m_frameQueueSize = 0;
    this->m_index = index;
    this->m_mediaType = AVMEDIA_TYPE_UNKNOWN;
    this->m_stream = NULL;
    this->m_codecOptions = NULL;
    this->m_frameQueue = NULL;

    this->m_stream = (formatContext && index < formatContext->nb_streams)?
                         formatContext->streams[index]: NULL;

    QString codecName = configs["codec"].toString();
    AVCodec *codec = avcodec_find_encoder_by_name(codecName.toStdString().c_str());
    this->m_codecContext = avcodec_alloc_context3(codec);

    // Some formats want stream headers to be separate.
    if (formatContext->oformat->flags & AVFMT_GLOBALHEADER)
        this->m_codecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;

    this->m_codecContext->strict_std_compliance = CODEC_COMPLIANCE;

    // Set codec options.
    auto optKey = QString("%1/%2/%3").arg(formatContext->oformat->name)
                                     .arg(streamIndex)
                                     .arg(codecName);
    QVariantMap options = codecOptions.value(optKey);

    if (codecName == "libvpx") {
        if (!options.contains("deadline"))
            options["deadline"] = "realtime";

        if (!options.contains("quality"))
            options["quality"] = "realtime";
    } else if (codecName == "libx265") {
        if (!options.contains("preset"))
            options["preset"] = "ultrafast";
    }

    for (const QString &key: options.keys()) {
        QString value = options[key].toString();

        av_dict_set(&this->m_codecOptions,
                    key.toStdString().c_str(),
                    value.toStdString().c_str(),
                    0);
    }
}

AbstractStream::~AbstractStream()
{
}

uint AbstractStream::index() const
{
    return this->m_index;
}

AVMediaType AbstractStream::mediaType() const
{
    return this->m_mediaType;
}

AVStream *AbstractStream::stream() const
{
    return this->m_stream;
}

AVCodecContext *AbstractStream::codecContext() const
{
    return this->m_codecContext;
}

void AbstractStream::packetEnqueue(const AkPacket &packet)
{
    while (this->m_runConvertLoop) {
        this->m_convertMutex.lock();
        bool enqueue = true;

        if (this->m_packetQueue.size() >= this->m_maxPacketQueueSize)
            enqueue = this->m_packetQueueNotFull.wait(&this->m_convertMutex,
                                                      THREAD_WAIT_LIMIT);

        if (enqueue) {
            this->m_packetQueue << packet;
            this->m_packetQueueNotEmpty.wakeAll();
        }

        this->m_convertMutex.unlock();
    }
}

void AbstractStream::convertPacket(const AkPacket &packet)
{
    Q_UNUSED(packet);
}

void AbstractStream::encodeData(const AVFrame *frame)
{
    Q_UNUSED(frame);
}

AVFrame *AbstractStream::dequeueFrame()
{
    return NULL;
}

void AbstractStream::convertLoop()
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
            this->convertPacket(packet);
    }
}

void AbstractStream::encodeLoop()
{
    while (this->m_runEncodeLoop) {
        this->m_encodeMutex.lock();
        bool gotFrame = true;

        if (this->m_frameQueueSize < this->m_maxFrameQueueSize)
            gotFrame = this->m_frameQueueNotEmpty.wait(&this->m_encodeMutex,
                                                       THREAD_WAIT_LIMIT);

        AVFrame *frame = NULL;

        if (gotFrame) {
            frame = this->dequeueFrame();

            if (this->m_frameQueueSize < this->m_maxFrameQueueSize)
                this->m_frameQueueNotFull.wakeAll();
        }

        this->m_encodeMutex.unlock();

        if (frame) {
            this->encodeData(frame);
            av_frame_free(&frame);
        }
    }
}

bool AbstractStream::init()
{
    if (!this->m_codecContext)
        return false;

    if (avcodec_open2(this->m_codecContext,
                      this->m_codecContext->codec,
                      &this->m_codecOptions) < 0)
        return false;

    this->m_runEncodeLoop = true;

    this->m_encodeLoopResult =
            QtConcurrent::run(&this->m_threadPool,
                              this,
                              &AbstractStream::encodeLoop);

    this->m_runConvertLoop = true;

    this->m_convertLoopResult =
            QtConcurrent::run(&this->m_threadPool,
                              this,
                              &AbstractStream::convertLoop);

    return true;
}

void AbstractStream::uninit()
{
    this->m_runConvertLoop = false;
    waitLoop(this->m_convertLoopResult);

    this->m_runEncodeLoop = false;
    waitLoop(this->m_encodeLoopResult);

    if (this->m_codecOptions)
        av_dict_free(&this->m_codecOptions);

    if (this->m_codecContext) {
        avcodec_close(this->m_codecContext);
        this->m_codecContext = NULL;
    }

    this->m_packetQueue.clear();
    av_frame_free(&this->m_frameQueue);
    this->m_frameQueue = NULL;
}
