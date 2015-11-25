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

#include "temperatureelement.h"

TemperatureElement::TemperatureElement(): QbElement()
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

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Temperature", (QObject *) this);
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
    if (this->m_temperature == temperature)
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

QbPacket TemperatureElement::iStream(const QbPacket &packet)
{
    QImage src = QbUtils::packetToImage(packet);

    if (src.isNull())
        return QbPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    int videoArea = src.width() * src.height();
    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    for (int i = 0; i < videoArea; i++) {
        int r = this->m_kr * qRed(srcBits[i]);
        int g = this->m_kg * qGreen(srcBits[i]);
        int b = this->m_kb * qBlue(srcBits[i]);

        r = qBound(0, r, 255);
        g = qBound(0, g, 255);
        b = qBound(0, b, 255);

        destBits[i] = qRgba(r, g, b, qAlpha(srcBits[i]));
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, packet);
    qbSend(oPacket)
}
