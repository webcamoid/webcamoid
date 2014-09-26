/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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

#include "quarkelement.h"

QuarkElement::QuarkElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetPlanes();
    this->m_plane = 0;
}

int QuarkElement::planes() const
{
    return this->m_planes;
}

void QuarkElement::setPlanes(int planes)
{
    this->m_planes = planes;
}

void QuarkElement::resetPlanes()
{
    this->setPlanes(16);
}

QbPacket QuarkElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    int videoArea = src.width() * src.height();

    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();


    if (packet.caps() != this->m_caps) {
        this->m_buffer = QImage(src.width(), src.height() * this->m_planes, QImage::Format_RGB32);
        this->m_planeTable.resize(this->m_planes);

        for(int i = 0; i < this->m_planes; i++)
            this->m_planeTable[i] = (QRgb *) this->m_buffer.bits() + videoArea * i;

        this->m_plane = this->m_planes - 1;

        this->m_caps = packet.caps();
    }

    memcpy(this->m_planeTable[this->m_plane], srcBits, src.bytesPerLine() * src.height());

    for (int i = 0; i < videoArea; i++) {
        int cf = (rand() >> 24) & (this->m_planes - 1);
        destBits[i] = this->m_planeTable[cf][i];
    }

    this->m_plane--;

    if (this->m_plane < 0)
        this->m_plane = this->m_planes - 1;

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
