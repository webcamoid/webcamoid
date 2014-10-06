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

#include "scrollelement.h"

ScrollElement::ScrollElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    this->m_offset = 0;
    this->resetScrollSteps();
}

int ScrollElement::scrollSteps() const
{
    return this->m_scrollSteps;
}

void ScrollElement::addNoise(QImage &dest)
{
    quint32 *destBits = (quint32 *) dest.bits();
    int y;
    int dy;

    for (y = this->m_offset, dy = 0; dy < 3 && y < dest.height(); y++, dy++) {
        int i = y * dest.width();

        for (int x = 0; x < dest.width(); x++, i++) {
            if (dy == 2 && qrand() >> 31)
                continue;

            destBits[i] = (qrand() >> 31) ? 0xffffff : 0;
        }
    }
}

void ScrollElement::setScrollSteps(int scrollSteps)
{
    this->m_scrollSteps = scrollSteps;
}

void ScrollElement::resetScrollSteps()
{
    this->setScrollSteps(30);
}

QbPacket ScrollElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame = QImage(src.size(), src.format());

    quint32 *srcBits = (quint32 *) src.bits();
    quint32 *destBits = (quint32 *) oFrame.bits();

    if (src.size() != this->m_curSize) {
        this->m_offset = 0;
        this->m_curSize = src.size();
    }

    memcpy(destBits,
           srcBits + (src.height() - this->m_offset) * src.width(),
           sizeof(qint32) * this->m_offset * src.width());

    memcpy(destBits + this->m_offset * src.width(),
           srcBits,
           sizeof(qint32) * (src.height() - this->m_offset) * src.width());

    this->addNoise(oFrame);

    this->m_offset += this->m_scrollSteps;

    if (this->m_offset >= src.height())
        this->m_offset = 0;

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
