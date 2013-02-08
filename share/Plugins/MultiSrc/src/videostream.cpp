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
    if (!this->isValid())
        return;

    this->m_ffToFormat[PIX_FMT_YUV420P] = "I420";
    this->m_ffToFormat[PIX_FMT_YUV422P] = "YUY2";
    this->m_ffToFormat[PIX_FMT_UYVY422] = "UYVY";
    this->m_ffToFormat[PIX_FMT_YUVA420P] = "AYUV";
    this->m_ffToFormat[PIX_FMT_RGB0] = "RGBx";
    this->m_ffToFormat[PIX_FMT_BGR0] = "BGRx";
    this->m_ffToFormat[PIX_FMT_0RGB] = "xRGB";
    this->m_ffToFormat[PIX_FMT_0BGR] = "xBGR";
    this->m_ffToFormat[PIX_FMT_RGBA] = "RGBA";
    this->m_ffToFormat[PIX_FMT_BGRA] = "BGRA";
    this->m_ffToFormat[PIX_FMT_ARGB] = "ARGB";
    this->m_ffToFormat[PIX_FMT_ABGR] = "ABGR";
    this->m_ffToFormat[PIX_FMT_RGB24] = "RGB";
    this->m_ffToFormat[PIX_FMT_BGR24] = "BGR";
    this->m_ffToFormat[PIX_FMT_YUV411P] = "Y41B";
    this->m_ffToFormat[PIX_FMT_YUV444P] = "Y444";
    this->m_ffToFormat[PIX_FMT_YUV422P16LE] = "v216";
    this->m_ffToFormat[PIX_FMT_NV12] = "NV12";
    this->m_ffToFormat[PIX_FMT_NV21] = "NV21";
    this->m_ffToFormat[PIX_FMT_GRAY8] = "GRAY8";
    this->m_ffToFormat[PIX_FMT_GRAY16BE] = "GRAY16_BE";
    this->m_ffToFormat[PIX_FMT_GRAY16LE] = "GRAY16_LE";
    this->m_ffToFormat[PIX_FMT_RGB565LE] = "RGB16";
    this->m_ffToFormat[PIX_FMT_BGR565LE] = "BGR16";
    this->m_ffToFormat[PIX_FMT_RGB555LE] = "RGB15";
    this->m_ffToFormat[PIX_FMT_BGR555LE] = "BGR15";
    this->m_ffToFormat[PIX_FMT_YUV422P12LE] = "UYVP";
    this->m_ffToFormat[PIX_FMT_RGB8] = "RGB8P";
    this->m_ffToFormat[PIX_FMT_YUV420P10LE] = "I420_10LE";
    this->m_ffToFormat[PIX_FMT_YUV420P10BE] = "I420_10BE";
    this->m_ffToFormat[PIX_FMT_YUV422P10LE] = "I422_10LE";
    this->m_ffToFormat[PIX_FMT_YUV422P10BE] = "I422_10BE";
}

VideoStream::VideoStream(const VideoStream &other):
    AbstractStream(other),
    m_oFrame(other.m_oFrame),
    m_oCaps(other.m_oCaps),
    m_ffToFormat(other.m_ffToFormat)
{
}

VideoStream &VideoStream::operator =(const VideoStream &other)
{
    if (this != &other)
    {
        this->m_oFrame = other.m_oFrame;
        this->m_oCaps = other.m_oCaps;
        this->m_ffToFormat = other.m_ffToFormat;

        AbstractStream::operator =(other);
    }

    return *this;
}

QbCaps VideoStream::oCaps()
{
    PixelFormat fmt = this->codecContext()->pix_fmt;

    if (!this->m_ffToFormat.contains(fmt))
        return QbCaps();

    QString caps = QString("video/x-raw,"
                           "format=%1,"
                           "width=%2,"
                           "height=%3").arg(this->m_ffToFormat[fmt])
                                       .arg(this->codecContext()->width)
                                       .arg(this->codecContext()->height);

    return QbCaps(caps);
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

    if (this->oCaps() != this->m_oCaps)
    {
        this->m_oFrame.resize(avpicture_get_size(this->codecContext()->pix_fmt,
                                                 this->codecContext()->width,
                                                 this->codecContext()->height));

        this->m_oCaps = this->oCaps();
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
