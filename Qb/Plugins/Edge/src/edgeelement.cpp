/* Webcamod, webcam capture plasmoid.
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

#include "edgeelement.h"

EdgeElement::EdgeElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));
}

void EdgeElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void EdgeElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void EdgeElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_RGB32);

    QImage oFrame = QImage(src.size(), src.format());

    quint32 *srcBits = (quint32 *) src.bits();
    quint32 *destBits = (quint32 *) oFrame.bits();

    int mapWidth = src.width() >> 2;
    int mapHeight = src.height() >> 2;
    int widthMargin = src.width() - (mapWidth << 2);
    QVector<qint32> map(mapWidth * mapHeight * 2, 0);

    srcBits += src.width() * 4 + 4;
    destBits += src.width() * 4 + 4;

    for (int y = 1; y < mapHeight - 1; y++) {
        for (int x = 1; x < mapWidth - 1; x++) {
            qint32 p = *srcBits;
            qint32 q = *(srcBits - 4);

            // difference between the current pixel and right neighbor.
            int r = ((int)(p & 0xff0000) - (int)(q & 0xff0000)) >> 16;
            int g = ((int)(p & 0x00ff00) - (int)(q & 0x00ff00)) >> 8;
            int b = ((int)(p & 0x0000ff) - (int)(q & 0x0000ff));

            r *= r; // Multiply itself and divide it with 16, instead of
            g *= g; // using abs().
            b *= b;

            r = r >> 5; // To lack the lower bit for saturated addition,
            g = g >> 5; // devide the value with 32, instead of 16. It is
            b = b >> 4; // same as `v2 &= 0xfefeff'

            if (r > 127)
                r = 127;

            if (g > 127)
                g = 127;

            if (b > 255)
                b = 255;

            qint32 v2 = (r << 17) | (g << 9) | b;

            // difference between the current pixel and upper neighbor.
            q = *(srcBits - src.width() * 4);

            r = ((int)(p & 0xff0000) - (int)(q & 0xff0000)) >> 16;
            g = ((int)(p & 0x00ff00) - (int)(q & 0x00ff00)) >> 8;
            b = ((int)(p & 0x0000ff) - (int)(q & 0x0000ff));

            r *= r;
            g *= g;
            b *= b;

            r = r >> 5;
            g = g >> 5;
            b = b >> 4;

            if (r > 127)
                r = 127;

            if (g > 127)
                g = 127;

            if (b > 255)
                b = 255;

            qint32 v3 = (r << 17) | (g << 9) | b;

            qint32 v0 = map[(y - 1) * mapWidth * 2 + x * 2];
            qint32 v1 = map[y * mapWidth * 2 + (x - 1) * 2 + 1];

            map[y * mapWidth * 2 + x * 2] = v2;
            map[y * mapWidth * 2 + x * 2 + 1] = v3;

            r = v0 + v1;
            g = r & 0x01010100;

            destBits[0] = r | (g - (g >> 8));

            r = v0 + v3;
            g = r & 0x01010100;

            destBits[1] = r | (g - (g >> 8));
            destBits[2] = v3;
            destBits[3] = v3;

            r = v2 + v1;
            g = r & 0x01010100;

            destBits[src.width()] = r | (g - (g >> 8));

            r = v2 + v3;
            g = r & 0x01010100;

            destBits[src.width() + 1] = r | (g - (g >> 8));
            destBits[src.width() + 2] = v3;
            destBits[src.width() + 3] = v3;
            destBits[src.width() * 2] = v2;
            destBits[src.width() * 2 + 1] = v2;
            destBits[src.width() * 3] = v2;
            destBits[src.width() * 3 + 1] = v2;

            srcBits += 4;
            destBits += 4;
        }

        srcBits += src.width() * 3 + 8 + widthMargin;
        destBits += src.width() * 3 + 8 + widthMargin;
    }

    QbBufferPtr oBuffer(new char[oFrame.byteCount()]);
    memcpy(oBuffer.data(), oFrame.constBits(), oFrame.byteCount());

    QbCaps caps(packet.caps());
    caps.setProperty("format", "bgr0");
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
