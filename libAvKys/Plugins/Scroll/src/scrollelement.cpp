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

#include <QTime>
#include <QPainter>
#include <QQmlContext>
#include <akutils.h>
#include <akpacket.h>

#include "scrollelement.h"

class ScrollElementPrivate
{
    public:
        qreal m_speed;
        qreal m_noise;
        qreal m_offset;
        QSize m_curSize;

        ScrollElementPrivate():
            m_speed(0.25),
            m_noise(0.1),
            m_offset(0.0)
        {
        }
};

ScrollElement::ScrollElement(): AkElement()
{
    this->d = new ScrollElementPrivate;

    qsrand(uint(QTime::currentTime().msec()));
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

QImage ScrollElement::generateNoise(const QSize &size, qreal persent)
{
    QImage noise = QImage(size, QImage::Format_ARGB32);
    noise.fill(0);

    int peper = int(persent * size.width() * size.height());

    for (int i = 0; i < peper; i++) {
        int gray = qrand() % 256;
        int alpha = qrand() % 256;
        int x = qrand() % noise.width();
        int y = qrand() % noise.height();
        noise.setPixel(x, y, qRgba(gray, gray, gray, alpha));
    }

    return noise;
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

AkPacket ScrollElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

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
    QImage noise = this->generateNoise(oFrame.size(), this->d->m_noise);
    painter.drawImage(0, 0, noise);
    painter.end();

    this->d->m_offset += this->d->m_speed * oFrame.height();

    if (this->d->m_offset >= qreal(src.height()))
        this->d->m_offset = 0.0;
    else if (this->d->m_offset < 0.0)
        this->d->m_offset = src.height();

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}

#include "moc_scrollelement.cpp"
