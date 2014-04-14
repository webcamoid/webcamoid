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

#include "vcapsconvertelement.h"

VCapsConvertElement::VCapsConvertElement(): QbElement()
{
    av_register_all();

    this->resetCaps();
}

QString VCapsConvertElement::caps()
{
    return this->m_caps.toString();
}

void VCapsConvertElement::deleteSwsContext(SwsContext *context)
{
    sws_freeContext(context);
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
    if (packet.caps().mimeType() != "video/x-raw")
        return;

    if (packet.caps() == this->m_caps)
    {
        emit this->oStream(packet);

        return;
    }

    ConvertIO convertIO(packet, this->m_caps);

    if (convertIO.check() != this->m_check)
    {
        this->m_scaleContext =
                SwsContextPtr(sws_getCachedContext(NULL,
                                                   convertIO.iWidth(),
                                                   convertIO.iHeight(),
                                                   convertIO.iFormat(),
                                                   convertIO.oWidth(),
                                                   convertIO.oHeight(),
                                                   convertIO.oFormat(),
                                                   SWS_FAST_BILINEAR,
                                                   NULL,
                                                   NULL,
                                                   NULL),
                              this->deleteSwsContext);

        this->m_check = convertIO.check();
    }

    if (!this->m_scaleContext)
        return;

    int oBufferSize = avpicture_get_size(convertIO.oFormat(),
                                         convertIO.oWidth(),
                                         convertIO.oHeight());

    QbBufferPtr oBuffer(new uchar[oBufferSize]);

    AVPicture iPicture;

    avpicture_fill(&iPicture,
                   (uint8_t *) packet.buffer().data(),
                   convertIO.iFormat(),
                   convertIO.iWidth(),
                   convertIO.iHeight());

    AVPicture oPicture;

    avpicture_fill(&oPicture,
                   (uint8_t *) oBuffer.data(),
                   convertIO.oFormat(),
                   convertIO.oWidth(),
                   convertIO.oHeight());

    sws_scale(this->m_scaleContext.data(),
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
    oPacket.setDuration(packet.duration());
    oPacket.setTimeBase(packet.timeBase());
    oPacket.setIndex(packet.index());

    emit this->oStream(oPacket);
}
