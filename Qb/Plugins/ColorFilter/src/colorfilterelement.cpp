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

#include "colorfilterelement.h"

ColorFilterElement::ColorFilterElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    qRegisterMetaType<QRgb>("QRgb");

    this->resetColor();
    this->resetRadius();
    this->resetGradient();
}

QRgb ColorFilterElement::color() const
{
    return this->m_color;
}

float ColorFilterElement::radius() const
{
    return this->m_radius;
}

bool ColorFilterElement::gradient() const
{
    return this->m_gradient;
}

void ColorFilterElement::setColor(QRgb color)
{
    this->m_color = color;
}

void ColorFilterElement::setRadius(float radius)
{
    this->m_radius = radius;
}

void ColorFilterElement::setGradient(bool gradient)
{
    this->m_gradient = gradient;
}

void ColorFilterElement::resetColor()
{
    this->m_color = qRgb(0, 0, 0);
}

void ColorFilterElement::resetRadius()
{
    this->setRadius(1.0);
}

void ColorFilterElement::resetGradient()
{
    this->setGradient(false);
}

QbPacket ColorFilterElement::iStream(const QbPacket &packet)
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
        int r = qRed(srcBits[i]);
        int g = qGreen(srcBits[i]);
        int b = qBlue(srcBits[i]);

        int rf = qRed(this->m_color);
        int gf = qGreen(this->m_color);
        int bf = qBlue(this->m_color);

        int rd = r - rf;
        int gd = g - gf;
        int bd = b - bf;

        float k = sqrt(rd * rd + gd * gd + bd * bd);

        if (k <= this->m_radius) {
            if (this->m_gradient) {
                float p = k / this->m_radius;

                int gray = qGray(srcBits[i]);

                r = p * (gray - r) + r;
                g = p * (gray - g) + g;
                b = p * (gray - b) + b;

                destBits[i] = qRgba(r, g, b, qAlpha(srcBits[i]));
            }
            else
                destBits[i] = srcBits[i];
        }
        else {
            int gray = qGray(srcBits[i]);
            destBits[i] = qRgba(gray, gray, gray, qAlpha(srcBits[i]));
        }
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
