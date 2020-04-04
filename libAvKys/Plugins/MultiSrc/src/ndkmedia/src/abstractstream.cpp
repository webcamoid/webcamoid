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
#include <QEventLoop>
#include <QtConcurrent>
#include <QFuture>
#include <QThread>
#include <QThreadPool>
#include <QWaitCondition>
#include <akfrac.h>
#include <akcaps.h>
#include <akpacket.h>
#include <media/NdkMediaExtractor.h>

#include "abstractstream.h"
#include "clock.h"

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
        AkFrac m_timeBase;
        AMediaExtractor *m_mediaExtractor {nullptr};
        AMediaCodec *m_codec {nullptr};
        AMediaFormat *m_mediaFormat {nullptr};
        QThreadPool m_threadPool;
        QMutex m_dataMutex;
        QWaitCondition m_dataQueueNotEmpty;
        QWaitCondition m_dataQueueNotFull;
        QQueue<AkPacket> m_packets;
        qint64 m_packetQueueSize {-1};
        Clock *m_globalClock {nullptr};
        QFuture<void> m_dataLoopResult;
        QString m_mimeType;
        qint64 m_id {-1};
        uint m_index {0};
        bool m_runDataLoop {false};

        explicit AbstractStreamPrivate(AbstractStream *self);
        void dataLoop();
};

AbstractStream::AbstractStream(AMediaExtractor *mediaExtractor,
                               uint index, qint64 id, Clock *globalClock,
                               QObject *parent):
    QObject(parent)
{
    this->d = new AbstractStreamPrivate(this);
    this->d->m_mediaExtractor = mediaExtractor;
    this->m_paused = false;
    this->m_isValid = false;
    this->m_clockDiff = 0;
    this->m_maxData = 0;
    this->d->m_index = index;
    this->d->m_id = id;

    this->d->m_mediaFormat =
            AMediaExtractor_getTrackFormat(this->d->m_mediaExtractor,
                                           index);
    const char *mime = nullptr;
    AMediaFormat_getString(this->d->m_mediaFormat,
                           AMEDIAFORMAT_KEY_MIME,
                           &mime);
    this->d->m_codec = AMediaCodec_createDecoderByType(mime);

    if (!this->d->m_codec)
        return;

    if (QString(mime).startsWith("audio/")) {
        this->d->m_mimeType = "audio/x-raw";
        int32_t rate = 0;
        AMediaFormat_getInt32(this->d->m_mediaFormat,
                              AMEDIAFORMAT_KEY_SAMPLE_RATE,
                              &rate);
        this->d->m_timeBase = AkFrac(1, rate);
    } else if (QString(mime).startsWith("video/")) {
        this->d->m_mimeType = "video/x-raw";
        int32_t frameRate;
        AMediaFormat_getInt32(this->d->m_mediaFormat,
                              AMEDIAFORMAT_KEY_FRAME_RATE,
                              &frameRate);
        this->d->m_timeBase = AkFrac(1, frameRate);
    }

    this->d->m_packetQueueSize = 0;
    this->d->m_globalClock = globalClock;

    this->m_isValid = true;

    if (this->d->m_threadPool.maxThreadCount() < 2)
        this->d->m_threadPool.setMaxThreadCount(2);
}

AbstractStream::~AbstractStream()
{
    if (this->d->m_codec)
        AMediaCodec_delete(this->d->m_codec);

    if (this->d->m_mediaFormat)
        AMediaFormat_delete(this->d->m_mediaFormat);

    delete this->d;
}

bool AbstractStream::paused() const
{
    return this->m_paused;
}

bool AbstractStream::isValid() const
{
    return this->m_isValid;
}

uint AbstractStream::index() const
{
    return this->d->m_index;
}

qint64 AbstractStream::id() const
{
    return this->d->m_id;
}

AkFrac AbstractStream::timeBase() const
{
    return this->d->m_timeBase;
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

AkCaps AbstractStream::caps() const
{
    return AkCaps();
}

bool AbstractStream::packetEnqueue(bool eos)
{
    ssize_t timeOut = 5000;
    auto bufferIndex =
            AMediaCodec_dequeueInputBuffer(this->d->m_codec, timeOut);

    if (bufferIndex < 0)
        return false;

    if (eos)  {
        AMediaCodec_queueInputBuffer(this->d->m_codec,
                                     size_t(bufferIndex),
                                     0,
                                     0,
                                     0,
                                     AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
    } else {
        size_t buffersize = 0;
        auto buffer = AMediaCodec_getInputBuffer(this->d->m_codec,
                                                 size_t(bufferIndex),
                                                 &buffersize);

        if (!buffer)
            return false;

        auto sampleSize =
                AMediaExtractor_readSampleData(this->d->m_mediaExtractor,
                                               buffer,
                                               buffersize);

        if (sampleSize < 1)
            return false;

        auto presentationTimeUs =
                AMediaExtractor_getSampleTime(this->d->m_mediaExtractor);
        AMediaCodec_queueInputBuffer(this->d->m_codec,
                                     size_t(bufferIndex),
                                     0,
                                     size_t(sampleSize),
                                     uint64_t(presentationTimeUs),
                                     0);
    }

    return true;
}

void AbstractStream::avPacketEnqueue(const AkPacket &packet)
{
    this->d->m_dataMutex.lock();

    if (this->d->m_packets.size() >= this->m_maxData)
        this->d->m_dataQueueNotFull.wait(&this->d->m_dataMutex);

    if (packet)
        this->d->m_packets.enqueue(packet);
    else
        this->d->m_packets.enqueue({});

    this->d->m_dataQueueNotEmpty.wakeAll();
    this->d->m_dataMutex.unlock();
}

qint64 AbstractStream::queueSize()
{
    return this->d->m_packetQueueSize;
}

Clock *AbstractStream::globalClock()
{
    return this->d->m_globalClock;
}

qreal AbstractStream::clockDiff() const
{
    return this->m_clockDiff;
}

qreal &AbstractStream::clockDiff()
{
    return this->m_clockDiff;
}

bool AbstractStream::decodeData()
{
    return false;
}

QString AbstractStream::mimeType(AMediaExtractor *mediaExtractor,
                                 uint index)
{
    auto format = AMediaExtractor_getTrackFormat(mediaExtractor, index);

    if (!format)
        return {};

    const char *mime = nullptr;
    AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime);
    auto mimeType = QString(mime).startsWith("audio/")?
                        "audio/x-raw":
                    QString(mime).startsWith("video/")?
                        "video/x-raw":
                        QString();
    AMediaFormat_delete(format);

    return mimeType;
}

void AbstractStream::processPacket(const AkPacket &packet)
{
    Q_UNUSED(packet)
}

AbstractStreamPrivate::AbstractStreamPrivate(AbstractStream *self):
    self(self)
{
}

void AbstractStreamPrivate::dataLoop()
{
    if (this->m_mimeType == "audio/x-raw"
        || this->m_mimeType == "video/x-raw") {
        while (this->m_runDataLoop) {
            this->m_dataMutex.lock();
            bool gotFrame = true;

            if (this->m_packets.isEmpty())
                gotFrame = this->m_dataQueueNotEmpty.wait(&this->m_dataMutex,
                                                          THREAD_WAIT_LIMIT);

            AkPacket packet;

            if (gotFrame) {
                packet = this->m_packets.dequeue();

                if (this->m_packets.size() < self->m_maxData)
                    this->m_dataQueueNotFull.wakeAll();
            }

            this->m_dataMutex.unlock();

            if (gotFrame) {
                if (packet)
                    self->processPacket(packet);
                else {
                    emit self->eof();
                    this->m_runDataLoop = false;
                }
            }
        }
    }
}

void AbstractStream::flush()
{
    this->d->m_dataMutex.lock();
    this->d->m_packets.clear();
    this->d->m_dataMutex.unlock();
}

void AbstractStream::setPaused(bool paused)
{
    if (this->m_paused == paused)
        return;

    this->d->m_runDataLoop = !paused;

    if (paused)
        this->d->m_dataLoopResult.waitForFinished();
    else
        this->d->m_dataLoopResult =
            QtConcurrent::run(&this->d->m_threadPool,
                              this->d,
                              &AbstractStreamPrivate::dataLoop);

    this->m_paused = paused;
    emit this->pausedChanged(paused);
}

void AbstractStream::resetPaused()
{
    this->setPaused(false);
}

bool AbstractStream::init(bool paused)
{
    if (!this->d->m_codec)
        return false;

    if (AMediaCodec_configure(this->d->m_codec,
                              this->d->m_mediaFormat,
                              nullptr,
                              nullptr,
                              0) != AMEDIA_OK)
        return false;

    if (AMediaCodec_start(this->d->m_codec) != AMEDIA_OK)
        return false;

    if (AMediaExtractor_selectTrack(this->d->m_mediaExtractor,
                                    this->d->m_index) != AMEDIA_OK)
        return false;

    this->m_clockDiff = 0;
    this->d->m_runDataLoop = !paused;

    if (this->d->m_runDataLoop)
        this->d->m_dataLoopResult =
                QtConcurrent::run(&this->d->m_threadPool,
                                  this->d,
                                  &AbstractStreamPrivate::dataLoop);

    return true;
}

void AbstractStream::uninit()
{
    this->d->m_runDataLoop = false;
    waitLoop(this->d->m_dataLoopResult);

    AMediaCodec_stop(this->d->m_codec);
    this->d->m_packets.clear();
}

#include "moc_abstractstream.cpp"
