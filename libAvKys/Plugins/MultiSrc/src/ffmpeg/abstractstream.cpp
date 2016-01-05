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
    this->m_isValid = false;
    this->m_clockDiff = 0;
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
    this->m_queueSize = 0;
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

void AbstractStream::enqueue(AVPacket *packet)
{
    if (!this->m_run)
        return;

    this->m_mutex.lock();
    this->m_packets.enqueue(packet);
    this->m_queueSize += packet->size;
    this->m_queueNotEmpty.wakeAll();
    this->m_mutex.unlock();
}

qint64 AbstractStream::queueSize()
{
    return this->m_queueSize;
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

void AbstractStream::decodeFrame(AbstractStream *stream)
{
    while (stream->m_run) {
        stream->m_mutex.lock();

        if (stream->m_packets.isEmpty())
            stream->m_queueNotEmpty.wait(&stream->m_mutex, THREAD_WAIT_LIMIT);

        if (!stream->m_packets.isEmpty()) {
            AVPacket *packet = stream->m_packets.dequeue();
            stream->processPacket(packet);
            stream->m_queueSize -= packet->size;
            av_free_packet(packet);
            delete packet;
            emit stream->notify();
        }

        stream->m_mutex.unlock();
    }
}

bool AbstractStream::open()
{
    if (!this->m_codecContext)
        return false;

    if (avcodec_open2(this->m_codecContext, this->m_codec,
                      &this->m_codecOptions) < 0)
        return false;

    return true;
}

void AbstractStream::close()
{
    if (this->m_codecContext)
        avcodec_close(this->m_codecContext);
}

void AbstractStream::init()
{
    if (!this->open())
        return;

    this->m_clockDiff = 0;
    this->m_run = true;
    QtConcurrent::run(&this->m_threadPool, this->decodeFrame, this);
}

void AbstractStream::uninit()
{
    this->m_run = false;
    this->m_mutex.lock();
    this->m_queueNotEmpty.wakeAll();
    this->m_mutex.unlock();
    this->m_threadPool.waitForDone();

    if (this->m_codecOptions)
        av_dict_free(&this->m_codecOptions);

    this->close();

    this->m_codecContext = NULL;
}
