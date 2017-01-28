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

#include "temperatureelement.h"

TemperatureElement::TemperatureElement(): AkElement()
{
    this->m_temperature = 6500;
    this->colorFromTemperature(this->m_temperature,
                               &this->m_kr, &this->m_kg, &this->m_kb);
}

QObject *TemperatureElement::controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine,
                            QUrl(QStringLiteral("qrc:/Temperature/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Temperature", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
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

qreal TemperatureElement::temperature() const
{
    return this->m_temperature;
}

void TemperatureElement::setTemperature(qreal temperature)
{
    if (qFuzzyCompare(this->m_temperature, temperature))
        return;

    this->m_temperature = temperature;
    this->colorFromTemperature(temperature,
                               &this->m_kr, &this->m_kg, &this->m_kb);
    emit this->temperatureChanged(temperature);
}

void TemperatureElement::resetTemperature()
{
    this->setTemperature(6500);
}

AkPacket TemperatureElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());

    for (int y = 0; y < src.height(); y++) {
        const QRgb *srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        QRgb *destLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            int r = int(this->m_kr * qRed(srcLine[x]));
            int g = int(this->m_kg * qGreen(srcLine[x]));
            int b = int(this->m_kb * qBlue(srcLine[x]));

            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);

            destLine[x] = qRgba(r, g, b, qAlpha(srcLine[x]));
        }
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
