/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "vcapsconvertelement.h"

VCapsConvertElement::VCapsConvertElement(): QbElement()
{
    av_register_all();
    this->m_scaleContext = NULL;

    this->resetCaps();
}

VCapsConvertElement::~VCapsConvertElement()
{
    this->deleteSwsContext();
}

QString VCapsConvertElement::caps() const
{
    return this->m_caps.toString();
}

void VCapsConvertElement::deleteSwsContext()
{
    if (this->m_scaleContext) {
        sws_freeContext(this->m_scaleContext);
        this->m_scaleContext = NULL;
    }
}

void VCapsConvertElement::setCaps(const QString &format)
{
    this->m_caps = QbCaps(format);
}

void VCapsConvertElement::resetCaps()
{
    this->setCaps("");
}

QbPacket VCapsConvertElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() != "video/x-raw")
        return QbPacket();

    if (packet.caps() == this->m_caps)
        qbSend(packet)

    ConvertIO convertIO(packet, this->m_caps);

    if (convertIO.check() != this->m_check) {
        this->m_scaleContext = sws_getCachedContext(this->m_scaleContext,
                                                    convertIO.iWidth(),
                                                    convertIO.iHeight(),
                                                    convertIO.iFormat(),
                                                    convertIO.oWidth(),
                                                    convertIO.oHeight(),
                                                    convertIO.oFormat(),
                                                    SWS_FAST_BILINEAR,
                                                    NULL,
                                                    NULL,
                                                    NULL);

        this->m_check = convertIO.check();
    }

    if (!this->m_scaleContext)
        return QbPacket();

    int oBufferSize = avpicture_get_size(convertIO.oFormat(),
                                         convertIO.oWidth(),
                                         convertIO.oHeight());

    QbBufferPtr oBuffer(new char[oBufferSize]);

    AVPicture iPicture;
    memset(&iPicture, 0, sizeof(AVPicture));

    avpicture_fill(&iPicture,
                   (const uint8_t *) packet.buffer().data(),
                   convertIO.iFormat(),
                   convertIO.iWidth(),
                   convertIO.iHeight());

    AVPicture oPicture;
    memset(&oPicture, 0, sizeof(AVPicture));

    avpicture_fill(&oPicture,
                   (uint8_t *) oBuffer.data(),
                   convertIO.oFormat(),
                   convertIO.oWidth(),
                   convertIO.oHeight());

    sws_scale(this->m_scaleContext,
              (uint8_t **) iPicture.data,
              iPicture.linesize,
              0,
              convertIO.iHeight(),
              oPicture.data,
              oPicture.linesize);

    QbPacket oPacket(packet.caps().update(this->m_caps),
                     oBuffer,
                     oBufferSize);

    oPacket.setPts(packet.pts());
    oPacket.setTimeBase(packet.timeBase());
    oPacket.setIndex(packet.index());

    qbSend(oPacket)
}
