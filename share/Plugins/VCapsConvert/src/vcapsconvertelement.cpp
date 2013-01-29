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

#include "vcapsconvertelement.h"

VCapsConvertElement::VCapsConvertElement(): QbElement()
{
    av_register_all();

    this->m_scaleContext = NULL;
    this->m_iPictureAlloc = -1;
    this->m_oPictureAlloc = -1;
    this->m_oWidth = -1;
    this->m_oHeight = -1;
    this->resetCaps();

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
}

VCapsConvertElement::~VCapsConvertElement()
{
    this->cleanAll();
}

QString VCapsConvertElement::caps()
{
    return this->m_caps.toString();
}

void VCapsConvertElement::cleanAll()
{
    if (this->m_oPictureAlloc >= 0)
    {
        avpicture_free(&this->m_oPicture);
        this->m_oPictureAlloc = -1;
    }
/*
    if (this->m_iPictureAlloc >= 0)
    {
        avpicture_free(&this->m_iPicture);
        this->m_iPictureAlloc = -1;
    }
*/
    if (this->m_scaleContext)
    {
        sws_freeContext(this->m_scaleContext);
        this->m_scaleContext = NULL;
    }
}

void VCapsConvertElement::setCaps(QString format)
{
    this->m_caps = QbCaps(format);
    QList<QByteArray> props = this->m_caps.dynamicPropertyNames();

    if (props.contains("width"))
        this->m_oWidth = this->m_caps.property("width").toInt();
    else
        this->m_oWidth = -1;

    if (props.contains("width"))
        this->m_oHeight = this->m_caps.property("height").toInt();
    else
        this->m_oHeight = -1;

    QString mimeType = this->m_caps.property("format").toString();

    if (this->m_mimeToFF.contains(mimeType))
        this->m_oFormat = this->m_mimeToFF[mimeType];
    else
        this->m_oFormat = PIX_FMT_NONE;
}

void VCapsConvertElement::resetCaps()
{
    this->setCaps("");
}

void VCapsConvertElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        packet.caps().mimeType() != "video/x-raw" ||
        this->state() != ElementStatePlaying)
        return;

    if (packet.caps() == this->m_caps)
    {
        emit this->oStream(packet);

        return;
    }

    int iWidth = packet.caps().property("width").toInt();
    int iHeight = packet.caps().property("height").toInt();
    QString mimeType = packet.caps().property("format").toString();

    PixelFormat iFormat;

    if (this->m_mimeToFF.contains(mimeType))
        iFormat = this->m_mimeToFF[mimeType];
    else
        return;

    if (packet.caps() != this->m_curInputCaps)
    {
        this->cleanAll();
        this->m_scaleContext = NULL;

        this->m_iPictureAlloc = avpicture_alloc(&this->m_iPicture,
                                                iFormat,
                                                iWidth,
                                                iHeight);

        if (this->m_oWidth < 0)
        {
            this->m_oWidth = iWidth;
            this->m_caps.setProperty("width", iWidth);
        }

        if (this->m_oHeight < 0)
        {
            this->m_oHeight = iHeight;
            this->m_caps.setProperty("height", iHeight);
        }

        this->m_oPictureAlloc = avpicture_alloc(&this->m_oPicture,
                                                this->m_oFormat,
                                                this->m_oWidth,
                                                this->m_oHeight);

        this->m_oFrame.resize(avpicture_get_size(this->m_oFormat,
                                                 this->m_oWidth,
                                                 this->m_oHeight));

        this->m_curInputCaps = packet.caps();
    }

    if (this->m_iPictureAlloc < 0 || this->m_oPictureAlloc < 0)
        return;

    this->m_scaleContext = sws_getCachedContext(this->m_scaleContext,
                                                iWidth,
                                                iHeight,
                                                iFormat,
                                                this->m_oWidth,
                                                this->m_oHeight,
                                                this->m_oFormat,
                                                SWS_FAST_BILINEAR,
                                                NULL,
                                                NULL,
                                                NULL);

    avpicture_fill(&this->m_iPicture,
                   (uint8_t *) packet.data(),
                   iFormat,
                   iWidth,
                   iHeight);

    sws_scale(this->m_scaleContext,
              (uint8_t **) this->m_iPicture.data,
              this->m_iPicture.linesize,
              0,
              iHeight,
              this->m_oPicture.data,
              this->m_oPicture.linesize);

    avpicture_layout(&this->m_oPicture,
                     this->m_oFormat,
                     this->m_oWidth,
                     this->m_oHeight,
                     (uint8_t *) this->m_oFrame.data(),
                     this->m_oFrame.size());

    QbPacket oPacket(QbCaps(this->m_caps.toString()),
                     this->m_oFrame.constData(),
                     this->m_oFrame.size());

    oPacket.setDts(packet.dts());
    oPacket.setPts(packet.pts());
    oPacket.setDuration(packet.duration());

    emit this->oStream(oPacket);
}
