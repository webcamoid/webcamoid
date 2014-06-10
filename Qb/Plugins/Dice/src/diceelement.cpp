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

#include "diceelement.h"

DiceElement::DiceElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->m_curCubeBits = -1;
    this->resetCubeBits();
}

int DiceElement::cubeBits() const
{
    return this->m_cubeBits;
}

QByteArray DiceElement::makeDiceMap(const QSize &size, int cubeBits) const
{
    QByteArray diceMap;

    int mapWidth = size.width() >> cubeBits;
    int mapHeight = size.height() >> cubeBits;

    diceMap.resize(mapWidth * mapHeight);
    int i = 0;

    for (int y = 0; y < mapHeight; y++)
        for (int x = 0; x < mapWidth; x++) {
            diceMap.data()[i] = (qrand() >> 24) & 0x03;
            i++;
        }

    return diceMap;
}

void DiceElement::init(const QSize &size)
{
    this->m_diceMap = this->makeDiceMap(size, this->m_cubeBits);
}

void DiceElement::setCubeBits(int cubeBits)
{
    this->m_cubeBits = qBound(0, cubeBits, 5);
}

void DiceElement::resetCubeBits()
{
    this->setCubeBits(4);
}

void DiceElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void DiceElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void DiceElement::processFrame(const QbPacket &packet)
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

    if (this->m_cubeBits != this->m_curCubeBits
        || src.size() != this->m_curSize) {
        this->init(src.size());
        this->m_curCubeBits = this->m_cubeBits;
        this->m_curSize = src.size();
    }

    int mapWidth = src.width() >> this->m_cubeBits;
    int mapHeight = src.height() >> this->m_cubeBits;
    int cubeSize = 1 << this->m_cubeBits;
    int mapI = 0;

    for (int mapY = 0; mapY < mapHeight; mapY++)
        for (int mapX = 0; mapX < mapWidth; mapX++) {
            int base = (mapY << this->m_cubeBits) * src.width() + (mapX << this->m_cubeBits);

            if (this->m_diceMap.data()[mapI] == DICEDIR_UP) {
                for (int dy = 0; dy < cubeSize; dy++) {
                    int i = base + dy * src.width();

                    for (int dx = 0; dx < cubeSize; dx++) {
                        destBits[i] = srcBits[i];
                        i++;
                    }
                }
            } else if (this->m_diceMap[mapI] == DICEDIR_LEFT) {
                for (int dy = 0; dy < cubeSize; dy++) {
                    int i = base + dy * src.width();

                    for (int dx = 0; dx < cubeSize; dx++) {
                        int di = base + (dx * src.width()) + (cubeSize - dy - 1);
                        destBits[di] = srcBits[i];
                        i++;
                    }
                }
            } else if (this->m_diceMap[mapI] == DICEDIR_DOWN) {
                for (int dy = 0; dy < cubeSize; dy++) {
                    int di = base + dy * src.width();
                    int i = base + (cubeSize - dy - 1) * src.width() + cubeSize;

                    for (int dx = 0; dx < cubeSize; dx++) {
                        i--;
                        destBits[di] = srcBits[i];
                        di++;
                    }
                }
            } else if (this->m_diceMap[mapI] == DICEDIR_RIGHT) {
                for (int dy = 0; dy < cubeSize; dy++) {
                    int i = base + (dy * src.width());

                    for (int dx = 0; dx < cubeSize; dx++) {
                        int di = base + dy + (cubeSize - dx - 1) * src.width();
                        destBits[di] = srcBits[i];
                        i++;
                    }
                }
            }

            mapI++;
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
