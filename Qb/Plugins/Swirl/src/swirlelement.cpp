/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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

#include "swirlelement.h"

SwirlElement::SwirlElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetDegrees();
}

QObject *SwirlElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Swirl/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Swirl", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

qreal SwirlElement::degrees() const
{
    return this->m_degrees;
}

void SwirlElement::setDegrees(qreal degrees)
{
    if (degrees != this->m_degrees) {
        this->m_degrees = degrees;
        emit this->degreesChanged();
    }
}

void SwirlElement::resetDegrees()
{
    this->setDegrees(0);
}

QbPacket SwirlElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame(src.size(), src.format());

    qreal xScale = 1.0;
    qreal yScale = 1.0;
    qreal xCenter = src.width() >> 1;
    qreal yCenter = src.height() >> 1;
    qreal radius = qMax(xCenter, yCenter);

    if (src.width() > src.height())
        yScale = (qreal) src.width() / src.height();
    else if (src.width() < src.height())
        xScale = (qreal) src.height() / src.width();

    qreal degrees = M_PI * this->m_degrees / 180.0;

    for (int y = 0; y < src.height(); y++) {
        QRgb *srcBits = (QRgb *) src.scanLine(y);
        QRgb *destBits = (QRgb *) oFrame.scanLine(y);
        qreal yDistance = yScale * (y - yCenter);

        for (int x = 0; x < src.width(); x++) {
            qreal xDistance = xScale * (x - xCenter);
            qreal distance = xDistance * xDistance + yDistance * yDistance;

            if (distance >= (radius * radius))
                *destBits = srcBits[x];
            else {
                qreal factor = 1.0 - sqrt(distance) / radius;
                qreal sine = sin(degrees * factor * factor);
                qreal cosine = cos(degrees * factor * factor);

                *destBits = this->interpolate(src,
                                          (cosine * xDistance - sine * yDistance) / xScale + xCenter,
                                          (sine * xDistance + cosine * yDistance) / yScale + yCenter);
            }

            destBits++;
        }
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
