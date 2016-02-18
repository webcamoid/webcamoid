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

#include <QPainter>
#include <QTime>

#include "scrollelement.h"

ScrollElement::ScrollElement(): AkElement()
{
    this->m_speed = 0.05;
    this->m_noise = 0.1;
    this->m_offset = 0.0;

    qsrand(QTime::currentTime().msec());
}

QObject *ScrollElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Scroll/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Scroll", (QObject *) this);
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

qreal ScrollElement::speed() const
{
    return this->m_speed;
}

qreal ScrollElement::noise() const
{
    return this->m_noise;
}

QImage ScrollElement::generateNoise(const QSize &size, qreal persent)
{
    QImage noise = QImage(size, QImage::Format_ARGB32);
    noise.fill(0);

    int peper = persent * size.width() * size.height();

    for (int i = 0; i < peper; i++) {
        int gray = qrand() % 256;
        int alpha = qrand() % 256;
        int x = qrand() % noise.width();
        int y = qrand() % noise.height();
        noise.setPixel(x, y, qRgba(gray, gray, gray, alpha));
    }

    return noise;
}

void ScrollElement::setSpeed(qreal speed)
{
    if (speed == this->m_speed)
        return;

    this->m_speed = speed;
    emit this->speedChanged(speed);
}

void ScrollElement::setNoise(qreal noise)
{
    if (this->m_noise == noise)
        return;

    this->m_noise = noise;
    emit this->noiseChanged(noise);
}

void ScrollElement::resetSpeed()
{
    this->setSpeed(0.05);
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

    if (src.size() != this->m_curSize) {
        this->m_offset = 0.0;
        this->m_curSize = src.size();
    }

    int offset = this->m_offset;

    memcpy(oFrame.scanLine(0),
           src.constScanLine(src.height() - offset - 1),
           src.bytesPerLine() * offset);

    memcpy(oFrame.scanLine(offset),
           src.constScanLine(0),
           src.bytesPerLine() * (src.height() - offset));

    QPainter painter;
    painter.begin(&oFrame);
    QImage noise = this->generateNoise(oFrame.size(), this->m_noise);
    painter.drawImage(0, 0, noise);
    painter.end();

    this->m_offset += this->m_speed * oFrame.height();

    if (this->m_offset >= qreal(src.height()))
        this->m_offset = 0.0;
    else if (this->m_offset < 0.0)
        this->m_offset = src.height();

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
