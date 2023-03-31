/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

using PacketPtr = QSharedPointer<AVPacket>;
using FramePtr = QSharedPointer<AVFrame>;
using SubtitlePtr = QSharedPointer<AVSubtitle>;

class AbstractStreamPrivate
{
    public:
        AbstractStream *self;
        AkFrac m_timeBase;
        AVStream *m_stream {nullptr};
        AVCodecContext *m_codecContext {nullptr};
        const AVCodec *m_codec {nullptr};
        AVDictionary *m_codecOptions {nullptr};
        QThreadPool m_threadPool;
        QMutex m_packetMutex;
        QMutex m_dataMutex;
        QWaitCondition m_packetQueueNotEmpty;
        QWaitCondition m_dataQueueNotEmpty;
        QWaitCondition m_dataQueueNotFull;
        QQueue<PacketPtr> m_packets;
        QQueue<FramePtr> m_frames;
        QQueue<SubtitlePtr> m_subtitles;
        qint64 m_packetQueueSize {0};
        Clock *m_globalClock {nullptr};
        QFuture<void> m_packetLoopResult;
        QFuture<void> m_dataLoopResult;
        qint64 m_id {-1};
        uint m_index {0};
        AVMediaType m_mediaType {AVMEDIA_TYPE_UNKNOWN};
        AkElement::ElementState m_state {AkElement::ElementStateNull};
        bool m_sync {true};
        bool m_runPacketLoop {false};
        bool m_run {false};
        bool m_paused {false};

        explicit AbstractStreamPrivate(AbstractStream *self);
        void packetLoop();
        void readPacket();
        void dataLoop();
        void readData();
        static void deletePacket(AVPacket *packet);
        static void deleteFrame(AVFrame *frame);
        static void deleteSubtitle(AVSubtitle *subtitle);
};

AbstractStream::AbstractStream(const AVFormatContext *formatContext,
                               uint index,
                               qint64 id,
                               Clock *globalClock,
                               bool sync,
                               bool noModify,
                               QObject *parent): QObject(parent)
{
    this->d = new AbstractStreamPrivate(this);
    this->m_isValid = false;
    this->m_clockDiff = 0;
    this->m_maxData = 0;
    this->d->m_index = index;
    this->d->m_id = id;
    this->d->m_sync = sync;

    this->d->m_stream = (formatContext && index < formatContext->nb_streams)?
                         formatContext->streams[index]: nullptr;

    this->d->m_mediaType = this->d->m_stream?
                               this->d->m_stream->codecpar->codec_type:
                               AVMEDIA_TYPE_UNKNOWN;

    this->d->m_codecContext = nullptr;

    if (this->d->m_stream) {
        this->d->m_codecContext = avcodec_alloc_context3(nullptr);

        if (avcodec_parameters_to_context(this->d->m_codecContext,
                                          this->d->m_stream->codecpar) < 0)
            avcodec_free_context(&this->d->m_codecContext);
    }

    this->d->m_codec =
            this->d->m_codecContext?
                avcodec_find_decoder(this->d->m_codecContext->codec_id):
                nullptr;

    this->d->m_codecOptions = nullptr;
    this->d->m_globalClock = globalClock;

    if (!this->d->m_codec)
        return;

    if (this->d->m_stream)
        this->d->m_timeBase =
            AkFrac(this->d->m_stream->time_base.num,
                   this->d->m_stream->time_base.den);

    if (!noModify) {
        if (this->d->m_stream)
            this->d->m_stream->discard = AVDISCARD_DEFAULT;

        this->d->m_codecContext->workaround_bugs = 1;
        this->d->m_codecContext->idct_algo = FF_IDCT_AUTO;
        this->d->m_codecContext->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;

#ifdef CODEC_FLAG_EMU_EDGE
        if (this->d->m_codec->capabilities & CODEC_CAP_DR1)
            this->d->m_codecContext->flags |= CODEC_FLAG_EMU_EDGE;
#endif

        av_dict_set(&this->d->m_codecOptions, "refcounted_frames", "0", 0);
    }

    this->m_isValid = true;

    if (this->d->m_threadPool.maxThreadCount() < 2)
        this->d->m_threadPool.setMaxThreadCount(2);
}

AbstractStream::~AbstractStream()
{
    this->setState(AkElement::ElementStateNull);

    if (this->d->m_codecContext)
        avcodec_free_context(&this->d->m_codecContext);

    delete this->d;
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

AVMediaType AbstractStream::mediaType() const
{
    return this->d->m_mediaType;
}

AVStream *AbstractStream::stream() const
{
    return this->d->m_stream;
}

AVCodecContext *AbstractStream::codecContext() const
{
    return this->d->m_codecContext;
}

const AVCodec *AbstractStream::codec() const
{
    return this->d->m_codec;
}

AVDictionary *AbstractStream::codecOptions() const
{
    return this->d->m_codecOptions;
}

AkCaps AbstractStream::caps() const
{
    return {};
}

bool AbstractStream::sync() const
{
    return this->d->m_sync;
}

qint64 AbstractStream::queueSize() const
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

void AbstractStream::packetEnqueue(AVPacket *packet)
{
    if (!this->d->m_runPacketLoop)
        return;

    this->d->m_packetMutex.lock();

    if (packet) {
        this->d->m_packets.enqueue(PacketPtr(packet,
                                             AbstractStreamPrivate::deletePacket));
        this->d->m_packetQueueSize += packet->size;
    } else {
        this->d->m_packets.enqueue(PacketPtr());
    }

    this->d->m_packetQueueNotEmpty.wakeAll();
    this->d->m_packetMutex.unlock();
}

void AbstractStream::dataEnqueue(AVFrame *frame)
{
    this->d->m_dataMutex.lock();

    if (this->d->m_frames.size() >= this->m_maxData)
        this->d->m_dataQueueNotFull.wait(&this->d->m_dataMutex);

    if (frame)
        this->d->m_frames.enqueue(FramePtr(frame,
                                           AbstractStreamPrivate::deleteFrame));
    else
        this->d->m_frames.enqueue(FramePtr());

    this->d->m_dataQueueNotEmpty.wakeAll();
    this->d->m_dataMutex.unlock();
}

void AbstractStream::subtitleEnqueue(AVSubtitle *subtitle)
{
    this->d->m_dataMutex.lock();

    if (this->d->m_subtitles.size() >= this->m_maxData)
        this->d->m_dataQueueNotFull.wait(&this->d->m_dataMutex);

    if (subtitle)
        this->d->m_subtitles.enqueue(SubtitlePtr(subtitle,
                                                 AbstractStreamPrivate::deleteSubtitle));
    else
        this->d->m_subtitles.enqueue(SubtitlePtr());

    this->d->m_dataQueueNotEmpty.wakeAll();
    this->d->m_dataMutex.unlock();
}

bool AbstractStream::decodeData()
{
    return false;
}

AVMediaType AbstractStream::type(const AVFormatContext *formatContext,
                                 uint index)
{
    return index < formatContext->nb_streams?
                formatContext->streams[index]->codecpar->codec_type:
                AVMEDIA_TYPE_UNKNOWN;
}

AkElement::ElementState AbstractStream::state() const
{
    return this->d->m_state;
}

void AbstractStream::processPacket(AVPacket *packet)
{
    Q_UNUSED(packet)
}

void AbstractStream::processData(AVFrame *frame)
{
    Q_UNUSED(frame)
}

void AbstractStream::processData(AVSubtitle *subtitle)
{
    Q_UNUSED(subtitle)
}

void AbstractStream::flush()
{
    this->d->m_dataMutex.lock();
    this->d->m_packets.clear();
    this->d->m_dataMutex.unlock();

    this->d->m_dataMutex.lock();
    this->d->m_frames.clear();
    this->d->m_subtitles.clear();
    this->d->m_dataMutex.unlock();
}

bool AbstractStream::setState(AkElement::ElementState state)
{
    switch (this->d->m_state) {
    case AkElement::ElementStateNull: {
        if (state == AkElement::ElementStatePaused
            || state == AkElement::ElementStatePlaying) {
            if (!this->d->m_codecContext || !this->d->m_codec)
                return false;

            if (avcodec_open2(this->d->m_codecContext,
                              this->d->m_codec,
                              &this->d->m_codecOptions) < 0)
                return false;

            this->m_clockDiff = 0.0;
            this->d->m_run = true;
            this->d->m_runPacketLoop = true;
            this->d->m_paused = state == AkElement::ElementStatePaused;
            this->d->m_dataLoopResult =
                    QtConcurrent::run(&this->d->m_threadPool,
                                      this->d,
                                      &AbstractStreamPrivate::dataLoop);
            this->d->m_packetLoopResult =
                    QtConcurrent::run(&this->d->m_threadPool,
                                      this->d,
                                      &AbstractStreamPrivate::packetLoop);
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull: {
            this->d->m_runPacketLoop = false;
            waitLoop(this->d->m_packetLoopResult);

            this->d->m_run = false;
            waitLoop(this->d->m_dataLoopResult);

            if (this->d->m_codecOptions)
                av_dict_free(&this->d->m_codecOptions);

            if (this->d->m_codecContext) {
                avcodec_close(this->d->m_codecContext);
                this->d->m_codecContext = nullptr;
            }

            this->d->m_packets.clear();
            this->d->m_frames.clear();
            this->d->m_subtitles.clear();
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        }
        case AkElement::ElementStatePlaying: {
            this->d->m_paused = false;
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        }
        default:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull: {
            this->d->m_runPacketLoop = false;
            waitLoop(this->d->m_packetLoopResult);

            this->d->m_run = false;
            waitLoop(this->d->m_dataLoopResult);

            if (this->d->m_codecOptions)
                av_dict_free(&this->d->m_codecOptions);

            if (this->d->m_codecContext) {
                avcodec_close(this->d->m_codecContext);
                this->d->m_codecContext = nullptr;
            }

            this->d->m_packets.clear();
            this->d->m_frames.clear();
            this->d->m_subtitles.clear();
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        }
        case AkElement::ElementStatePaused: {
            this->d->m_paused = true;
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        }
        default:
            break;
        }

        break;
    }
    }

    return false;
}

void AbstractStream::setSync(bool sync)
{
    this->d->m_sync = sync;
}

AbstractStreamPrivate::AbstractStreamPrivate(AbstractStream *self):
    self(self)
{
}

void AbstractStreamPrivate::packetLoop()
{
    while (this->m_runPacketLoop) {
        if (this->m_paused) {
            QThread::msleep(500);

            continue;
        }

        this->readPacket();
    }
}

void AbstractStreamPrivate::readPacket()
{
    this->m_packetMutex.lock();
    bool gotPacket = true;

    if (this->m_packets.isEmpty())
        gotPacket = this->m_packetQueueNotEmpty.wait(&this->m_packetMutex,
                                                     THREAD_WAIT_LIMIT);

    PacketPtr packet;

    if (gotPacket && !this->m_packets.isEmpty()) {
        packet = this->m_packets.dequeue();

        if (packet)
            this->m_packetQueueSize -= packet->size;
    }

    this->m_packetMutex.unlock();

    if (gotPacket) {
        self->processPacket(packet.data());
        emit self->notify();
    }

    self->decodeData();

    if (!packet)
        this->m_runPacketLoop = false;
}

void AbstractStreamPrivate::dataLoop()
{
    while (this->m_run) {
        if (this->m_paused) {
            QThread::msleep(500);

            continue;
        }

        this->readData();
    }
}

void AbstractStreamPrivate::readData()
{
    switch (self->mediaType()) {
    case AVMEDIA_TYPE_VIDEO:
    case AVMEDIA_TYPE_AUDIO: {
        this->m_dataMutex.lock();
        bool gotFrame = true;

        if (this->m_frames.isEmpty())
            gotFrame = this->m_dataQueueNotEmpty.wait(&this->m_dataMutex,
                                                      THREAD_WAIT_LIMIT);

        FramePtr frame;

        if (gotFrame) {
            frame = this->m_frames.dequeue();

            if (this->m_frames.size() < self->m_maxData)
                this->m_dataQueueNotFull.wakeAll();
        }

        this->m_dataMutex.unlock();

        if (gotFrame) {
            if (frame) {
                self->processData(frame.data());
            } else {
                emit self->eof();
                this->m_run = false;
            }
        }

        break;
    }
    case AVMEDIA_TYPE_SUBTITLE: {
        this->m_dataMutex.lock();
        bool gotSubtitle = true;

        if (this->m_subtitles.isEmpty())
            gotSubtitle = this->m_dataQueueNotEmpty.wait(&this->m_dataMutex,
                                                         THREAD_WAIT_LIMIT);

        SubtitlePtr subtitle;

        if (gotSubtitle) {
            subtitle = this->m_subtitles.dequeue();

            if (this->m_subtitles.size() < self->m_maxData)
                this->m_dataQueueNotFull.wakeAll();
        }

        this->m_dataMutex.unlock();

        if (gotSubtitle) {
            if (subtitle)
                self->processData(subtitle.data());
            else {
                emit self->eof();
                this->m_run = false;
            }
        }

        break;
    }
    default:
        break;
    }
}

void AbstractStreamPrivate::deletePacket(AVPacket *packet)
{
    if (!packet)
        return;

    av_packet_unref(packet);
    av_packet_free(&packet);
}

void AbstractStreamPrivate::deleteFrame(AVFrame *frame)
{
    if (!frame)
        return;

    av_freep(&frame->data[0]);
    frame->data[0] = nullptr;
    av_frame_unref(frame);
    av_frame_free(&frame);
}

void AbstractStreamPrivate::deleteSubtitle(AVSubtitle *subtitle)
{
    if (!subtitle)
        return;

    avsubtitle_free(subtitle);
    delete subtitle;
}

#include "moc_abstractstream.cpp"
