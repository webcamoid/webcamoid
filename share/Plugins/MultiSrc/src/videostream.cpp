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

#include "videostream.h"

VideoStream::VideoStream(QObject *parent): AbstractStream(parent)
{
}

VideoStream::VideoStream(AVFormatContext *formatContext, uint index):
    AbstractStream(formatContext, index)
{
}

VideoStream::VideoStream(const VideoStream &other):
    AbstractStream(other),
    m_oFrame(other.m_oFrame),
    m_oCaps(other.m_oCaps)
{
}

VideoStream &VideoStream::operator =(const VideoStream &other)
{
    if (this != &other)
    {
        this->m_oFrame = other.m_oFrame;
        this->m_oCaps = other.m_oCaps;

        AbstractStream::operator =(other);
    }

    return *this;
}

QbPacket VideoStream::readPacket(AVPacket *packet)
{
    if (!this->isValid())
        return QbPacket();

    int gotFrame;

    avcodec_decode_video2(this->codecContext(),
                          this->m_iFrame,
                          &gotFrame,
                          packet);

    if (!gotFrame)
        return QbPacket();

    const char *format = av_get_pix_fmt_name(this->codecContext()->pix_fmt);

    QbCaps caps(QString("video/x-raw,"
                        "format=%1,"
                        "width=%2,"
                        "height=%3").arg(format)
                                    .arg(this->codecContext()->width)
                                    .arg(this->codecContext()->height));

    if (caps != this->m_oCaps)
    {
        this->m_oFrame.resize(avpicture_get_size(this->codecContext()->pix_fmt,
                                                 this->codecContext()->width,
                                                 this->codecContext()->height));

        this->m_oCaps = caps;
    }

    this->m_iFrame->pts = av_frame_get_best_effort_timestamp(this->m_iFrame);
    this->m_iFrame->pkt_duration = av_frame_get_pkt_duration(this->m_iFrame);

    avpicture_layout((AVPicture *) this->m_iFrame,
                     this->codecContext()->pix_fmt,
                     this->codecContext()->width,
                     this->codecContext()->height,
                     (uint8_t *) this->m_oFrame.data(),
                     this->m_oFrame.size());

    QbPacket oPacket(this->m_oCaps,
                     this->m_oFrame.constData(),
                     this->m_oFrame.size());

    oPacket.setDts(this->m_iFrame->pts);
    oPacket.setPts(this->m_iFrame->pkt_dts);
    oPacket.setDuration(m_iFrame->pkt_duration);
    oPacket.setTimeBase(this->timeBase());
    oPacket.setIndex(this->index());

    return oPacket;
}

QSize VideoStream::size()
{
    return QSize(this->codecContext()->width,
                 this->codecContext()->height);
}
