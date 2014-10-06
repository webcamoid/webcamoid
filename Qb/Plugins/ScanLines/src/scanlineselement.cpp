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

#include "scanlineselement.h"

ScanLinesElement::ScanLinesElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    qRegisterMetaType<QRgb>("QRgb");

    this->resetShowSize();
    this->resetHideSize();
    this->resetHideColor();
}

int ScanLinesElement::showSize() const
{
    return this->m_showSize;
}

int ScanLinesElement::hideSize() const
{
    return this->m_hideSize;
}

QRgb ScanLinesElement::hideColor() const
{
    return this->m_hideColor;
}

void ScanLinesElement::setShowSize(int showSize)
{
    this->m_showSize = showSize;
}

void ScanLinesElement::setHideSize(int hideSize)
{
    this->m_hideSize = hideSize;
}

void ScanLinesElement::setHideColor(QRgb hideColor)
{
    this->m_hideColor = hideColor;
}

void ScanLinesElement::resetShowSize()
{
    this->setShowSize(1);
}

void ScanLinesElement::resetHideSize()
{
    this->setHideSize(4);
}

void ScanLinesElement::resetHideColor()
{
    this->setHideColor(qRgb(0, 0, 0));
}

QbPacket ScanLinesElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame(src.size(), src.format());

    for (int y = 0; y < src.height(); y++) {
        for (int i = 0; i < this->m_showSize && y < src.height(); i++, y++)
            memcpy(oFrame.scanLine(y), src.scanLine(y), src.bytesPerLine());

        for (int j = 0; j < this->m_hideSize && y < src.height(); j++, y++) {
            QRgb *line = (QRgb *) oFrame.scanLine(y);

            for (int x = 0; x < src.width(); x++)
                line[x] = this->m_hideColor;
        }
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
