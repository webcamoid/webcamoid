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

    this->m_oWidth = -1;
    this->m_oHeight = -1;
    this->resetCaps();
}

VCapsConvertElement::~VCapsConvertElement()
{
}

QString VCapsConvertElement::caps()
{
    return this->m_caps.toString();
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

    if (props.contains("format"))
    {
        QString oFormat = this->m_caps.property("format").toString();

        this->m_oFormat = av_get_pix_fmt(oFormat.toUtf8().constData());
    }
    else
        this->m_oFormat = AV_PIX_FMT_NONE;
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
    QString format = packet.caps().property("format").toString();

    PixelFormat iFormat = av_get_pix_fmt(format.toUtf8().constData());

    if (packet.caps() != this->m_curInputCaps)
    {
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

        if (this->m_oFormat == AV_PIX_FMT_NONE)
        {
            this->m_oFormat = iFormat;
            this->m_caps.setProperty("format", format);
        }

        this->m_oFrame.resize(avpicture_get_size(this->m_oFormat,
                                                 this->m_oWidth,
                                                 this->m_oHeight));

        QString fps = packet.caps().property("fps").toString();

        this->m_caps.setProperty("fps", fps);
        this->m_curInputCaps = packet.caps();
    }

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

    AVPicture iPicture;

    avpicture_fill(&iPicture,
                   (uint8_t *) packet.data(),
                   iFormat,
                   iWidth,
                   iHeight);

    AVPicture oPicture;

    avpicture_fill(&oPicture,
                   (uint8_t *) this->m_oFrame.data(),
                   this->m_oFormat,
                   this->m_oWidth,
                   this->m_oHeight);

    sws_scale(this->m_scaleContext,
              (uint8_t **) iPicture.data,
              iPicture.linesize,
              0,
              iHeight,
              oPicture.data,
              oPicture.linesize);

    QbPacket oPacket(this->m_caps,
                     this->m_oFrame.constData(),
                     this->m_oFrame.size());

    oPacket.setDts(packet.dts());
    oPacket.setPts(packet.pts());
    oPacket.setDuration(packet.duration());
    oPacket.setTimeBase(packet.timeBase());
    oPacket.setIndex(packet.index());

    emit this->oStream(oPacket);
}

void VCapsConvertElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);

    if (this->state() == ElementStateNull ||
        this->state() == ElementStateReady)
    {
        if (this->m_scaleContext)
        {
            sws_freeContext(this->m_scaleContext);
            this->m_scaleContext = NULL;
        }

        this->m_curInputCaps = QbCaps();
    }
}
