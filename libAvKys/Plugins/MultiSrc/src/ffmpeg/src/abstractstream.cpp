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

typedef QSharedPointer<AVPacket> PacketPtr;
typedef QSharedPointer<AVFrame> FramePtr;
typedef QSharedPointer<AVSubtitle> SubtitlePtr;

class AbstractStreamPrivate
{
    public:
        AbstractStream *self;
        uint m_index;
        qint64 m_id;
        AkFrac m_timeBase;
        AVMediaType m_mediaType;
        AVStream *m_stream;
        AVCodecContext *m_codecContext;
        AVCodec *m_codec;
        AVDictionary *m_codecOptions;
        QThreadPool m_threadPool;
        QMutex m_packetMutex;
        QMutex m_dataMutex;
        QWaitCondition m_packetQueueNotEmpty;
        QWaitCondition m_dataQueueNotEmpty;
        QWaitCondition m_dataQueueNotFull;
        QQueue<PacketPtr> m_packets;
        QQueue<FramePtr> m_frames;
        QQueue<SubtitlePtr> m_subtitles;
        qint64 m_packetQueueSize;
        Clock *m_globalClock;
        bool m_runPacketLoop;
        bool m_runDataLoop;
        QFuture<void> m_packetLoopResult;
        QFuture<void> m_dataLoopResult;

        AbstractStreamPrivate(AbstractStream *self):
            self(self),
            m_index(0),
            m_id(-1),
            m_mediaType(AVMEDIA_TYPE_UNKNOWN),
            m_stream(nullptr),
            m_codecContext(nullptr),
            m_codec(nullptr),
            m_codecOptions(nullptr),
            m_packetQueueSize(-1),
            m_globalClock(nullptr),
            m_runPacketLoop(false),
            m_runDataLoop(false)
        {
        }

        inline void packetLoop();
        inline void dataLoop();
        inline static void deletePacket(AVPacket *packet);
        inline static void deleteFrame(AVFrame *frame);
        inline static void deleteSubtitle(AVSubtitle *subtitle);
};

AbstractStream::AbstractStream(const AVFormatContext *formatContext,
                               uint index, qint64 id, Clock *globalClock,
                               bool noModify,
                               QObject *parent): QObject(parent)
{
    this->d = new AbstractStreamPrivate(this);
    this->m_paused = false;
    this->m_isValid = false;
    this->m_clockDiff = 0;
    this->m_maxData = 0;
    this->d->m_index = index;
    this->d->m_id = id;

    this->d->m_stream = (formatContext && index < formatContext->nb_streams)?
                         formatContext->streams[index]: nullptr;

    this->d->m_mediaType = this->d->m_stream?
#ifdef HAVE_CODECPAR
                            this->d->m_stream->codecpar->codec_type:
#else
                            this->d->m_stream->codec->codec_type:
#endif
                            AVMEDIA_TYPE_UNKNOWN;

    this->d->m_codecContext = nullptr;

    if (this->d->m_stream) {
        this->d->m_codecContext = avcodec_alloc_context3(nullptr);

#ifdef HAVE_CODECPAR
        if (avcodec_parameters_to_context(this->d->m_codecContext,
                                          this->d->m_stream->codecpar) < 0)
            avcodec_free_context(&this->d->m_codecContext);
#else
        if (avcodec_copy_context(this->d->m_codecContext,
                                 this->d->m_stream->codec) < 0) {
            avcodec_close(this->d->m_codecContext);
            av_free(this->d->m_codecContext);
        }
#endif
    }

    this->d->m_codec =
            this->d->m_codecContext?
                avcodec_find_decoder(this->d->m_codecContext->codec_id):
                nullptr;

    this->d->m_codecOptions = nullptr;
    this->d->m_packetQueueSize = 0;
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
    if (this->d->m_codecContext)
        avcodec_free_context(&this->d->m_codecContext);

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

AVCodec *AbstractStream::codec() const
{
    return this->d->m_codec;
}

AVDictionary *AbstractStream::codecOptions() const
{
    return this->d->m_codecOptions;
}

AkCaps AbstractStream::caps() const
{
    return AkCaps();
}

void AbstractStream::packetEnqueue(AVPacket *packet)
{
    if (!this->d->m_runPacketLoop)
        return;

    this->d->m_packetMutex.lock();

    if (packet) {
        this->d->m_packets.enqueue(PacketPtr(packet, this->d->deletePacket));
        this->d->m_packetQueueSize += packet->size;
    } else
        this->d->m_packets.enqueue(PacketPtr());

    this->d->m_packetQueueNotEmpty.wakeAll();
    this->d->m_packetMutex.unlock();
}

void AbstractStream::dataEnqueue(AVFrame *frame)
{
    this->d->m_dataMutex.lock();

    if (this->d->m_frames.size() >= this->m_maxData)
        this->d->m_dataQueueNotFull.wait(&this->d->m_dataMutex);

    if (frame)
        this->d->m_frames.enqueue(FramePtr(frame, this->d->deleteFrame));
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
                                                 this->d->deleteSubtitle));
    else
        this->d->m_subtitles.enqueue(SubtitlePtr());

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

AVMediaType AbstractStream::type(const AVFormatContext *formatContext,
                                 uint index)
{
    return index < formatContext->nb_streams?
#ifdef HAVE_CODECPAR
                formatContext->streams[index]->codecpar->codec_type:
#else
                formatContext->streams[index]->codec->codec_type:
#endif
                AVMEDIA_TYPE_UNKNOWN;
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

void AbstractStreamPrivate::packetLoop()
{
    while (this->m_runPacketLoop) {
        this->m_packetMutex.lock();
        bool gotPacket = true;

        if (this->m_packets.isEmpty())
            gotPacket = this->m_packetQueueNotEmpty.wait(&this->m_packetMutex,
                                                         THREAD_WAIT_LIMIT);

        PacketPtr packet;

        if (gotPacket) {
            packet = this->m_packets.dequeue();

            if (packet)
                this->m_packetQueueSize -= packet->size;
        }

        this->m_packetMutex.unlock();

        if (gotPacket) {
            self->processPacket(packet.data());
            emit self->notify();
        }

        if (!packet)
            this->m_runPacketLoop = false;
    }
}

void AbstractStreamPrivate::dataLoop()
{
    switch (self->mediaType()) {
    case AVMEDIA_TYPE_VIDEO:
    case AVMEDIA_TYPE_AUDIO:
        while (this->m_runDataLoop) {
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
                if (frame)
                    self->processData(frame.data());
                else {
                    emit self->eof();
                    this->m_runDataLoop = false;
                }
            }
        }

        break;
    case AVMEDIA_TYPE_SUBTITLE:
        while (this->m_runDataLoop) {
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
                    this->m_runDataLoop = false;
                }
            }
        }

        break;
    default:
        break;
    }
}

void AbstractStreamPrivate::deletePacket(AVPacket *packet)
{
    av_packet_unref(packet);
    delete packet;
}

void AbstractStreamPrivate::deleteFrame(AVFrame *frame)
{
    av_freep(&frame->data[0]);
    frame->data[0] = nullptr;
    av_frame_unref(frame);
    av_frame_free(&frame);
}

void AbstractStreamPrivate::deleteSubtitle(AVSubtitle *subtitle)
{
    avsubtitle_free(subtitle);
    delete subtitle;
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

bool AbstractStream::init()
{
    if (!this->d->m_codecContext
        || !this->d->m_codec)
        return false;

    if (avcodec_open2(this->d->m_codecContext,
                      this->d->m_codec,
                      &this->d->m_codecOptions) < 0)
        return false;

    this->m_clockDiff = 0;
    this->d->m_runPacketLoop = true;
    this->d->m_runDataLoop = true;
    this->d->m_packetLoopResult =
            QtConcurrent::run(&this->d->m_threadPool,
                              this->d,
                              &AbstractStreamPrivate::packetLoop);
    this->d->m_dataLoopResult =
            QtConcurrent::run(&this->d->m_threadPool,
                              this->d,
                              &AbstractStreamPrivate::dataLoop);

    return true;
}

void AbstractStream::uninit()
{
    this->d->m_runPacketLoop = false;
    waitLoop(this->d->m_packetLoopResult);

    this->d->m_runDataLoop = false;
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
}

#include "moc_abstractstream.cpp"
