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

#include <QRect>
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideomixer.h>
#include <akvideopacket.h>

#include "dizzy.h"

class DizzyPrivate
{
    public:
        Dizzy *self {nullptr};
        QString m_description {QObject::tr("Stoned")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        qreal m_speed {5.0};
        qreal m_zoomRate {0.02};
        qreal m_strength {0.75};
        AkVideoPacket m_prevFrame;
        IAkVideoFilterPtr m_transform {akPluginManager->create<IAkVideoFilter>("VideoFilter/MatrixTransform")};
        IAkVideoFilterPtr m_opacity {akPluginManager->create<IAkVideoFilter>("VideoFilter/Opacity")};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        AkVideoMixer m_videoMixer;

        explicit DizzyPrivate(Dizzy *self);
};

Dizzy::Dizzy(QObject *parent):
      QObject(parent)
{
    this->d = new DizzyPrivate(this);
}

Dizzy::~Dizzy()
{
    delete this->d;
}

QString Dizzy::description() const
{
    return this->d->m_description;
}

AkElementType Dizzy::type() const
{
    return this->d->m_type;
}

AkElementCategory Dizzy::category() const
{
    return this->d->m_category;
}

void *Dizzy::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Dizzy::create(const QString &id)
{
    Q_UNUSED(id)

    return new Dizzy;
}

int Dizzy::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Dizzy",
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

qreal Dizzy::speed() const
{
    return this->d->m_speed;
}

qreal Dizzy::zoomRate() const
{
    return this->d->m_zoomRate;
}

qreal Dizzy::strength() const
{
    return this->d->m_strength;
}

void Dizzy::deleteThis(void *userData) const
{
    delete reinterpret_cast<Dizzy *>(userData);
}

QString Dizzy::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Dizzy/share/qml/main.qml");
}

void Dizzy::controlInterfaceConfigure(QQmlContext *context,
                                      const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Dizzy", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Dizzy::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps(), true);
    dst.copyMetadata(src);

    if (!this->d->m_prevFrame)
        this->d->m_prevFrame = AkVideoPacket(src.caps(), true);

    qreal pts = 2.0 * M_PI * packet.pts() * packet.timeBase().value()
                / this->d->m_speed;
    qreal angle = (2.0 * qSin(pts) + qSin(pts + 2.5)) * M_PI / 180.0;
    qreal scale = 1.0 + this->d->m_zoomRate;
    QVector<qreal> kernel {
        scale * qCos(angle), -scale * qSin(angle), 0,
        scale * qSin(angle),  scale * qCos(angle), 0,
    };

    this->d->m_transform->property<IAkPropertyDoubleList>("kernel")->setValue(kernel);
    AkVideoPacket transformedFrame =
        this->d->m_transform->iStream(this->d->m_prevFrame);

    auto opacity = qBound(0.0, 1.0 - this->d->m_strength, 1.0);;
    this->d->m_opacity->property<IAkPropertyDouble>("opacity")->setValue(opacity);
    auto topFrame = this->d->m_opacity->iStream(src);

    QRect rect(0,
               0,
               transformedFrame.caps().width(),
               transformedFrame.caps().height());
    rect.moveCenter({dst.caps().width() >> 1,
                     dst.caps().height() >> 1});

    this->d->m_videoMixer.begin(&dst);
    this->d->m_videoMixer.draw(rect.x(),
                               rect.y(),
                               transformedFrame);
    this->d->m_videoMixer.draw(topFrame);
    this->d->m_videoMixer.end();

    this->d->m_prevFrame = dst;

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void Dizzy::setSpeed(qreal speed)
{
    if (qFuzzyCompare(this->d->m_speed, speed))
        return;

    this->d->m_speed = speed;
    emit this->speedChanged(speed);
}

void Dizzy::setZoomRate(qreal zoomRate)
{
    if (qFuzzyCompare(this->d->m_zoomRate, zoomRate))
        return;

    this->d->m_zoomRate = zoomRate;
    emit this->zoomRateChanged(zoomRate);
}

void Dizzy::setStrength(qreal strength)
{
    if (qFuzzyCompare(this->d->m_strength, strength))
        return;

    this->d->m_strength = strength;
    emit this->strengthChanged(strength);
}

void Dizzy::resetSpeed()
{
    this->setSpeed(5.0);
}

void Dizzy::resetZoomRate()
{
    this->setZoomRate(0.02);
}

void Dizzy::resetStrength()
{
    this->setStrength(0.15);
}

DizzyPrivate::DizzyPrivate(Dizzy *self):
    self(self)
{

}

#include "moc_dizzy.cpp"
