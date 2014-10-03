/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "abstractstream.h"

AbstractStream::AbstractStream(const AVFormatContext *formatContext,
                               uint index, qint64 id, bool noModify,
                               QObject *parent): QObject(parent)
{
    this->m_isValid = false;
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
    this->m_outputThread = NULL;

    if (!this->m_codec)
        return;

    if (this->m_stream)
        this->m_timeBase = QbFrac(this->m_stream->time_base.num,
                                  this->m_stream->time_base.den);

    if (!noModify) {
        this->m_stream->discard = AVDISCARD_DEFAULT;
        this->m_codecContext->workaround_bugs = 1;
        this->m_codecContext->idct_algo = FF_IDCT_AUTO;
        this->m_codecContext->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;

        if (this->m_codec->capabilities & CODEC_CAP_DR1)
            this->m_codecContext->flags |= CODEC_FLAG_EMU_EDGE;
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

QbFrac AbstractStream::timeBase() const
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

QbCaps AbstractStream::caps() const
{
    return QbCaps();
}

void AbstractStream::enqueue(AVPacket *packet)
{
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

    this->m_outputThread = new Thread();

    QObject::connect(this->m_outputThread,
                     SIGNAL(runTh()),
                     this,
                     SLOT(pullFrame()),
                     Qt::DirectConnection);

    this->m_run = true;
    this->m_outputThread->start();
}

void AbstractStream::uninit()
{
    this->m_run = false;
    this->m_mutex.lock();
    this->m_queueNotEmpty.wakeAll();
    this->m_mutex.unlock();

    if (this->m_outputThread) {
        this->m_outputThread->wait();
        delete this->m_outputThread;
        this->m_outputThread = NULL;
    }

    if (this->m_codecOptions)
        av_dict_free(&this->m_codecOptions);

    this->close();

    this->m_codecContext = NULL;
}

void AbstractStream::pullFrame()
{
    while (this->m_run) {
        this->m_mutex.lock();

        if (this->m_packets.isEmpty())
            this->m_queueNotEmpty.wait(&this->m_mutex);

        if (!this->m_packets.isEmpty()) {
            AVPacket *packet = this->m_packets.dequeue();
            this->processPacket(packet);
            this->m_queueSize -= packet->size;
            av_free_packet(packet);
            delete packet;
            emit this->notify();
        }

        this->m_mutex.unlock();
    }
}
