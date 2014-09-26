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

#include "colorreplaceelement.h"

ColorReplaceElement::ColorReplaceElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    qRegisterMetaType<QRgb>("QRgb");

    this->resetFrom();
    this->resetTo();
    this->resetRadius();
}

QRgb ColorReplaceElement::from() const
{
    return this->m_from;
}

QRgb ColorReplaceElement::to() const
{
    return this->m_to;
}

float ColorReplaceElement::radius() const
{
    return this->m_radius;
}

void ColorReplaceElement::setFrom(QRgb from)
{
    this->m_from = from;
}

void ColorReplaceElement::setTo(QRgb to)
{
    this->m_to = to;
}

void ColorReplaceElement::setRadius(float radius)
{
    this->m_radius = radius;
}

void ColorReplaceElement::resetFrom()
{
    this->m_from = qRgb(0, 0, 0);
}

void ColorReplaceElement::resetTo()
{
    this->m_to = qRgb(0, 0, 0);
}

void ColorReplaceElement::resetRadius()
{
    this->setRadius(1.0);
}

QbPacket ColorReplaceElement::iStream(const QbPacket &packet)
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

        int rf = qRed(this->m_from);
        int gf = qGreen(this->m_from);
        int bf = qBlue(this->m_from);

        int rd = r - rf;
        int gd = g - gf;
        int bd = b - bf;

        float k = sqrt(rd * rd + gd * gd + bd * bd);

        if (k <= this->m_radius) {
            float p = k / this->m_radius;

            int rt = qRed(this->m_to);
            int gt = qGreen(this->m_to);
            int bt = qBlue(this->m_to);

            r = p * (r - rt) + rt;
            g = p * (g - gt) + gt;
            b = p * (b - bt) + bt;

            destBits[i] = qRgba(r, g, b, qAlpha(srcBits[i]));
        }
        else
            destBits[i] = srcBits[i];
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
