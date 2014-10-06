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
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "warholelement.h"

WarholElement::WarholElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    this->m_colorTable << 0x000080 << 0x008000 << 0x800000
                       << 0x00e000 << 0x808000 << 0x800080
                       << 0x808080 << 0x008080 << 0xe0e000;

    this->resetNFrames();
}

int WarholElement::nFrames() const
{
    return this->m_nFrames;
}

void WarholElement::setNFrames(int nFrames)
{
    this->m_nFrames = nFrames;
}

void WarholElement::resetNFrames()
{
    this->setNFrames(3);
}

QbPacket WarholElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame = QImage(src.size(), src.format());

    quint32 *srcBits = (quint32 *) src.bits();
    quint32 *destBits = (quint32 *) oFrame.bits();

    for (int y = 0; y < src.height(); y++)
        for (int x = 0; x < src.width(); x++)
        {
            int p = (x * this->nFrames()) % src.width();
            int q = (y * this->nFrames()) % src.height();
            int i = ((y * this->nFrames()) / src.height()) * this->nFrames() +
                    ((x * this->nFrames()) / src.width());

            *destBits++ = srcBits[q * src.width() + p] ^ this->m_colorTable[i];
        }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
