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

#include "colortapelement.h"

ColorTapElement::ColorTapElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetTable();
}

QString ColorTapElement::table() const
{
    return this->m_tableName;
}

void ColorTapElement::setTable(const QString &table)
{
    this->m_tableName = table;
    this->m_table = QImage(table);
}

void ColorTapElement::resetTable()
{
    this->setTable(":/ColorTap/share/xpro.bmp");
}

QbPacket ColorTapElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    int videoArea = src.width() * src.height();

    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();
    QRgb *tableBits = (QRgb *) this->m_table.bits();

    for (int i = 0; i < videoArea; i++) {
        int r = qRed(srcBits[i]);
        int g = qGreen(srcBits[i]);
        int b = qBlue(srcBits[i]);

        int ro = qRed(tableBits[r]);
        int go = qGreen(tableBits[g]);
        int bo = qBlue(tableBits[b]);

        destBits[i] = qRgb(ro, go, bo);
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
