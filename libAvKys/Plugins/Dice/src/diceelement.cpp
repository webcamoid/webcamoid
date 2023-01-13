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

#include <QMutex>
#include <QQmlContext>
#include <QRandomGenerator>
#include <QSize>
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideomixer.h>
#include <akvideopacket.h>

#include "diceelement.h"

class DiceElementPrivate
{
    public:
        AkVideoPacket m_diceMap;
        QSize m_frameSize;
        int m_diceSize {24};
        int m_currentDiceSize {0};
        AkElementPtr m_rotate90 {akPluginManager->create<AkElement>("VideoFilter/Rotate")};
        AkElementPtr m_rotate180 {akPluginManager->create<AkElement>("VideoFilter/Rotate")};
        AkElementPtr m_rotate270 {akPluginManager->create<AkElement>("VideoFilter/Rotate")};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        AkVideoMixer m_videoMixer;

        void updateDiceMap(const QSize &frameSize, int diceSize);
};

DiceElement::DiceElement(): AkElement()
{
    this->d = new DiceElementPrivate;
    this->d->m_rotate90->setProperty("angle", 90);
    this->d->m_rotate180->setProperty("angle", 180);
    this->d->m_rotate270->setProperty("angle", 270);
    this->d->m_videoMixer.setFlags(AkVideoMixer::MixerFlagLightweightCache
                                   | AkVideoMixer::MixerFlagForceBlit);
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

AkPacket DiceElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);
    QSize frameSize(src.caps().width(), src.caps().height());

    if (frameSize != this->d->m_frameSize
        || this->d->m_diceSize != this->d->m_currentDiceSize) {
        this->d->updateDiceMap(frameSize, this->d->m_diceSize);
        this->d->m_frameSize = frameSize;
        this->d->m_currentDiceSize = this->d->m_diceSize;
    }

    this->d->m_videoMixer.begin(&dst);

    for (int y = 0; y < this->d->m_diceMap.caps().height(); y++) {
        auto diceLine = this->d->m_diceMap.constLine(0, y);

        for (int x = 0; x < this->d->m_diceMap.caps().width(); x++) {
            int xp = this->d->m_currentDiceSize * x;
            int yp = this->d->m_currentDiceSize * y;
            auto dice = src.copy(xp,
                                 yp,
                                 this->d->m_currentDiceSize,
                                 this->d->m_currentDiceSize);

            switch (diceLine[x]) {
            case 0:
                dice = this->d->m_rotate90->iStream(dice);

                break;
            case 1:
                dice = this->d->m_rotate180->iStream(dice);

                break;
            case 2:
                dice = this->d->m_rotate270->iStream(dice);

                break;
            default:
                break;
            }

            this->d->m_videoMixer.draw(xp, yp, dice);
        }
    }

    this->d->m_videoMixer.end();

    if (dst)
        emit this->oStream(dst);

    return dst;
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

void DiceElementPrivate::updateDiceMap(const QSize &frameSize, int diceSize)
{
    qreal diceSize_ = qMax(1, diceSize);

    int width = qCeil(frameSize.width() / diceSize_);
    int height = qCeil(frameSize.height() / diceSize_);

    AkVideoPacket diceMap({AkVideoCaps::Format_gray8, width, height, {}});

    for (int y = 0; y < diceMap.caps().height(); y++) {
        auto oLine = diceMap.line(0, y);

        for (int x = 0; x < diceMap.caps().width(); x++)
            oLine[x] = quint8(QRandomGenerator::global()->bounded(4));
    }

    this->m_diceMap = diceMap;
}

#include "moc_diceelement.cpp"
