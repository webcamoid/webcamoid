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

#include "implodeelement.h"

ImplodeElement::ImplodeElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetAmount();
}

float ImplodeElement::amount() const
{
    return this->m_amount;
}

void ImplodeElement::setAmount(float amount)
{
    this->m_amount = amount;
}

void ImplodeElement::resetAmount()
{
    this->setAmount(1);
}

QbPacket ImplodeElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame(src.size(), src.format());

    float xScale = 1.0;
    float yScale = 1.0;
    float xCenter = src.width() >> 1;
    float yCenter = src.height() >> 1;
    float radius = xCenter;

    if (src.width() > src.height())
        yScale = (float) src.width() / src.height();
    else if (src.width() < src.height()) {
        xScale = (float) src.height() / src.width();
        radius = yCenter;
    }

    for (int y = 0; y < src.height(); y++) {
        QRgb *srcBits = (QRgb *) src.scanLine(y);
        QRgb *destBits = (QRgb *) oFrame.scanLine(y);
        float yDistance = yScale * (y - yCenter);

        for (int x = 0; x < src.width(); x++) {
            float xDistance = xScale * (x - xCenter);
            float distance = xDistance * xDistance + yDistance * yDistance;

            if (distance >= (radius * radius))
                *destBits = srcBits[x];
            else {
                float factor = 1.0;

                if (distance > 0.0)
                    factor = pow(sin((M_PI) * sqrt(distance) / radius / 2), -this->m_amount);

                *destBits = this->interpolate(src, factor * xDistance / xScale + xCenter,
                                               factor * yDistance / yScale + yCenter);
            }

            destBits++;
        }
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
