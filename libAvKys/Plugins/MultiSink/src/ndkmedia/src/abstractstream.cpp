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
        AMediaCodec *m_codec {nullptr};
        AMediaFormat *m_mediaFormat {nullptr};
        AkCaps m_caps;
        QThreadPool m_threadPool;
        uint m_index {0};
        int m_streamIndex {-1};

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

        explicit AbstractStreamPrivate(AbstractStream *self);
        void convertLoop();
        void encodeLoop();
};

AbstractStream::AbstractStream(AMediaMuxer *mediaMuxer,
                               uint index, int streamIndex,
                               const QVariantMap &configs,
                               MediaWriterNDKMedia *mediaWriter,
                               QObject *parent):
    QObject(parent)
{
    Q_UNUSED(mediaWriter)

    this->d = new AbstractStreamPrivate(this);
    this->m_maxPacketQueueSize = 9;
    this->d->m_index = index;
    this->d->m_streamIndex = streamIndex;
    this->d->m_mediaMuxer = mediaMuxer;
    this->d->m_caps = configs["caps"].value<AkCaps>();

    QString codecName = configs["codec"].toString();
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
/*    AMediaFormat_setInt64(this->d->m_mediaFormat,
                          AMEDIAFORMAT_KEY_DURATION,
                          0);*/
    AMediaFormat_setString(this->d->m_mediaFormat,
                           AMEDIAFORMAT_KEY_LANGUAGE,
                           "und");
    this->d->m_codec =
            AMediaCodec_createEncoderByType(codecName.toStdString().c_str());

    if (this->d->m_threadPool.maxThreadCount() < 2)
        this->d->m_threadPool.setMaxThreadCount(2);
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

AkCaps AbstractStream::caps() const
{
    return this->d->m_caps;
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

bool AbstractStream::encodeData(bool eos)
{
    Q_UNUSED(eos)

    return false;
}

AkPacket AbstractStream::avPacketDequeue()
{
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

void AbstractStreamPrivate::encodeLoop()
{
    while (this->m_runEncodeLoop)
        self->encodeData();

    self->encodeData(true);
}

bool AbstractStream::init()
{
    if (!this->d->m_codec)
        return false;

    if (AMediaMuxer_addTrack(this->d->m_mediaMuxer,
                             this->d->m_mediaFormat) < 0)
        return false;

    if (AMediaCodec_configure(this->d->m_codec,
                              this->d->m_mediaFormat,
                              nullptr,
                              nullptr,
                              AMEDIACODEC_CONFIGURE_FLAG_ENCODE) != AMEDIA_OK)
        return false;

    if (AMediaCodec_start(this->d->m_codec) != AMEDIA_OK)
        return false;

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

    AMediaCodec_stop(this->d->m_codec);
    this->d->m_packetQueue.clear();
}

#include "moc_abstractstream.cpp"
