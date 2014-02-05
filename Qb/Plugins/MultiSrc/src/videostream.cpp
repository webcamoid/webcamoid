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

#include "videostream.h"

VideoStream::VideoStream(QObject *parent): AbstractStream(parent)
{
}

VideoStream::VideoStream(AVFormatContext *formatContext, uint index):
    AbstractStream(formatContext, index)
{
}

QbCaps VideoStream::caps() const
{
    const char *format = av_get_pix_fmt_name(this->codecContext()->pix_fmt);
    QbFrac fps = this->fps();

    double maxFrameDuration = (this->formatContext()->iformat->flags &
                               AVFMT_TS_DISCONT)? 10.0: 3600.0;

    QbCaps caps(QString("video/x-raw,"
                        "format=%1,"
                        "width=%2,"
                        "height=%3,"
                        "fps=%4/%5,"
                        "maxFrameDuration=%6").arg(format)
                                              .arg(this->codecContext()->width)
                                              .arg(this->codecContext()->height)
                                              .arg(fps.num())
                                              .arg(fps.den())
                                              .arg(maxFrameDuration));

    return caps;
}

QList<QbPacket> VideoStream::readPackets(AVPacket *packet)
{
    QList<QbPacket> packets;

    if (!this->isValid())
        return packets;

    AVFrame iFrame;
    avcodec_get_frame_defaults(&iFrame);

    int gotFrame;

    avcodec_decode_video2(this->codecContext(),
                          &iFrame,
                          &gotFrame,
                          packet);

    if (!gotFrame)
        return packets;

    int frameSize = avpicture_get_size(this->codecContext()->pix_fmt,
                                       this->codecContext()->width,
                                       this->codecContext()->height);

    QbBufferPtr oBuffer(new uchar[frameSize]);

    if (!oBuffer)
        return packets;

    int64_t pts = av_frame_get_best_effort_timestamp(&iFrame);
    int64_t duration = this->fps().invert().value()
                       * this->timeBase().invert().value();

    avpicture_layout((AVPicture *) &iFrame,
                     this->codecContext()->pix_fmt,
                     this->codecContext()->width,
                     this->codecContext()->height,
                     (uint8_t *) oBuffer.data(),
                     frameSize);

    QbPacket oPacket(this->caps(),
                     oBuffer,
                     frameSize);

    oPacket.setPts(pts);
    oPacket.setDuration(duration);
    oPacket.setTimeBase(this->timeBase());
    oPacket.setIndex(this->index());

    packets << oPacket;

    return packets;
}

QbFrac VideoStream::fps() const
{
    QbFrac fps;

    if (this->stream()->avg_frame_rate.num &&
        this->stream()->avg_frame_rate.den)
        fps = QbFrac(this->stream()->avg_frame_rate.num, this->stream()->avg_frame_rate.den);
    else
        fps = QbFrac(this->stream()->r_frame_rate.num, this->stream()->r_frame_rate.den);

    return fps;
}
