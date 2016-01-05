/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include <QtMath>
#include <QPainter>

#include "diceelement.h"

DiceElement::DiceElement(): AkElement()
{
    this->m_diceSize = 24;
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

    if (!item) {
        delete context;

        return NULL;
    }

    context->setParent(item);

    return item;
}

int DiceElement::diceSize() const
{
    return this->m_diceSize;
}

void DiceElement::setDiceSize(int diceSize)
{
    if (this->m_diceSize == diceSize)
        return;

    this->m_diceSize = diceSize;
    emit this->diceSizeChanged(diceSize);
}

void DiceElement::resetDiceSize()
{
    this->setDiceSize(24);
}

AkPacket DiceElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame = src.copy();

    static int diceSize = this->m_diceSize;

    if (src.size() != this->m_frameSize
        || this->m_diceSize != diceSize) {
        diceSize = this->m_diceSize;
        this->m_frameSize = src.size();
        this->updateDiceMap();
        emit this->frameSizeChanged(this->m_frameSize);
    }

    QTransform rotateLeft;
    QTransform rotateRight;
    QTransform rotate180;

    rotateLeft.rotate(90);
    rotateRight.rotate(-90);
    rotate180.rotate(180);

    QPainter painter;
    painter.begin(&oFrame);

    for (int y = 0; y < this->m_diceMap.height(); y++) {
        const quint8 *diceLine = (const quint8 *) this->m_diceMap.constScanLine(y);

        for (int x = 0; x < this->m_diceMap.width(); x++) {
            int xp = this->m_diceSize * x;
            int yp = this->m_diceSize * y;
            QImage dice = src.copy(xp, yp, this->m_diceSize, this->m_diceSize);
            quint8 direction = diceLine[x];

            if (direction == 0)
                dice = dice.transformed(rotateLeft);
            else if (direction == 1)
                dice = dice.transformed(rotateRight);
            else if (direction == 2)
                dice = dice.transformed(rotate180);

            painter.drawImage(xp, yp, dice);
        }
    }

    painter.end();

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}

void DiceElement::updateDiceMap()
{
    int width = qCeil(this->m_frameSize.width() / qreal(this->m_diceSize));
    int height = qCeil(this->m_frameSize.height() / qreal(this->m_diceSize));

    QImage diceMap(width, height, QImage::Format_Grayscale8);

    for (int y = 0; y < diceMap.height(); y++) {
        quint8 *oLine = (quint8 *) diceMap.scanLine(y);

        for (int x = 0; x < diceMap.width(); x++)
            oLine[x] = qrand() % 4;
    }

    this->m_diceMap = diceMap;
}
