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

#include <QQmlContext>
#include <QRect>
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideomixer.h>
#include <akvideopacket.h>

#include "dizzyelement.h"

class DizzyElementPrivate
{
    public:
        qreal m_speed {5.0};
        qreal m_zoomRate {0.02};
        qreal m_strength {0.75};
        AkVideoPacket m_prevFrame;
        AkElementPtr m_transform {akPluginManager->create<AkElement>("VideoFilter/MatrixTransform")};
        AkElementPtr m_opacity {akPluginManager->create<AkElement>("VideoFilter/Opacity")};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        AkVideoMixer m_videoMixer;
};

DizzyElement::DizzyElement():
    AkElement()
{
    this->d = new DizzyElementPrivate;
}

DizzyElement::~DizzyElement()
{
    delete this->d;
}

qreal DizzyElement::speed() const
{
    return this->d->m_speed;
}

qreal DizzyElement::zoomRate() const
{
    return this->d->m_zoomRate;
}

qreal DizzyElement::strength() const
{
    return this->d->m_strength;
}

QString DizzyElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Dizzy/share/qml/main.qml");
}

void DizzyElement::controlInterfaceConfigure(QQmlContext *context,
                                             const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Dizzy", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket DizzyElement::iVideoStream(const AkVideoPacket &packet)
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
    QVariantList kernel {
        scale * qCos(angle), -scale * qSin(angle), 0,
        scale * qSin(angle),  scale * qCos(angle), 0,
    };
    this->d->m_transform->setProperty("kernel", kernel);
    AkVideoPacket transformedFrame =
            this->d->m_transform->iStream(this->d->m_prevFrame);

    auto opacity = qBound(0.0, 1.0 - this->d->m_strength, 1.0);;
    this->d->m_opacity->setProperty("opacity", opacity);
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

void DizzyElement::setSpeed(qreal speed)
{
    if (qFuzzyCompare(this->d->m_speed, speed))
        return;

    this->d->m_speed = speed;
    emit this->speedChanged(speed);
}

void DizzyElement::setZoomRate(qreal zoomRate)
{
    if (qFuzzyCompare(this->d->m_zoomRate, zoomRate))
        return;

    this->d->m_zoomRate = zoomRate;
    emit this->zoomRateChanged(zoomRate);
}

void DizzyElement::setStrength(qreal strength)
{
    if (qFuzzyCompare(this->d->m_strength, strength))
        return;

    this->d->m_strength = strength;
    emit this->strengthChanged(strength);
}

void DizzyElement::resetSpeed()
{
    this->setSpeed(5.0);
}

void DizzyElement::resetZoomRate()
{
    this->setZoomRate(0.02);
}

void DizzyElement::resetStrength()
{
    this->setStrength(0.15);
}

#include "moc_dizzyelement.cpp"
