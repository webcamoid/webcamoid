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

#include "diceelement.h"

DiceElement::DiceElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    this->resetCubeBits();
}

QObject *DiceElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Dice/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Dice", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
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

void DiceElement::setCubeBits(int cubeBits)
{
    cubeBits = qBound(0, cubeBits, 5);

    if (cubeBits != this->m_cubeBits) {
        this->m_cubeBits = cubeBits;
        emit this->cubeBitsChanged();
    }
}

void DiceElement::resetCubeBits()
{
    this->setCubeBits(4);
}

QbPacket DiceElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame = QImage(src.size(), src.format());

    quint32 *srcBits = (quint32 *) src.bits();
    quint32 *destBits = (quint32 *) oFrame.bits();

    static int cubeBits = this->m_cubeBits;
    static QSize curSize;

    if (this->m_cubeBits != cubeBits
        || src.size() != curSize) {
        cubeBits = this->m_cubeBits;
        curSize = src.size();
        this->m_diceMap = this->makeDiceMap(src.size(), this->m_cubeBits);
    }

    int mapWidth = src.width() >> cubeBits;
    int mapHeight = src.height() >> cubeBits;
    int cubeSize = 1 << cubeBits;
    int mapI = 0;

    for (int mapY = 0; mapY < mapHeight; mapY++)
        for (int mapX = 0; mapX < mapWidth; mapX++) {
            int base = (mapY << cubeBits) * src.width() + (mapX << cubeBits);

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

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
