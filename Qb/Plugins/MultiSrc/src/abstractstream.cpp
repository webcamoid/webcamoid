/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "abstractstream.h"

AbstractStream::AbstractStream(QObject *parent): QObject(parent)
{
    this->m_isValid = false;
    this->m_index = -1;
    this->m_mediaType = AVMEDIA_TYPE_UNKNOWN;
    this->m_formatContext = NULL;
    this->m_stream = NULL;
    this->m_codecContext = NULL;
    this->m_codec = NULL;
    this->m_codecOptions = NULL;
}

AbstractStream::AbstractStream(AVFormatContext *formatContext, uint index)
{
    this->m_isValid = false;
    this->m_index = index;
    this->m_stream = formatContext->streams[index];
    this->m_mediaType = this->m_stream->codec->codec_type;
    this->m_formatContext = formatContext;
    this->m_codecContext = this->m_stream->codec;
    this->m_codec = avcodec_find_decoder(this->m_codecContext->codec_id);
    this->m_codecOptions = NULL;

    this->m_timeBase = QbFrac(this->m_stream->time_base.num,
                              this->m_stream->time_base.den);

    if (!this->m_codec)
        return;

    if (this->m_codec->capabilities & CODEC_CAP_TRUNCATED)
        this->m_codecContext->flags |= CODEC_FLAG_TRUNCATED;

    if (avcodec_open2(this->m_codecContext, this->m_codec, &this->m_codecOptions) < 0)
        return;

    this->m_isValid = true;
}

AbstractStream::~AbstractStream()
{
    this->cleanUp();
}

bool AbstractStream::isValid() const
{
    return this->m_isValid;
}

uint AbstractStream::index() const
{
    return this->m_index;
}

QbFrac AbstractStream::timeBase() const
{
    return this->m_timeBase;
}

AVMediaType AbstractStream::mediaType() const
{
    return this->m_mediaType;
}

AVFormatContext *AbstractStream::formatContext() const
{
    return this->m_formatContext;
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

QbPacket AbstractStream::readPacket(AVPacket *packet)
{
    Q_UNUSED(packet)

    return QbPacket();
}

AVMediaType AbstractStream::type(AVFormatContext *formatContext, uint index)
{
    return formatContext->streams[index]->codec->codec_type;
}

void AbstractStream::cleanUp()
{
    if (this->m_codecOptions)
        av_dict_free(&this->m_codecOptions);

    if (this->m_codecContext)
        avcodec_close(this->m_codecContext);
}
