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

#include "audiostream.h"

AudioStream::AudioStream(QObject *parent): AbstractStream(parent)
{
    this->m_oBuffer = NULL;
    this->m_fst = true;
}

AudioStream::AudioStream(AVFormatContext *formatContext, uint index):
    AbstractStream(formatContext, index)
{
    this->m_fst = true;

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

    this->m_isValid = true;
}

AudioStream::~AudioStream()
{
    av_free(this->m_oBuffer);
    this->cleanUp();
}

QbCaps AudioStream::caps() const
{
    const char *format = av_get_sample_fmt_name(this->codecContext()->sample_fmt);
    char layout[256];

    av_get_channel_layout_string(layout,
                                 sizeof(layout),
                                 this->codecContext()->channels,
                                 this->codecContext()->channel_layout);

    QbCaps caps(QString("audio/x-raw,"
                        "format=%1,"
                        "channels=%2,"
                        "rate=%3,"
                        "layout=%4").arg(format)
                                    .arg(this->codecContext()->channels)
                                    .arg(this->codecContext()->sample_rate)
                                    .arg(layout));

    return caps;
}

QbPacket AudioStream::readPacket(AVPacket *packet)
{
    if (!this->isValid())
        return QbPacket();

    AVFrame iFrame;
    avcodec_get_frame_defaults(&iFrame);

    int gotFrame;

    avcodec_decode_audio4(this->codecContext(),
                          &iFrame,
                          &gotFrame,
                          packet);

    if (!gotFrame)
        return QbPacket();

    if (this->m_fst)
    {
        this->m_pts = 0;
        this->m_duration = av_frame_get_pkt_duration(&iFrame);
        this->m_fst = false;
    }
    else
        this->m_pts += this->m_duration;

    int oBufferLineSize;

    int ret = av_samples_alloc(this->m_oBuffer,
                               &oBufferLineSize,
                               iFrame.channels,
                               iFrame.nb_samples,
                               (AVSampleFormat) iFrame.format,
                               1);

    if (ret < 0)
        return QbPacket();

    int oBufferSize = av_samples_get_buffer_size(NULL,
                                                 iFrame.channels,
                                                 iFrame.nb_samples,
                                                 (AVSampleFormat) iFrame.format,
                                                 1);

    av_samples_copy(this->m_oBuffer,
                    iFrame.data,
                    0,
                    0,
                    iFrame.nb_samples,
                    iFrame.channels,
                    (AVSampleFormat) iFrame.format);

    QSharedPointer<uchar> oBuffer(new uchar[oBufferSize]);
    memcpy(oBuffer.data(), this->m_oBuffer[0], oBufferSize);
    av_freep(&this->m_oBuffer[0]);

    QbCaps caps = this->caps();
    caps.setProperty("samples", iFrame.nb_samples);

    QbPacket oPacket(caps,
                     oBuffer,
                     oBufferSize);

    oPacket.setPts(this->m_pts);
    oPacket.setDuration(this->m_duration);
    oPacket.setTimeBase(this->timeBase());
    oPacket.setIndex(this->index());

    return oPacket;
}

void AudioStream::cleanUp()
{
    AbstractStream::cleanUp();
}
