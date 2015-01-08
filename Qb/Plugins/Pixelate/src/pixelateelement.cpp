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

#include "pixelateelement.h"

PixelateElement::PixelateElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetBlockSize();
}

QSize PixelateElement::blockSize() const
{
    return this->m_blockSize;
}

void PixelateElement::setBlockSize(const QSize &blockSize)
{
    this->m_blockSize = blockSize;
}

void PixelateElement::resetBlockSize()
{
    this->setBlockSize(QSize(8, 8));
}

QbPacket PixelateElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage oFrame = QbUtils::packetToImage(iPacket);

    if (oFrame.isNull())
        return QbPacket();

    qreal sw = 1.0 / this->m_blockSize.width();
    qreal sh = 1.0 / this->m_blockSize.height();

    oFrame = oFrame.scaled(sw * oFrame.width(),
                           sh * oFrame.height(),
                           Qt::IgnoreAspectRatio,
                           Qt::FastTransformation)
                   .scaled(oFrame.width(),
                           oFrame.height(),
                           Qt::IgnoreAspectRatio,
                           Qt::FastTransformation);

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
