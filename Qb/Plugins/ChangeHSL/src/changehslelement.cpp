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

#include "changehslelement.h"

ChangeHSLElement::ChangeHSLElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    qRegisterMetaType<QRgb>("QRgb");

    this->resetHsl();
}

QVariantList ChangeHSLElement::hsl() const
{
    QVariantList hsl;

    foreach (float e, this->m_hsl)
        hsl << e - 1;

    return hsl;
}

void ChangeHSLElement::setHsl(const QVariantList &hsl)
{
    this->m_hsl.clear();

    foreach (QVariant e, hsl)
        this->m_hsl << e.toFloat() + 1;
}

void ChangeHSLElement::resetHsl()
{
    this->m_hsl.clear();
    this->m_hsl << 1 << 1 << 1;
}

QbPacket ChangeHSLElement::iStream(const QbPacket &packet)
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
        int h;
        int s;
        int l;
        int a;

        QColor(srcBits[i]).getHsl(&h, &s, &l, &a);

        h *= this->m_hsl[0];
        s *= this->m_hsl[1];
        l *= this->m_hsl[2];

        h = qBound(0, h, 255);
        s = qBound(0, s, 255);
        l = qBound(0, l, 255);

        QColor color;
        color.setHsl(h, s, l, a);

        destBits[i] = color.rgba();
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
