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

#include <QPainter>
#include <QQmlContext>
#include <QRandomGenerator>
#include <QTime>
#include <akpacket.h>
#include <akvideopacket.h>

#include "scrollelement.h"

class ScrollElementPrivate
{
    public:
        qreal m_speed {0.25};
        qreal m_noise {0.1};
        qreal m_offset {0.0};
        QSize m_curSize;

        QImage generateNoise(const QSize &size, qreal persent) const;
};

ScrollElement::ScrollElement(): AkElement()
{
    this->d = new ScrollElementPrivate;
}

ScrollElement::~ScrollElement()
{
    delete this->d;
}

qreal ScrollElement::speed() const
{
    return this->d->m_speed;
}

qreal ScrollElement::noise() const
{
    return this->d->m_noise;
}

QString ScrollElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Scroll/share/qml/main.qml");
}

void ScrollElement::controlInterfaceConfigure(QQmlContext *context,
                                              const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Scroll", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket ScrollElement::iVideoStream(const AkVideoPacket &packet)
{
    auto src = packet.toImage();

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame = QImage(src.size(), src.format());

    if (src.size() != this->d->m_curSize) {
        this->d->m_offset = 0.0;
        this->d->m_curSize = src.size();
    }

    int offset = int(this->d->m_offset);

    memcpy(oFrame.scanLine(0),
           src.constScanLine(src.height() - offset - 1),
           size_t(src.bytesPerLine() * offset));

    memcpy(oFrame.scanLine(offset),
           src.constScanLine(0),
           size_t(src.bytesPerLine() * (src.height() - offset)));

    QPainter painter;
    painter.begin(&oFrame);
    QImage noise = this->d->generateNoise(oFrame.size(), this->d->m_noise);
    painter.drawImage(0, 0, noise);
    painter.end();

    this->d->m_offset += this->d->m_speed * oFrame.height();

    if (this->d->m_offset >= qreal(src.height()))
        this->d->m_offset = 0.0;
    else if (this->d->m_offset < 0.0)
        this->d->m_offset = src.height();

    auto oPacket = AkVideoPacket::fromImage(oFrame, packet);
    akSend(oPacket)
}

void ScrollElement::setSpeed(qreal speed)
{
    if (qFuzzyCompare(speed, this->d->m_speed))
        return;

    this->d->m_speed = speed;
    emit this->speedChanged(speed);
}

void ScrollElement::setNoise(qreal noise)
{
    if (qFuzzyCompare(this->d->m_noise, noise))
        return;

    this->d->m_noise = noise;
    emit this->noiseChanged(noise);
}

void ScrollElement::resetSpeed()
{
    this->setSpeed(0.25);
}

void ScrollElement::resetNoise()
{
    this->setNoise(0.1);
}

QImage ScrollElementPrivate::generateNoise(const QSize &size, qreal persent) const
{
    QImage noise(size, QImage::Format_ARGB32);
    noise.fill(0);

    auto peper = qRound(persent * size.width() * size.height());

    for (int i = 0; i < peper; i++) {
        int gray = QRandomGenerator::global()->bounded(256);
        int alpha = QRandomGenerator::global()->bounded(256);
        int x = QRandomGenerator::global()->bounded(noise.width());
        int y = QRandomGenerator::global()->bounded(noise.height());
        noise.setPixel(x, y, qRgba(gray, gray, gray, alpha));
    }

    return noise;
}

#include "moc_scrollelement.cpp"
