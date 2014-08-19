/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
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

#include "falsecolorelement.h"

FalseColorElement::FalseColorElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    qRegisterMetaType<QRgb>("QRgb");

    this->resetGradient();
}

QVariantList FalseColorElement::table() const
{
    QVariantList table;

    foreach (QRgb color, this->m_table)
        table << color;

    return table;
}

bool FalseColorElement::gradient() const
{
    return this->m_gradient;
}

void FalseColorElement::setTable(const QVariantList &table)
{
    this->m_table.clear();

    foreach (QVariant color, table)
        this->m_table << color.value<QRgb>();
}

void FalseColorElement::setGradient(bool gradient)
{
    this->m_gradient = gradient;
}

void FalseColorElement::resetTable()
{
    this->m_table.clear();
}

void FalseColorElement::resetGradient()
{
    this->setGradient(false);
}

void FalseColorElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void FalseColorElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void FalseColorElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_ARGB32);

    int videoArea = width * height;

    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    quint8 *grayBuffer = new quint8[videoArea];
    int minGray = 255;
    int maxGray = 0;

    for (int i = 0; i < videoArea; i++) {
        int gray = qGray(srcBits[i]);
        grayBuffer[i] = gray;

        if (gray < minGray)
            minGray = gray;

        if (gray > maxGray)
            maxGray = gray;
    }

    QRgb *table = new QRgb[256];
    int grayDiff = maxGray - minGray;

    if (grayDiff > 0) {
        int tableSize = this->m_table.size() - 1;

        if (this->m_gradient)
            tableSize--;

        int size = this->m_table.size();

        if (this->m_gradient)
            size--;

        float j = (float) grayDiff / (this->m_table.size() - 1);

        for (int gray = minGray; gray <= maxGray; gray++) {
            int low = size * (gray - minGray) / grayDiff;
            low = qMin(low, tableSize);

            QRgb color;

            if (this->m_gradient) {
                int high = low + 1;

                int rl = qRed(this->m_table[low]);
                int gl = qGreen(this->m_table[low]);
                int bl = qBlue(this->m_table[low]);

                int rh = qRed(this->m_table[high]);
                int gh = qGreen(this->m_table[high]);
                int bh = qBlue(this->m_table[high]);

                int minGraySec = low * j + minGray;
                int maxGraySec = high * j + minGray;

                float k = (float) (gray - minGraySec) / (maxGraySec - minGraySec);

                int r = k * (rh - rl) + rl;
                int g = k * (gh - gl) + gl;
                int b = k * (bh - bl) + bl;

                color = qRgb(r, g, b);
            }
            else
                color = this->m_table[low];

            table[gray] = color;
        }
    }
    else
        for (int i = 0; i < 256; i++)
            table[i] = this->m_table[0];

    for (int i = 0; i < videoArea; i++)
        destBits[i] = table[grayBuffer[i]];

    delete table;
    delete grayBuffer;

    QbBufferPtr oBuffer(new char[oFrame.byteCount()]);
    memcpy(oBuffer.data(), oFrame.constBits(), oFrame.byteCount());

    QbCaps caps(packet.caps());
    caps.setProperty("format", "bgra");
    caps.setProperty("width", oFrame.width());
    caps.setProperty("height", oFrame.height());

    QbPacket oPacket(caps,
                     oBuffer,
                     oFrame.byteCount());

    oPacket.setPts(packet.pts());
    oPacket.setTimeBase(packet.timeBase());
    oPacket.setIndex(packet.index());

    emit this->oStream(oPacket);
}
