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

    PixelFormat iFormat = av_get_pix_fmt(format.toStdString().c_str());

    QList<QByteArray> props = this->m_caps.dynamicPropertyNames();

    int oWidth = props.contains("width")?
                     this->m_caps.property("width").toInt():
                     iWidth;

    int oHeight = props.contains("height")?
                      this->m_caps.property("height").toInt():
                      iHeight;

    PixelFormat oFormat;

    if (props.contains("format"))
    {
        QString oFormatString = this->m_caps.property("format").toString();

        oFormat = av_get_pix_fmt(oFormatString.toStdString().c_str());
    }
    else
        oFormat = iFormat;

    SwsContext *scaleContext = sws_getCachedContext(NULL,
                                                    iWidth,
                                                    iHeight,
                                                    iFormat,
                                                    oWidth,
                                                    oHeight,
                                                    oFormat,
                                                    SWS_FAST_BILINEAR,
                                                    NULL,
                                                    NULL,
                                                    NULL);

    if (!scaleContext)
        return;

    int oBufferSize = avpicture_get_size(oFormat,
                                         oWidth,
                                         oHeight);

    QSharedPointer<uchar> oBuffer(new uchar[oBufferSize]);

    AVPicture iPicture;

    avpicture_fill(&iPicture,
                   (uint8_t *) packet.buffer().data(),
                   iFormat,
                   iWidth,
                   iHeight);

    AVPicture oPicture;

    avpicture_fill(&oPicture,
                   (uint8_t *) oBuffer.data(),
                   oFormat,
                   oWidth,
                   oHeight);

    sws_scale(scaleContext,
              (uint8_t **) iPicture.data,
              iPicture.linesize,
              0,
              iHeight,
              oPicture.data,
              oPicture.linesize);

    sws_freeContext(scaleContext);

    QbPacket oPacket(packet.caps().update(this->m_caps),
                     oBuffer,
                     oBufferSize);

    oPacket.setPts(packet.pts());
    oPacket.setDuration(packet.duration());
    oPacket.setTimeBase(packet.timeBase());
    oPacket.setIndex(packet.index());

    emit this->oStream(oPacket);
}
