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

#include "oilpaintelement.h"
#include "defs.h"

OilPaintElement::OilPaintElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    this->resetRadius();
}

int OilPaintElement::radius() const
{
    return this->m_radius;
}

void OilPaintElement::setRadius(int radius)
{
    this->m_radius = radius;
}

void OilPaintElement::resetRadius()
{
    this->setRadius(1);
}

QbPacket OilPaintElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame(src.size(), src.format());
    quint16 histogram[256];
    QRgb *oLine = (QRgb *) oFrame.bits();
    int scanBlockLen = (this->m_radius << 1) + 1;
    QRgb *scanBlock[scanBlockLen];

    for (int y = 0; y < src.height(); y++) {
        int pos = y - this->m_radius;

        for (int j = 0; j < scanBlockLen; j++) {
            scanBlock[j] = (QRgb *) src.constScanLine(qBound(0, pos, src.height()));
            pos++;
        }

        for (int x = 0; x < src.width(); x++) {
            int minI = x - this->m_radius;
            int maxI = x + this->m_radius + 1;

            if (minI < 0)
                minI = 0;

            if (maxI > src.width())
                maxI = src.width();

            memset(histogram, 0, 512);
            quint16 max = 0;

            for (int j = 0; j < scanBlockLen; j++)
                for (int i = minI; i < maxI; i++) {
                    Pixel *p = (Pixel *) &scanBlock[j][i];
                    quint16 value = ++histogram[(p->r + p->g + p->b) / 3];

                    if (value > max) {
                        max = value;
                        *oLine = scanBlock[j][i];
                    }
                }

            oLine++;
        }
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
