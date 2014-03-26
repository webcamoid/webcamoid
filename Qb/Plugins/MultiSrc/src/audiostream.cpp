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
    this->m_fst = true;
    this->resetAlign();
}

AudioStream::AudioStream(const AVFormatContext *formatContext, uint index):
    AbstractStream(formatContext, index)
{
    this->m_fst = true;
    this->resetAlign();
}

bool AudioStream::align() const
{
    return this->m_align;
}

QbCaps AudioStream::caps() const
{
    const char *format = av_get_sample_fmt_name(this->codecContext()->sample_fmt);
    char layout[256];
    int64_t channelLayout;

    if (this->codecContext()->channel_layout)
        channelLayout = this->codecContext()->channel_layout;
    else
        channelLayout = av_get_default_channel_layout(this->codecContext()->channels);

    av_get_channel_layout_string(layout,
                                 sizeof(layout),
                                 this->codecContext()->channels,
                                 channelLayout);

    int bytesPerSample = av_get_bytes_per_sample(this->codecContext()->sample_fmt);

    QbCaps caps(QString("audio/x-raw,"
                        "format=%1,"
                        "bps=%2,"
                        "channels=%3,"
                        "rate=%4,"
                        "layout=%5,"
                        "align=%6").arg(format)
                                   .arg(bytesPerSample)
                                   .arg(this->codecContext()->channels)
                                   .arg(this->codecContext()->sample_rate)
                                   .arg(layout)
                                   .arg(this->align()));

    return caps;
}

void AudioStream::processPacket(const PacketPtr &packet)
{
    if (!this->isValid())
        return;

    AVFrame iFrame;
    avcodec_get_frame_defaults(&iFrame);

    int gotFrame;

    avcodec_decode_audio4(this->codecContext(),
                          &iFrame,
                          &gotFrame,
                          packet.data());

    if (!gotFrame)
        return;

    if (this->m_fst) {
        this->m_pts = 0;
        this->m_duration = av_frame_get_pkt_duration(&iFrame);
        this->m_fst = false;
    }
    else
        this->m_pts += this->m_duration;

    int64_t pts = (iFrame.pts != AV_NOPTS_VALUE) ? iFrame.pts :
                  (iFrame.pkt_pts != AV_NOPTS_VALUE) ? iFrame.pkt_pts :
                  this->m_pts;

    int oLineSize;

    int oBufferSize = av_samples_get_buffer_size(&oLineSize,
                      iFrame.channels,
                      iFrame.nb_samples,
                      (AVSampleFormat) iFrame.format,
                      this->align() ? 0 : 1);

    QbBufferPtr oBuffer(new uchar[oBufferSize]);

    int planes = av_sample_fmt_is_planar((AVSampleFormat) iFrame.format) ?
                 iFrame.channels : 1;
    QVector<uint8_t *> oData(planes);

    if (av_samples_fill_arrays(&oData.data()[0],
                               &oLineSize,
                               (const uint8_t *) oBuffer.data(),
                               iFrame.channels,
                               iFrame.nb_samples,
                               (AVSampleFormat) iFrame.format,
                               this->align() ? 0 : 1) < 0)
        return;

    av_samples_copy(&oData.data()[0],
                    iFrame.data,
                    0,
                    0,
                    iFrame.nb_samples,
                    iFrame.channels,
                    (AVSampleFormat) iFrame.format);

    QbCaps caps = this->caps();
    caps.setProperty("samples", iFrame.nb_samples);

    QbPacket oPacket(caps,
                     oBuffer,
                     oBufferSize);

    oPacket.setPts(pts);
    oPacket.setDuration(this->m_duration);
    oPacket.setTimeBase(this->timeBase());
    oPacket.setIndex(this->index());

    emit this->oStream(oPacket);
}

void AudioStream::setAlign(bool align)
{
    this->m_align = align;
}

void AudioStream::resetAlign()
{
    this->setAlign(false);
}
