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

#include <QtMath>
#include <QPainter>
#include <QQmlContext>
#include <akvideopacket.h>

#include "dizzyelement.h"

class DizzyElementPrivate
{
    public:
        qreal m_speed {5.0};
        qreal m_zoomRate {0.02};
        qreal m_strength {0.75};
        QImage m_prevFrame;

        void setParams(int *dx, int *dy,
                       int *sx, int *sy,
                       int width, int height,
                       qreal phase, qreal zoomRate);
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

AkPacket DizzyElement::iStream(const AkPacket &packet)
{
    AkVideoPacket videoPacket(packet);
    auto src = videoPacket.toImage();

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());
    oFrame.fill(0);

    if (this->d->m_prevFrame.isNull()) {
        this->d->m_prevFrame = QImage(src.size(), src.format());
        this->d->m_prevFrame.fill(0);
    }

    qreal pts = 2 * M_PI * packet.pts() * packet.timeBase().value()
                / this->d->m_speed;

    qreal angle = (2 * M_PI / 180) * sin(pts) + (M_PI / 180) * sin(pts + 2.5);
    qreal scale = 1.0 + this->d->m_zoomRate;

    QTransform transform;
    transform.scale(scale, scale);
    transform.rotateRadians(angle);
    this->d->m_prevFrame = this->d->m_prevFrame.transformed(transform);

    QRect rect(this->d->m_prevFrame.rect());
    rect.moveCenter(oFrame.rect().center());

    QPainter painter;
    painter.begin(&oFrame);
    painter.drawImage(rect, this->d->m_prevFrame);
    painter.setOpacity(1.0 - this->d->m_strength);
    painter.drawImage(0, 0, src);
    painter.end();

    this->d->m_prevFrame = oFrame;

    auto oPacket = AkVideoPacket::fromImage(oFrame, videoPacket).toPacket();
    akSend(oPacket)
}

#include "moc_dizzyelement.cpp"
