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

#include "cinemaelement.h"

CinemaElement::CinemaElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    qRegisterMetaType<QRgb>("QRgb");

    this->resetStripSize();
    this->resetStripColor();
}

float CinemaElement::stripSize() const
{
    return this->m_stripSize;
}

QRgb CinemaElement::stripColor() const
{
    return this->m_stripColor;
}

void CinemaElement::setStripSize(float stripSize)
{
    this->m_stripSize = stripSize;
}

void CinemaElement::setStripColor(QRgb hideColor)
{
    this->m_stripColor = hideColor;
}

void CinemaElement::resetStripSize()
{
    this->setStripSize(0.5);
}

void CinemaElement::resetStripColor()
{
    this->setStripColor(qRgb(0, 0, 0));
}

QbPacket CinemaElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame(src.size(), src.format());
    int cy = src.height() >> 1;

    for (int y = 0; y < src.height(); y++) {
        float k = 1.0 - fabs(y - cy) / cy;
        QRgb *iLine = (QRgb *) src.scanLine(y);
        QRgb *oLine = (QRgb *) oFrame.scanLine(y);

        if (k >= this->m_stripSize)
            memcpy(oLine, iLine, src.bytesPerLine());
        else
            for (int x = 0; x < src.width(); x++) {
                float a = qAlpha(this->m_stripColor) / 255.0;

                int r = a * (qRed(this->m_stripColor) - qRed(iLine[x])) + qRed(iLine[x]);
                int g = a * (qGreen(this->m_stripColor) - qGreen(iLine[x])) + qGreen(iLine[x]);
                int b = a * (qBlue(this->m_stripColor) - qBlue(iLine[x])) + qBlue(iLine[x]);

                oLine[x] = qRgba(r, g, b, qAlpha(iLine[x]));
            }
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
