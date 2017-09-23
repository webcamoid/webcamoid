/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include "dizzyelement.h"

DizzyElement::DizzyElement(): AkElement()
{
    this->m_speed = 5.0;
    this->m_zoomRate = 0.02;
    this->m_strength = 0.75;
}

qreal DizzyElement::speed() const
{
    return this->m_speed;
}

qreal DizzyElement::zoomRate() const
{
    return this->m_zoomRate;
}

qreal DizzyElement::strength() const
{
    return this->m_strength;
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
    if (qFuzzyCompare(this->m_speed, speed))
        return;

    this->m_speed = speed;
    emit this->speedChanged(speed);
}

void DizzyElement::setZoomRate(qreal zoomRate)
{
    if (qFuzzyCompare(this->m_zoomRate, zoomRate))
        return;

    this->m_zoomRate = zoomRate;
    emit this->zoomRateChanged(zoomRate);
}

void DizzyElement::setStrength(qreal strength)
{
    if (qFuzzyCompare(this->m_strength, strength))
        return;

    this->m_strength = strength;
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
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());
    oFrame.fill(0);

    if (this->m_prevFrame.isNull()) {
        this->m_prevFrame = QImage(src.size(), src.format());
        this->m_prevFrame.fill(0);
    }

    qreal pts = 2 * M_PI * packet.pts() * packet.timeBase().value()
                / this->m_speed;

    qreal angle = (2 * M_PI / 180) * sin(pts) + (M_PI / 180) * sin(pts + 2.5);
    qreal scale = 1.0 + this->m_zoomRate;

    QTransform transform;
    transform.scale(scale, scale);
    transform.rotateRadians(angle);
    this->m_prevFrame = this->m_prevFrame.transformed(transform);

    QRect rect(this->m_prevFrame.rect());
    rect.moveCenter(oFrame.rect().center());

    QPainter painter;
    painter.begin(&oFrame);
    painter.drawImage(rect, this->m_prevFrame);
    painter.setOpacity(1.0 - this->m_strength);
    painter.drawImage(0, 0, src);
    painter.end();

    this->m_prevFrame = oFrame;

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
