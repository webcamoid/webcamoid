/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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
    this->m_iFrame = NULL;
    this->m_orig = NULL;
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
    this->m_iFrame = NULL;
    this->m_orig = NULL;

    this->m_timeBase = QbFrac(this->m_stream->time_base.num,
                              this->m_stream->time_base.den);

    if (!this->m_codec)
        return;

    if (this->m_codec->capabilities & CODEC_CAP_TRUNCATED)
        this->m_codecContext->flags |= CODEC_FLAG_TRUNCATED;

    if (avcodec_open2(this->m_codecContext, this->m_codec, &this->m_codecOptions) < 0)
        return;

    this->m_iFrame = avcodec_alloc_frame();

    if (!this->m_iFrame)
    {
        this->cleanUp();

        return;
    }

    this->m_isValid = true;
}

AbstractStream::AbstractStream(const AbstractStream &other):
    QObject(NULL),
    m_isValid(other.m_isValid),
    m_iFrame(other.m_iFrame),
    m_index(other.m_index),
    m_timeBase(other.m_timeBase),
    m_mediaType(other.m_mediaType),
    m_formatContext(other.m_formatContext),
    m_stream(other.m_stream),
    m_codecContext(other.m_codecContext),
    m_codec(other.m_codec),
    m_codecOptions(other.m_codecOptions)
{
    this->m_orig = (AbstractStream *) &other;
    this->m_orig->m_copy << this;
}

AbstractStream::~AbstractStream()
{
    if (this->m_orig)
    {
        this->m_orig->m_copy.removeAll(this);

        if (!this->m_copy.isEmpty())
        {
            this->m_orig->m_copy << this->m_copy;

            foreach (AbstractStream *copy, this->m_copy)
                copy->m_orig = this->m_orig;
        }
    }
    else if (this->m_copy.isEmpty())
        this->cleanUp();
    else
        foreach (AbstractStream *copy, this->m_copy)
            copy->m_orig = NULL;
}

AbstractStream &AbstractStream::operator =(const AbstractStream &other)
{
    if (this != &other)
    {
        this->m_isValid = other.m_isValid;
        this->m_index = other.m_index;
        this->m_timeBase = other.m_timeBase;
        this->m_mediaType = other.m_mediaType;
        this->m_formatContext = other.m_formatContext;
        this->m_stream = other.m_stream;
        this->m_codecContext = other.m_codecContext;
        this->m_codec = other.m_codec;
        this->m_codecOptions = other.m_codecOptions;
        this->m_iFrame = other.m_iFrame;

        if (this->m_orig)
        {
            this->m_orig->m_copy.removeAll(this);

            if (!this->m_copy.isEmpty())
            {
                this->m_orig->m_copy << this->m_copy;

                foreach (AbstractStream *copy, this->m_copy)
                    copy->m_orig = this->m_orig;
            }
        }
        else if (!this->m_copy.isEmpty())
                foreach (AbstractStream *copy, this->m_copy)
                    copy->m_orig = NULL;

        this->m_orig = (AbstractStream *) &other;
        this->m_orig->m_copy << this;
    }

    return *this;
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

QbCaps AbstractStream::oCaps()
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
{/*
    if (this->m_iFrame)
        av_free(this->m_iFrame);*/

    if (this->m_codecOptions)
        av_dict_free(&this->m_codecOptions);

    if (this->m_codecContext)
        avcodec_close(this->m_codecContext);
}
