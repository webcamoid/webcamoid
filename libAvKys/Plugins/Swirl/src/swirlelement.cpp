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

#include "swirlelement.h"

SwirlElement::SwirlElement(): AkElement()
{
    this->m_degrees = 0;
}

QObject *SwirlElement::controlInterface(QQmlEngine *engine,
                                        const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine,
                            QUrl(QStringLiteral("qrc:/Swirl/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Swirl", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
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

qreal SwirlElement::degrees() const
{
    return this->m_degrees;
}

void SwirlElement::setDegrees(qreal degrees)
{
    if (qFuzzyCompare(this->m_degrees, degrees))
        return;

    this->m_degrees = degrees;
    emit this->degreesChanged(degrees);
}

void SwirlElement::resetDegrees()
{
    this->setDegrees(0);
}

AkPacket SwirlElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());

    qreal xScale = 1.0;
    qreal yScale = 1.0;
    qreal xCenter = src.width() >> 1;
    qreal yCenter = src.height() >> 1;
    qreal radius = qMax(xCenter, yCenter);

    if (src.width() > src.height())
        yScale = qreal(src.width() / src.height());
    else if (src.width() < src.height())
        xScale = qreal(src.height() / src.width());

    qreal degrees = M_PI * this->m_degrees / 180.0;

    for (int y = 0; y < src.height(); y++) {
        const QRgb *iLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        QRgb *oLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));
        qreal yDistance = yScale * (y - yCenter);

        for (int x = 0; x < src.width(); x++) {
            qreal xDistance = xScale * (x - xCenter);
            qreal distance = xDistance * xDistance + yDistance * yDistance;

            if (distance >= radius * radius)
                oLine[x] = iLine[x];
            else {
                qreal factor = 1.0 - sqrt(distance) / radius;
                qreal sine = sin(degrees * factor * factor);
                qreal cosine = cos(degrees * factor * factor);

                int xp = int((cosine * xDistance - sine * yDistance) / xScale + xCenter);
                int yp = int((sine * xDistance + cosine * yDistance) / yScale + yCenter);

                if (!oFrame.rect().contains(xp, yp))
                    continue;

                const QRgb *line = reinterpret_cast<const QRgb *>(src.constScanLine(yp));
                oLine[x] = line[xp];
            }
        }
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
