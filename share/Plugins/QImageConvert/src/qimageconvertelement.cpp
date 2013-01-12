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

#include "qimageconvertelement.h"

QImageConvertElement::QImageConvertElement(): QbElement()
{
    av_register_all();

    this->m_scaleContext = NULL;
    this->m_iPictureAlloc = -1;
    this->m_oPictureAlloc = -1;
    this->resetFormat();

    this->m_mimeToFF["I420"] = PIX_FMT_YUV420P;
    this->m_mimeToFF["YUY2"] = PIX_FMT_YUV422P;
    this->m_mimeToFF["UYVY"] = PIX_FMT_UYVY422;
    this->m_mimeToFF["AYUV"] = PIX_FMT_YUVA420P;
    this->m_mimeToFF["RGBx"] = PIX_FMT_RGB0;
    this->m_mimeToFF["BGRx"] = PIX_FMT_BGR0;
    this->m_mimeToFF["xRGB"] = PIX_FMT_0RGB;
    this->m_mimeToFF["xBGR"] = PIX_FMT_0BGR;
    this->m_mimeToFF["RGBA"] = PIX_FMT_RGBA;
    this->m_mimeToFF["BGRA"] = PIX_FMT_BGRA;
    this->m_mimeToFF["ARGB"] = PIX_FMT_ARGB;
    this->m_mimeToFF["ABGR"] = PIX_FMT_ABGR;
    this->m_mimeToFF["RGB"] = PIX_FMT_RGB24;
    this->m_mimeToFF["BGR"] = PIX_FMT_BGR24;
    this->m_mimeToFF["Y41B"] = PIX_FMT_YUV411P;
    this->m_mimeToFF["Y42B"] = PIX_FMT_YUV422P;
    this->m_mimeToFF["YVYU"] = PIX_FMT_UYVY422;
    this->m_mimeToFF["Y444"] = PIX_FMT_YUV444P;
    this->m_mimeToFF["v210"] = PIX_FMT_YUV422P10LE;
    this->m_mimeToFF["v216"] = PIX_FMT_YUV422P16LE;
    this->m_mimeToFF["NV12"] = PIX_FMT_NV12;
    this->m_mimeToFF["NV21"] = PIX_FMT_NV21;
    this->m_mimeToFF["GRAY8"] = PIX_FMT_GRAY8;
    this->m_mimeToFF["GRAY16_BE"] = PIX_FMT_GRAY16BE;
    this->m_mimeToFF["GRAY16_LE"] = PIX_FMT_GRAY16LE;
    this->m_mimeToFF["v308"] = PIX_FMT_YUV444P;
    this->m_mimeToFF["RGB16"] = PIX_FMT_RGB565LE;
    this->m_mimeToFF["BGR16"] = PIX_FMT_BGR565LE;
    this->m_mimeToFF["RGB15"] = PIX_FMT_RGB555LE;
    this->m_mimeToFF["BGR15"] = PIX_FMT_BGR555LE;
    this->m_mimeToFF["UYVP"] = PIX_FMT_YUV422P12LE;
    this->m_mimeToFF["A420"] = PIX_FMT_YUVA420P;
    this->m_mimeToFF["RGB8P"] = PIX_FMT_RGB8;
    this->m_mimeToFF["IYU1"] = PIX_FMT_YUV411P;
    this->m_mimeToFF["I420_10LE"] = PIX_FMT_YUV420P10LE;
    this->m_mimeToFF["I420_10BE"] = PIX_FMT_YUV420P10BE;
    this->m_mimeToFF["I422_10LE"] = PIX_FMT_YUV422P10LE;
    this->m_mimeToFF["I422_10BE"] = PIX_FMT_YUV422P10BE;

    this->m_imageToFF[QImage::Format_Mono] = PIX_FMT_MONOBLACK;
    this->m_imageToFF[QImage::Format_MonoLSB] = PIX_FMT_MONOWHITE;
    this->m_imageToFF[QImage::Format_RGB32] = PIX_FMT_BGR0;
    this->m_imageToFF[QImage::Format_ARGB32] = PIX_FMT_BGRA;
    this->m_imageToFF[QImage::Format_RGB16] = PIX_FMT_RGB565LE;
    this->m_imageToFF[QImage::Format_RGB555] = PIX_FMT_RGB555LE;
    this->m_imageToFF[QImage::Format_RGB888] = PIX_FMT_RGB24;
    this->m_imageToFF[QImage::Format_RGB444] = PIX_FMT_RGB444LE;
}

QImageConvertElement::~QImageConvertElement()
{
    this->cleanAll();
}

QString QImageConvertElement::format()
{
    return this->m_format;
}

void QImageConvertElement::cleanAll()
{
    if (this->m_oPictureAlloc >= 0)
        avpicture_free(&this->m_oPicture);
/*
    if (this->m_iPictureAlloc >= 0)
        avpicture_free(&this->m_iPicture);
*/
    if (this->m_scaleContext)
        sws_freeContext(this->m_scaleContext);
}

void QImageConvertElement::setFormat(QString format)
{
    this->m_format = format;

    if (format == "Mono")
        this->m_qFormat = QImage::Format_Mono;
    else if (format == "MonoLSB")
        this->m_qFormat = QImage::Format_MonoLSB;
    else if (format == "Indexed8")
        this->m_qFormat = QImage::Format_Indexed8;
    else if (format == "RGB32")
        this->m_qFormat = QImage::Format_RGB32;
    else if (format == "ARGB32")
        this->m_qFormat = QImage::Format_ARGB32;
    else if (format == "ARGB32_Premultiplied")
        this->m_qFormat = QImage::Format_ARGB32_Premultiplied;
    else if (format == "RGB16")
        this->m_qFormat = QImage::Format_RGB16;
    else if (format == "ARGB8565_Premultiplied")
        this->m_qFormat = QImage::Format_ARGB8565_Premultiplied;
    else if (format == "RGB666")
        this->m_qFormat = QImage::Format_RGB666;
    else if (format == "ARGB6666_Premultiplied")
        this->m_qFormat = QImage::Format_ARGB6666_Premultiplied;
    else if (format == "RGB555")
        this->m_qFormat = QImage::Format_RGB555;
    else if (format == "ARGB8555_Premultiplied")
        this->m_qFormat = QImage::Format_ARGB8555_Premultiplied;
    else if (format == "RGB888")
        this->m_qFormat = QImage::Format_RGB888;
    else if (format == "RGB444")
        this->m_qFormat = QImage::Format_RGB444;
    else if (format == "ARGB4444_Premultiplied")
        this->m_qFormat = QImage::Format_ARGB4444_Premultiplied;
    else
    {
        this->m_format = "";
        this->m_qFormat = QImage::Format_Invalid;
    }
}

void QImageConvertElement::resetFormat()
{
    this->setFormat("RGB888");
}

void QImageConvertElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        packet.caps().mimeType() != "video/x-raw" ||
        this->state() != ElementStatePlaying)
        return;

    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();
    QString format = packet.caps().property("format").toString();

    PixelFormat iFormat;

    if (this->m_mimeToFF.contains(format))
        iFormat = this->m_mimeToFF[format];
    else
        return;

    PixelFormat oFormat;

    if (this->m_imageToFF.contains(this->m_qFormat))
        oFormat = this->m_imageToFF[this->m_qFormat];
    else
        return;

    if (packet.caps() != this->m_curCaps)
    {
        this->cleanAll();
        this->m_scaleContext = NULL;

        this->m_iPictureAlloc = avpicture_alloc(&this->m_iPicture,
                                                iFormat,
                                                width,
                                                height);

        this->m_oPictureAlloc = avpicture_alloc(&this->m_oPicture,
                                                oFormat,
                                                width,
                                                height);

        this->m_curCaps = packet.caps();
    }

    if (this->m_iPictureAlloc < 0)
        return;

    if (this->m_oPictureAlloc < 0)
        return;

    this->m_scaleContext = sws_getCachedContext(this->m_scaleContext,
                                                width,
                                                height,
                                                iFormat,
                                                width,
                                                height,
                                                oFormat,
                                                SWS_FAST_BILINEAR,
                                                NULL,
                                                NULL,
                                                NULL);

    avpicture_fill(&this->m_iPicture,
                   (uint8_t *) packet.data(),
                   iFormat,
                   width,
                   height);

    sws_scale(this->m_scaleContext,
              (uint8_t **) this->m_iPicture.data,
              this->m_iPicture.linesize,
              0,
              height,
              this->m_oPicture.data,
              this->m_oPicture.linesize);

    this->m_oFrame = QImage(width,
                            height,
                            this->m_qFormat);


    avpicture_layout(&this->m_oPicture,
                     oFormat,
                     width,
                     height,
                     (uint8_t *) this->m_oFrame.bits(),
                     this->m_oFrame.byteCount());

    QbPacket oPacket(QbCaps("application/x-qt-image"), &this->m_oFrame);

    oPacket.setDts(packet.dts());
    oPacket.setPts(packet.pts());
    oPacket.setDuration(packet.duration());

    emit this->oStream(oPacket);
}
