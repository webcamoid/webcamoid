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

extern "C"
{
    #include <libavutil/imgutils.h>
}

#include "videostream.h"

VideoStream::VideoStream(QObject *parent): AbstractStream(parent)
{
    this->m_oBufferSize = 0;
}

VideoStream::VideoStream(AVFormatContext *formatContext, uint index):
    AbstractStream(formatContext, index)
{
    this->m_oBufferSize = 0;

    if (!this->isValid())
    {
        this->cleanUp();

        return;
    }

    this->m_isValid = false;

    this->m_oBufferSize = av_image_alloc(this->m_oBuffer,
                                         this->m_oBufferLineSize,
                                         this->codecContext()->width,
                                         this->codecContext()->height,
                                         this->codecContext()->pix_fmt,
                                         1);

    if (this->m_oBufferSize < 0)
    {
        this->cleanUp();

        return;
    }

    this->m_ffToMime[PIX_FMT_YUV420P] = "I420";
    this->m_ffToMime[PIX_FMT_YUV422P] = "YUY2";
    this->m_ffToMime[PIX_FMT_UYVY422] = "UYVY";
    this->m_ffToMime[PIX_FMT_YUVA420P] = "AYUV";
    this->m_ffToMime[PIX_FMT_RGB0] = "RGBx";
    this->m_ffToMime[PIX_FMT_BGR0] = "BGRx";
    this->m_ffToMime[PIX_FMT_0RGB] = "xRGB";
    this->m_ffToMime[PIX_FMT_0BGR] = "xBGR";
    this->m_ffToMime[PIX_FMT_RGBA] = "RGBA";
    this->m_ffToMime[PIX_FMT_BGRA] = "BGRA";
    this->m_ffToMime[PIX_FMT_ARGB] = "ARGB";
    this->m_ffToMime[PIX_FMT_ABGR] = "ABGR";
    this->m_ffToMime[PIX_FMT_RGB24] = "RGB";
    this->m_ffToMime[PIX_FMT_BGR24] = "BGR";
    this->m_ffToMime[PIX_FMT_YUV411P] = "Y41B";
    this->m_ffToMime[PIX_FMT_YUV444P] = "Y444";
    this->m_ffToMime[PIX_FMT_YUV422P16LE] = "v216";
    this->m_ffToMime[PIX_FMT_NV12] = "NV12";
    this->m_ffToMime[PIX_FMT_NV21] = "NV21";
    this->m_ffToMime[PIX_FMT_GRAY8] = "GRAY8";
    this->m_ffToMime[PIX_FMT_GRAY16BE] = "GRAY16_BE";
    this->m_ffToMime[PIX_FMT_GRAY16LE] = "GRAY16_LE";
    this->m_ffToMime[PIX_FMT_RGB565LE] = "RGB16";
    this->m_ffToMime[PIX_FMT_BGR565LE] = "BGR16";
    this->m_ffToMime[PIX_FMT_RGB555LE] = "RGB15";
    this->m_ffToMime[PIX_FMT_BGR555LE] = "BGR15";
    this->m_ffToMime[PIX_FMT_YUV422P12LE] = "UYVP";
    this->m_ffToMime[PIX_FMT_RGB8] = "RGB8P";
    this->m_ffToMime[PIX_FMT_YUV420P10LE] = "I420_10LE";
    this->m_ffToMime[PIX_FMT_YUV420P10BE] = "I420_10BE";
    this->m_ffToMime[PIX_FMT_YUV422P10LE] = "I422_10LE";
    this->m_ffToMime[PIX_FMT_YUV422P10BE] = "I422_10BE";

    this->m_isValid = true;
}

VideoStream::VideoStream(const VideoStream &other):
    AbstractStream(other),
    m_oFrame(other.m_oFrame),
    m_oBufferSize(other.m_oBufferSize),
    m_ffToMime(other.m_ffToMime)
{
    for (int i = 0; i < 4; i++)
    {
        this->m_oBuffer[i] = other.m_oBuffer[i];
        this->m_oBufferLineSize[i] = other.m_oBufferLineSize[i];
    }
}

VideoStream::~VideoStream()
{
    if (this->m_orig || !this->m_copy.isEmpty())
        return;

    this->cleanUp();
}

VideoStream &VideoStream::operator =(const VideoStream &other)
{
    if (this != &other)
    {
        this->m_oFrame = other.m_oFrame;

        for (int i = 0; i < 4; i++)
        {
            this->m_oBuffer[i] = other.m_oBuffer[i];
            this->m_oBufferLineSize[i] = other.m_oBufferLineSize[i];
        }

        this->m_oBufferSize = other.m_oBufferSize;
        this->m_ffToMime = other.m_ffToMime;

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

    this->m_oFrame.resize(avpicture_get_size(this->codecContext()->pix_fmt,
                                             this->codecContext()->width,
                                             this->codecContext()->height));

    avpicture_layout((AVPicture *) this->m_iFrame,
                     this->codecContext()->pix_fmt,
                     this->codecContext()->width,
                     this->codecContext()->height,
                     (uint8_t *) this->m_oFrame.data(),
                     this->m_oFrame.size());

    PixelFormat fmt = this->codecContext()->pix_fmt;

    if (!this->m_ffToMime.contains(fmt))
        return QbPacket();

    QbPacket oPacket(QString("video/x-raw,"
                             "format=%1,"
                             "width=%2,"
                             "height=%3").arg(this->m_ffToMime[fmt])
                                         .arg(this->codecContext()->width)
                                         .arg(this->codecContext()->height),
                    this->m_oFrame.constData(),
                    this->m_oFrame.size());

    oPacket.setDts(packet->dts);
    oPacket.setPts(packet->pts);
    oPacket.setDuration(packet->duration);
    oPacket.setIndex(this->index());

    return oPacket;
}

void VideoStream::cleanUp()
{
    if (this->m_oBuffer)
        av_free(this->m_oBuffer[0]);

    AbstractStream::cleanUp();
}
