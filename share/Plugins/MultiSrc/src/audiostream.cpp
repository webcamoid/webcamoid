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

#include "audiostream.h"

AudioStream::AudioStream(QObject *parent): AbstractStream(parent)
{
    this->m_oBuffer = NULL;
}

AudioStream::AudioStream(AVFormatContext *formatContext, uint index):
    AbstractStream(formatContext, index)
{
    if (!this->isValid())
    {
        this->cleanUp();

        return;
    }

    this->m_isValid = false;

    int nPlanes = av_sample_fmt_is_planar(this->codecContext()->sample_fmt)? this->codecContext()->channels: 1;
    this->m_oBuffer = (uint8_t **) av_mallocz(sizeof(uint8_t *) * nPlanes);

    if (!this->m_oBuffer)
    {
        this->cleanUp();

        return;
    }

    this->m_ffToMime[AV_SAMPLE_FMT_U8] = "U8";
    this->m_ffToMime[AV_SAMPLE_FMT_S16] = "S16LE";
    this->m_ffToMime[AV_SAMPLE_FMT_S32] = "S32LE";
    this->m_ffToMime[AV_SAMPLE_FMT_FLT] = "F32LE";
    this->m_ffToMime[AV_SAMPLE_FMT_DBL] = "F64LE";
    this->m_ffToMime[AV_SAMPLE_FMT_U8P] = "U8";
    this->m_ffToMime[AV_SAMPLE_FMT_S16P] = "S16LE";
    this->m_ffToMime[AV_SAMPLE_FMT_S32P] = "S32LE";
    this->m_ffToMime[AV_SAMPLE_FMT_FLTP] = "F32LE";
    this->m_ffToMime[AV_SAMPLE_FMT_DBLP] = "F64LE";

    this->m_isValid = true;
}

AudioStream::AudioStream(const AudioStream &other):
    AbstractStream(other),
    m_oBuffer(other.m_oBuffer),
    m_ffToMime(other.m_ffToMime)
{
}

AudioStream::~AudioStream()
{
    if (this->m_orig || !this->m_copy.isEmpty())
        return;

    av_free(this->m_oBuffer);
    this->cleanUp();
}

AudioStream &AudioStream::operator =(const AudioStream &other)
{
    if (this != &other)
    {
        this->m_oBuffer = other.m_oBuffer;
        this->m_ffToMime = other.m_ffToMime;

        AbstractStream::operator =(other);
    }

    return *this;
}

QbPacket AudioStream::readPacket(AVPacket *packet)
{
    if (!this->isValid())
        return QbPacket();

    int gotFrame;

    avcodec_decode_audio4(this->codecContext(),
                          this->m_iFrame,
                          &gotFrame,
                          packet);

    if (!gotFrame)
        return QbPacket();

    int oBufferLineSize;

    int ret = av_samples_alloc(this->m_oBuffer,
                               &oBufferLineSize,
                               this->m_iFrame->channels,
                               this->m_iFrame->nb_samples,
                               (AVSampleFormat) this->m_iFrame->format,
                               1);

    if (ret < 0)
        return QbPacket();

    int oBufferSize = av_samples_get_buffer_size(NULL,
                                                 this->m_iFrame->channels,
                                                 this->m_iFrame->nb_samples,
                                                 (AVSampleFormat) this->m_iFrame->format,
                                                 1);

    av_samples_copy(this->m_oBuffer,
                    this->m_iFrame->data,
                    0,
                    0,
                    this->m_iFrame->nb_samples,
                    this->m_iFrame->channels,
                    (AVSampleFormat) this->m_iFrame->format);

    this->m_oFrame = QByteArray((const char *) this->m_oBuffer[0], oBufferSize);
    av_freep(&this->m_oBuffer[0]);

    AVSampleFormat fmt = this->codecContext()->sample_fmt;

    if (!this->m_ffToMime.contains(fmt))
        return QbPacket();

    QbPacket oPacket(QString("audio/x-raw,"
                             "format=%1,"
                             "channels=%2,"
                             "rate=%3").arg(this->m_ffToMime[fmt])
                                       .arg(this->m_iFrame->channels)
                                       .arg(this->m_iFrame->sample_rate),
                     this->m_oFrame.constData(),
                     this->m_oFrame.size());

    oPacket.setDts(packet->dts);
    oPacket.setPts(packet->pts);
    oPacket.setDuration(packet->duration);
    oPacket.setIndex(this->index());

    return oPacket;
}

void AudioStream::cleanUp()
{/*
    if (this->m_oBuffer)
        av_free(this->m_oBuffer);*/

    AbstractStream::cleanUp();
}
