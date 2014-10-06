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

#include "temperatureelement.h"

TemperatureElement::TemperatureElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    qRegisterMetaType<QRgb>("QRgb");

    this->resetTemperature();
}

float TemperatureElement::temperature() const
{
    return this->m_temperature;
}

void TemperatureElement::setTemperature(float temperature)
{
    this->m_temperature = temperature;

    this->colorFromTemperature(temperature,
                               &this->m_kr, &this->m_kg, &this->m_kb);
}

void TemperatureElement::resetTemperature()
{
    this->setTemperature(6500);
}

QbPacket TemperatureElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    int videoArea = src.width() * src.height();

    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    for (int i = 0; i < videoArea; i++) {
        int r = this->m_kr * qRed(srcBits[i]);
        int g = this->m_kg * qGreen(srcBits[i]);
        int b = this->m_kb * qBlue(srcBits[i]);

        r = qBound(0, r, 255);
        g = qBound(0, g, 255);
        b = qBound(0, b, 255);

        destBits[i] = qRgba(r, g, b, qAlpha(srcBits[i]));
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
