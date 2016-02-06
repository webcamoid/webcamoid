/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "abstractstream.h"

AbstractStream::AbstractStream(const AVFormatContext *formatContext,
                               uint index, qint64 id, Clock *globalClock,
                               bool noModify,
                               QObject *parent): QObject(parent)
{
    this->m_runPacketLoop = false;
    this->m_runDataLoop = false;
    this->m_paused = false;
    this->m_isValid = false;
    this->m_clockDiff = 0;
    this->m_maxData = 0;
    this->m_index = index;
    this->m_id = id;

    this->m_stream = (formatContext && index < formatContext->nb_streams)?
                         formatContext->streams[index]: NULL;

    this->m_mediaType = this->m_stream?
                            this->m_stream->codec->codec_type:
                            AVMEDIA_TYPE_UNKNOWN;

    this->m_codecContext = this->m_stream? this->m_stream->codec: NULL;

    this->m_codec = this->m_codecContext?
                        avcodec_find_decoder(this->m_codecContext->codec_id):
                        NULL;

    this->m_codecOptions = NULL;
    this->m_packetQueueSize = 0;
    this->m_globalClock = globalClock;

    if (!this->m_codec)
        return;

    if (this->m_stream)
        this->m_timeBase = AkFrac(this->m_stream->time_base.num,
                                  this->m_stream->time_base.den);

    if (!noModify) {
        this->m_stream->discard = AVDISCARD_DEFAULT;
        this->m_codecContext->workaround_bugs = 1;
        this->m_codecContext->idct_algo = FF_IDCT_AUTO;
        this->m_codecContext->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;

        if (this->m_codec->capabilities & CODEC_CAP_DR1)
            this->m_codecContext->flags |= CODEC_FLAG_EMU_EDGE;

        av_dict_set_int(&this->m_codecOptions, "refcounted_frames", 1, 0);
    }

    this->m_isValid = true;
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
    return this->m_index;
}

qint64 AbstractStream::id() const
{
    return this->m_id;
}

AkFrac AbstractStream::timeBase() const
{
    return this->m_timeBase;
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

AVCodec *AbstractStream::codec() const
{
    return this->m_codec;
}

AVDictionary *AbstractStream::codecOptions() const
{
    return this->m_codecOptions;
}

AkCaps AbstractStream::caps() const
{
    return AkCaps();
}

void AbstractStream::packetEnqueue(AVPacket *packet)
{
    this->m_packetMutex.lock();
    this->m_packets.enqueue(PacketPtr(packet, this->deletePacket));
    this->m_packetQueueSize += packet->size;
    this->m_packetQueueNotEmpty.wakeAll();
    this->m_packetMutex.unlock();
}

void AbstractStream::dataEnqueue(AVFrame *frame)
{
    this->m_dataMutex.lock();

    if (this->m_frames.size() >= this->m_maxData)
        this->m_dataQueueNotFull.wait(&this->m_dataMutex);

    this->m_frames.enqueue(FramePtr(frame, this->deleteFrame));
    this->m_dataQueueNotEmpty.wakeAll();
    this->m_dataMutex.unlock();
}

void AbstractStream::dataEnqueue(AVSubtitle *subtitle)
{
    this->m_dataMutex.lock();

    if (this->m_subtitles.size() >= this->m_maxData)
        this->m_dataQueueNotFull.wait(&this->m_dataMutex);

    this->m_subtitles.enqueue(SubtitlePtr(subtitle, this->deleteSubtitle));
    this->m_dataQueueNotEmpty.wakeAll();
    this->m_dataMutex.unlock();
}

qint64 AbstractStream::queueSize()
{
    return this->m_packetQueueSize;
}

Clock *AbstractStream::globalClock()
{
    return this->m_globalClock;
}

qreal AbstractStream::clockDiff() const
{
    return this->m_clockDiff;
}

AVMediaType AbstractStream::type(const AVFormatContext *formatContext,
                                 uint index)
{
    return index < formatContext->nb_streams?
                formatContext->streams[index]->codec->codec_type:
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

void AbstractStream::packetLoop(AbstractStream *stream)
{
    while (stream->m_runPacketLoop) {
        stream->m_packetMutex.lock();

        if (stream->m_packets.isEmpty())
            stream->m_packetQueueNotEmpty.wait(&stream->m_packetMutex,
                                               THREAD_WAIT_LIMIT);

        if (!stream->m_packets.isEmpty()) {
            PacketPtr packet = stream->m_packets.dequeue();
            stream->processPacket(packet.data());
            stream->m_packetQueueSize -= packet->size;
            emit stream->notify();
        }

        stream->m_packetMutex.unlock();
    }
}

void AbstractStream::dataLoop(AbstractStream *stream)
{
    switch (stream->mediaType()) {
    case AVMEDIA_TYPE_VIDEO:
    case AVMEDIA_TYPE_AUDIO:
        while (stream->m_runDataLoop) {
            stream->m_dataMutex.lock();

            if (stream->m_frames.isEmpty())
                stream->m_dataQueueNotEmpty.wait(&stream->m_dataMutex,
                                                 THREAD_WAIT_LIMIT);

            if (!stream->m_frames.isEmpty()) {
                FramePtr frame = stream->m_frames.dequeue();
                stream->processData(frame.data());

                if (stream->m_frames.size() < stream->m_maxData)
                    stream->m_dataQueueNotFull.wakeAll();
            }

            stream->m_dataMutex.unlock();
        }

        break;
    case AVMEDIA_TYPE_SUBTITLE:
        while (stream->m_runDataLoop) {
            stream->m_dataMutex.lock();

            if (stream->m_subtitles.isEmpty())
                stream->m_dataQueueNotEmpty.wait(&stream->m_dataMutex,
                                                 THREAD_WAIT_LIMIT);

            if (!stream->m_subtitles.isEmpty()) {
                SubtitlePtr subtitle = stream->m_subtitles.dequeue();
                stream->processData(subtitle.data());

                if (stream->m_subtitles.size() < stream->m_maxData)
                    stream->m_dataQueueNotFull.wakeAll();
            }

            stream->m_dataMutex.unlock();
        }

        break;
    default:
        break;
    }
}

void AbstractStream::deletePacket(AVPacket *packet)
{
    av_packet_unref(packet);
    delete packet;
}

void AbstractStream::deleteFrame(AVFrame *frame)
{
    av_frame_unref(frame);
    av_frame_free(&frame);
}

void AbstractStream::deleteSubtitle(AVSubtitle *subtitle)
{
    avsubtitle_free(subtitle);
    delete subtitle;
}

void AbstractStream::setPaused(bool paused)
{
    if (this->m_paused == paused)
        return;

    this->m_runDataLoop = !paused;

    if (paused)
        this->m_dataLoopResult.waitForFinished();
    else
        this->m_dataLoopResult = QtConcurrent::run(&this->m_threadPool, this->dataLoop, this);

    this->m_paused = paused;
    emit this->pausedChanged(paused);
}

void AbstractStream::resetPaused()
{
    this->setPaused(false);
}

bool AbstractStream::init()
{
    if (avcodec_open2(this->m_codecContext,
                      this->m_codec,
                      &this->m_codecOptions) < 0)
        return false;

    this->m_clockDiff = 0;
    this->m_runPacketLoop = true;
    this->m_runDataLoop = true;
    this->m_packetLoopResult = QtConcurrent::run(&this->m_threadPool, this->packetLoop, this);
    this->m_dataLoopResult = QtConcurrent::run(&this->m_threadPool, this->dataLoop, this);

    return true;
}

void AbstractStream::uninit()
{
    this->m_runPacketLoop = false;
    this->m_packetLoopResult.waitForFinished();

    this->m_runDataLoop = false;
    this->m_dataLoopResult.waitForFinished();

    if (this->m_codecOptions)
        av_dict_free(&this->m_codecOptions);

    if (this->m_codecContext) {
        avcodec_close(this->m_codecContext);
        this->m_codecContext = NULL;
    }
}
