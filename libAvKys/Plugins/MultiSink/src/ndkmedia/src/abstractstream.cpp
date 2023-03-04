/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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
#include <akfrac.h>
#include <akcaps.h>
#include <akaudiocaps.h>
#include <akvideocaps.h>
#include <akpacket.h>
#include <media/NdkMediaMuxer.h>

#include "abstractstream.h"
#include "audiostream.h"
#include "videostream.h"
#include "mediawriterndkmedia.h"

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
        QString m_mimeType;
        AMediaMuxer *m_mediaMuxer {nullptr};
        MediaWriterNDKMedia *m_mediaWriter {nullptr};
        AMediaCodec *m_codec {nullptr};
        AMediaFormat *m_mediaFormat {nullptr};
        QThreadPool m_threadPool;
        qint64 m_id {-1};
        qint64 m_pts {0};
        qint64 m_ptsDiff {0};
        qint64 m_ptsDrift {0};
        QQueue<AkPacket> m_packetQueue;
        QMutex m_convertMutex;
        QWaitCondition m_packetQueueNotFull;
        QWaitCondition m_packetQueueNotEmpty;
        QFuture<void> m_convertLoopResult;
        QFuture<void> m_equeueLoopResult;
        QFuture<void> m_dequeueLoopResult;
        uint m_index {0};
        int m_streamIndex {-1};
        bool m_runConvertLoop {false};
        bool m_runEqueueLoop {false};
        bool m_runDequeueLoop {false};
        bool m_setMediaFormat {false};
        bool m_ready {false};

        explicit AbstractStreamPrivate(AbstractStream *self);
        void convertLoop();
        void equeueLoop();
        void dequeueLoop();
        qint64 nextPts(qint64 pts, qint64 id);
};

AbstractStream::AbstractStream(AMediaMuxer *mediaMuxer,
                               uint index, int streamIndex,
                               const QVariantMap &configs,
                               const QMap<QString, QVariantMap> &codecOptions,
                               MediaWriterNDKMedia *mediaWriter,
                               QObject *parent):
    QObject(parent)
{
    this->d = new AbstractStreamPrivate(this);
    this->m_maxPacketQueueSize = 9;
    this->d->m_index = index;
    this->d->m_streamIndex = streamIndex;
    this->d->m_mediaMuxer = mediaMuxer;
    this->d->m_mediaWriter = mediaWriter;

    QString codecName = configs["codec"].toString();
    this->d->m_codec =
            AMediaCodec_createEncoderByType(codecName.toStdString().c_str());
    this->d->m_mediaFormat = AMediaFormat_new();
    AMediaFormat_setString(this->d->m_mediaFormat,
                           AMEDIAFORMAT_KEY_MIME,
                           codecName.toStdString().c_str());

    auto bitrate = configs["bitrate"].toInt();

    if (bitrate < 1)
        bitrate = configs["defaultBitRate"].toInt();

    AMediaFormat_setInt32(this->d->m_mediaFormat,
                          AMEDIAFORMAT_KEY_BIT_RATE,
                          bitrate);
    //AMediaFormat_setInt64(this->d->m_mediaFormat,
    //                    AMEDIAFORMAT_KEY_DURATION,
    //                    0);
    AMediaFormat_setString(this->d->m_mediaFormat,
                           AMEDIAFORMAT_KEY_LANGUAGE,
                           "und");

    // Set codec options.
    auto optKey =
            QString("%1/%2/%3").arg(this->d->m_mediaWriter->outputFormat())
                               .arg(streamIndex)
                               .arg(codecName);
    auto options = codecOptions.value(optKey);

    for (auto it = options.begin(); it != options.end(); it++)
        AMediaFormat_setInt32(this->d->m_mediaFormat,
                              it.key().toStdString().c_str(),
                              it.value().toInt());

    if (this->d->m_threadPool.maxThreadCount() < 4)
        this->d->m_threadPool.setMaxThreadCount(4);
}

AbstractStream::~AbstractStream()
{
    this->uninit();

    if (this->d->m_mediaFormat)
        AMediaFormat_delete(this->d->m_mediaFormat);

    if (this->d->m_codec)
        AMediaCodec_delete(this->d->m_codec);

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

QString AbstractStream::mimeType() const
{
    return this->d->m_mimeType;
}

AMediaCodec *AbstractStream::codec() const
{
    return this->d->m_codec;
}

AMediaFormat *AbstractStream::mediaFormat() const
{
    return this->d->m_mediaFormat;
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

bool AbstractStream::ready() const
{
    return this->d->m_ready;
}

void AbstractStream::convertPacket(const AkPacket &packet)
{
    Q_UNUSED(packet)
}

void AbstractStream::encode(const AkPacket &packet,
                            uint8_t *buffer,
                            size_t bufferSize)
{
    Q_UNUSED(packet)
    Q_UNUSED(buffer)
    Q_UNUSED(bufferSize)
}

AkPacket AbstractStream::avPacketDequeue(size_t bufferSize)
{
    Q_UNUSED(bufferSize)

    return {};
}

AbstractStreamPrivate::AbstractStreamPrivate(AbstractStream *self):
    self(self)
{
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

void AbstractStreamPrivate::equeueLoop()
{
    const ssize_t timeOut = 5000;
    bool eosSent = false;
    qint64 pts = 0;
    qint64 ptsDiff = 0;

    while (this->m_runEqueueLoop) {
        auto bufferIndex =
                AMediaCodec_dequeueInputBuffer(this->m_codec, timeOut);

        if (bufferIndex < 0)
            continue;

        size_t bufferSize = 0;
        auto buffer = AMediaCodec_getInputBuffer(this->m_codec,
                                                 size_t(bufferIndex),
                                                 &bufferSize);
        AkPacket packet;

        while (this->m_runEqueueLoop) {
            packet = self->avPacketDequeue(bufferSize);

            if (packet)
                break;
        }

        if (this->m_runEqueueLoop) {
            auto presentationTimeUs =
                    qRound64(1e6 * packet.pts() * packet.timeBase().value());
            presentationTimeUs = this->nextPts(presentationTimeUs, packet.id());
            self->encode(packet,
                         buffer,
                         bufferSize);
            AMediaCodec_queueInputBuffer(this->m_codec,
                                         bufferIndex,
                                         0,
                                         bufferSize,
                                         size_t(presentationTimeUs),
                                         0);
            ptsDiff = presentationTimeUs - pts;
            pts = presentationTimeUs;
        } else {
            auto presentationTimeUs = pts + ptsDiff;
            AMediaCodec_queueInputBuffer(this->m_codec,
                                         size_t(bufferIndex),
                                         0,
                                         0,
                                         size_t(presentationTimeUs),
                                         AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
            eosSent = true;
        }
    }

    // End Stream
    if (!eosSent) {
        auto bufferIndex =
                AMediaCodec_dequeueInputBuffer(this->m_codec, timeOut);

        if (bufferIndex >= 0) {
            AMediaCodec_queueInputBuffer(this->m_codec,
                                         size_t(bufferIndex),
                                         0,
                                         0,
                                         0,
                                         AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
        }
    }
}

void AbstractStreamPrivate::dequeueLoop()
{
    const ssize_t timeOut = 5000;
    bool eos = false;

    while (!eos) {
        AMediaCodecBufferInfo info;
        memset(&info, 0, sizeof(AMediaCodecBufferInfo));
        auto bufferIndex = AMediaCodec_dequeueOutputBuffer(this->m_codec,
                                                           &info,
                                                           timeOut);

        if (bufferIndex < 0)
            continue;

        if (info.flags & AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG)
            continue;

        if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM)
            eos = true;

        size_t bufferSize = 0;
        auto data = AMediaCodec_getOutputBuffer(this->m_codec,
                                                size_t(bufferIndex),
                                                &bufferSize);

        if (!this->m_ready) {
            if (this->m_mediaFormat)
                AMediaFormat_delete(this->m_mediaFormat);

            this->m_mediaFormat = AMediaCodec_getOutputFormat(this->m_codec);
            this->m_ready = true;

            while (!this->m_mediaWriter->startMuxing()
                   && this->m_runDequeueLoop)
                QThread::msleep(500);
        }

        emit self->packetReady(this->m_index, data, &info);
        AMediaCodec_releaseOutputBuffer(this->m_codec,
                                        size_t(bufferIndex),
                                        info.size != 0);
    }
}

qint64 AbstractStreamPrivate::nextPts(qint64 pts, qint64 id)
{
    if (this->m_pts < 0 || this->m_id < 0) {
        this->m_ptsDrift = -pts;
        this->m_pts = pts;
        this->m_id = id;

        return 0;
    }

    if (pts <= this->m_pts || id != this->m_id) {
        this->m_ptsDrift += this->m_pts - pts + this->m_ptsDiff;
        this->m_pts = pts;
        this->m_id = id;

        return pts + this->m_ptsDrift;
    }

    this->m_ptsDiff = pts - this->m_pts;
    this->m_pts = pts;

    return pts + this->m_ptsDrift;
}

bool AbstractStream::init()
{
    if (!this->d->m_codec)
        return false;

    this->d->m_setMediaFormat = false;

    if (AMediaCodec_configure(this->d->m_codec,
                              this->d->m_mediaFormat,
                              nullptr,
                              nullptr,
                              AMEDIACODEC_CONFIGURE_FLAG_ENCODE) != AMEDIA_OK)
        return false;

    if (AMediaCodec_start(this->d->m_codec) != AMEDIA_OK)
        return false;

    this->d->m_runDequeueLoop = true;
    this->d->m_dequeueLoopResult =
            QtConcurrent::run(&this->d->m_threadPool,
                              this->d,
                              &AbstractStreamPrivate::dequeueLoop);

    this->d->m_runEqueueLoop = true;
    this->d->m_equeueLoopResult =
            QtConcurrent::run(&this->d->m_threadPool,
                              this->d,
                              &AbstractStreamPrivate::equeueLoop);

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

    this->d->m_runEqueueLoop = false;
    waitLoop(this->d->m_equeueLoopResult);

    this->d->m_runDequeueLoop = false;
    waitLoop(this->d->m_dequeueLoopResult);

    AMediaCodec_stop(this->d->m_codec);
    this->d->m_packetQueue.clear();

    this->d->m_ready = false;
}

#include "moc_abstractstream.cpp"
