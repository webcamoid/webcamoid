/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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
 * Web-Site: http://webcamoid.github.io/
 */

#include <QImage>
#include <QPainter>
#include <QQmlContext>
#include <QMutex>
#include <QtMath>
#include <akutils.h>
#include <akpacket.h>

#include "diceelement.h"

class DiceElementPrivate
{
    public:
        int m_diceSize {24};
        QMutex m_mutex;
        QImage m_diceMap;
        QSize m_frameSize;
};

DiceElement::DiceElement(): AkElement()
{
    this->d = new DiceElementPrivate;
}

DiceElement::~DiceElement()
{
    delete this->d;
}

int DiceElement::diceSize() const
{
    return this->d->m_diceSize;
}

QString DiceElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Dice/share/qml/main.qml");
}

void DiceElement::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Dice", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void DiceElement::setDiceSize(int diceSize)
{
    if (this->d->m_diceSize == diceSize)
        return;

    this->d->m_diceSize = diceSize;
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

    static int diceSize = this->d->m_diceSize;

    if (src.size() != this->d->m_frameSize
        || this->d->m_diceSize != diceSize) {
        diceSize = this->d->m_diceSize;
        this->d->m_frameSize = src.size();
        this->updateDiceMap();
        emit this->frameSizeChanged(this->d->m_frameSize);
    }

    QTransform rotateLeft;
    QTransform rotateRight;
    QTransform rotate180;

    rotateLeft.rotate(90);
    rotateRight.rotate(-90);
    rotate180.rotate(180);

    QPainter painter;
    painter.begin(&oFrame);

    for (int y = 0; y < this->d->m_diceMap.height(); y++) {
        auto diceLine = reinterpret_cast<const quint8 *>(this->d->m_diceMap.constScanLine(y));

        for (int x = 0; x < this->d->m_diceMap.width(); x++) {
            int xp = this->d->m_diceSize * x;
            int yp = this->d->m_diceSize * y;
            QImage dice = src.copy(xp, yp,
                                   this->d->m_diceSize, this->d->m_diceSize);
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
    int width = qCeil(this->d->m_frameSize.width() / qreal(this->d->m_diceSize));
    int height = qCeil(this->d->m_frameSize.height() / qreal(this->d->m_diceSize));

    QImage diceMap(width, height, QImage::Format_Grayscale8);

    for (int y = 0; y < diceMap.height(); y++) {
        auto oLine = reinterpret_cast<quint8 *>(diceMap.scanLine(y));

        for (int x = 0; x < diceMap.width(); x++)
            oLine[x] = qrand() % 4;
    }

    this->d->m_diceMap = diceMap;
}

#include "moc_diceelement.cpp"
