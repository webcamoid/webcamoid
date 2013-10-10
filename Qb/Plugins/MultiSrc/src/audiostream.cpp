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
}

AudioStream::AudioStream(AVFormatContext *formatContext, uint index):
    AbstractStream(formatContext, index)
{
    this->m_fst = true;
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

    double maxFrameDuration = (this->formatContext()->iformat->flags &
                               AVFMT_TS_DISCONT)? 10.0: 3600.0;

    int bytesPerSample = av_get_bytes_per_sample(this->codecContext()->sample_fmt);

    QbCaps caps(QString("audio/x-raw,"
                        "format=%1,"
                        "bps=%2,"
                        "channels=%3,"
                        "rate=%4,"
                        "layout=%5,"
                        "maxFrameDuration=%6").arg(format)
                                              .arg(bytesPerSample)
                                              .arg(this->codecContext()->channels)
                                              .arg(this->codecContext()->sample_rate)
                                              .arg(layout)
                                              .arg(maxFrameDuration));

    return caps;
}

QList<QbPacket> AudioStream::readPackets(AVPacket *packet)
{
    QList<QbPacket> packets;

    if (!this->isValid())
        return packets;

    AVFrame iFrame;
    avcodec_get_frame_defaults(&iFrame);

    int gotFrame;

    avcodec_decode_audio4(this->codecContext(),
                          &iFrame,
                          &gotFrame,
                          packet);

    if (!gotFrame)
        return packets;

    if (this->m_fst)
    {
        this->m_pts = 0;
        this->m_duration = av_frame_get_pkt_duration(&iFrame);
        this->m_fst = false;
    }
    else
        this->m_pts += this->m_duration;

    int oLineSize;

    int oBufferSize = av_samples_get_buffer_size(&oLineSize,
                                                 iFrame.channels,
                                                 iFrame.nb_samples,
                                                 (AVSampleFormat) iFrame.format,
                                                 0);

    QbBufferPtr oBuffer(new uchar[oBufferSize]);

    int planes = av_sample_fmt_is_planar((AVSampleFormat) iFrame.format)? iFrame.channels: 1;
    QVector<uint8_t *> oData(planes);

    if (av_samples_fill_arrays(&oData.data()[0],
                               &oLineSize,
                               (const uint8_t *) oBuffer.data(),
                               iFrame.channels,
                               iFrame.nb_samples,
                               (AVSampleFormat) iFrame.format,
                               0) < 0)
        return packets;

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

    oPacket.setPts(this->m_pts);
    oPacket.setDuration(this->m_duration);
    oPacket.setTimeBase(this->timeBase());
    oPacket.setIndex(this->index());

    packets << oPacket;

    return packets;
}
