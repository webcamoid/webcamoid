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
#include <QRandomGenerator>
#include <QSize>
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideomixer.h>
#include <akvideopacket.h>

#include "dice.h"

class DicePrivate
{
    public:
        Dice *self {nullptr};
        QString m_description {QObject::tr("Dice")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        AkVideoPacket m_diceMap;
        QSize m_frameSize;
        int m_diceSize {24};
        int m_currentDiceSize {0};
        IAkVideoFilterPtr m_rotate90 {akPluginManager->create<IAkVideoFilter>("VideoFilter/Rotate")};
        IAkVideoFilterPtr m_rotate180 {akPluginManager->create<IAkVideoFilter>("VideoFilter/Rotate")};
        IAkVideoFilterPtr m_rotate270 {akPluginManager->create<IAkVideoFilter>("VideoFilter/Rotate")};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        AkVideoMixer m_videoMixer;

        explicit DicePrivate(Dice *self);
        void updateDiceMap(const QSize &frameSize, int diceSize);
};

Dice::Dice(QObject *parent):
      QObject(parent)
{
    this->d = new DicePrivate(this);

    this->d->m_rotate90->property<IAkPropertyDouble>("angle")->setValue(90.0);
    this->d->m_rotate180->property<IAkPropertyDouble>("angle")->setValue(180.0);
    this->d->m_rotate270->property<IAkPropertyDouble>("angle")->setValue(270.0);
    this->d->m_videoMixer.setFlags(AkVideoMixer::MixerFlagLightweightCache
                                   | AkVideoMixer::MixerFlagForceBlit);
}

Dice::~Dice()
{
    delete this->d;
}

QString Dice::description() const
{
    return this->d->m_description;
}

AkElementType Dice::type() const
{
    return this->d->m_type;
}

AkElementCategory Dice::category() const
{
    return this->d->m_category;
}

void *Dice::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Dice::create(const QString &id)
{
    Q_UNUSED(id)

    return new Dice;
}

int Dice::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Dice",
                            this->d->m_description,
                            pluginPath,
                            QStringList(),
                            this->d->m_type,
                            this->d->m_category,
                            0,
                            this);
    akPluginManager->registerPlugin(pluginInfo);

    return 0;
}

int Dice::diceSize() const
{
    return this->d->m_diceSize;
}

void Dice::deleteThis(void *userData) const
{
    delete reinterpret_cast<Dice *>(userData);
}

QString Dice::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Dice/share/qml/main.qml");
}

void Dice::controlInterfaceConfigure(QQmlContext *context,
                                     const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Dice", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Dice::iVideoStream(const AkVideoPacket &packet)
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
        this->oStream(dst);

    return dst;
}

void Dice::setDiceSize(int diceSize)
{
    if (this->d->m_diceSize == diceSize)
        return;

    this->d->m_diceSize = diceSize;
    emit this->diceSizeChanged(diceSize);
}

void Dice::resetDiceSize()
{
    this->setDiceSize(24);
}

DicePrivate::DicePrivate(Dice *self):
    self(self)
{

}

void DicePrivate::updateDiceMap(const QSize &frameSize, int diceSize)
{
    qreal diceSize_ = qMax(1, diceSize);

    int width = qCeil(frameSize.width() / diceSize_);
    int height = qCeil(frameSize.height() / diceSize_);

    AkVideoPacket diceMap({AkVideoCaps::Format_y8, width, height, {}});

    for (int y = 0; y < diceMap.caps().height(); y++) {
        auto oLine = diceMap.line(0, y);

        for (int x = 0; x < diceMap.caps().width(); x++)
            oLine[x] = quint8(QRandomGenerator::global()->bounded(4));
    }

    this->m_diceMap = diceMap;
}

#include "moc_dice.cpp"
