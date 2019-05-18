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

#include <QImage>
#include <QQmlContext>
#include <QtMath>
#include <akpacket.h>
#include <akvideopacket.h>

#include "temperatureelement.h"

class TemperatureElementPrivate
{
    public:
        qreal m_temperature {6500.0};
        qreal m_kr {0.0};
        qreal m_kg {0.0};
        qreal m_kb {0.0};

        inline void colorFromTemperature(qreal temperature,
                                         qreal *r,
                                         qreal *g,
                                         qreal *b);
};

TemperatureElement::TemperatureElement(): AkElement()
{
    this->d = new TemperatureElementPrivate;
    this->d->colorFromTemperature(this->d->m_temperature,
                                  &this->d->m_kr,
                                  &this->d->m_kg,
                                  &this->d->m_kb);
}

TemperatureElement::~TemperatureElement()
{
    delete this->d;
}

qreal TemperatureElement::temperature() const
{
    return this->d->m_temperature;
}

QString TemperatureElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Temperature/share/qml/main.qml");
}

void TemperatureElement::controlInterfaceConfigure(QQmlContext *context,
                                                   const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Temperature", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket TemperatureElement::iVideoStream(const AkVideoPacket &packet)
{
    auto src = packet.toImage();

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());

    for (int y = 0; y < src.height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        auto destLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            int r = int(this->d->m_kr * qRed(srcLine[x]));
            int g = int(this->d->m_kg * qGreen(srcLine[x]));
            int b = int(this->d->m_kb * qBlue(srcLine[x]));

            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);

            destLine[x] = qRgba(r, g, b, qAlpha(srcLine[x]));
        }
    }

    auto oPacket = AkVideoPacket::fromImage(oFrame, packet);
    akSend(oPacket)
}

void TemperatureElement::setTemperature(qreal temperature)
{
    if (qFuzzyCompare(this->d->m_temperature, temperature))
        return;

    this->d->m_temperature = temperature;
    this->d->colorFromTemperature(temperature,
                                  &this->d->m_kr,
                                  &this->d->m_kg,
                                  &this->d->m_kb);
    emit this->temperatureChanged(temperature);
}

void TemperatureElement::resetTemperature()
{
    this->setTemperature(6500);
}

void TemperatureElementPrivate::colorFromTemperature(qreal temperature,
                                                     qreal *r,
                                                     qreal *g,
                                                     qreal *b)
{
    // This algorithm was taken from here:
    // http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/

    // Temperature must fall between 1000 and 40000 degrees
    temperature = qBound<qreal>(1000.0, temperature, 40000.0);

    // All calculations require temperature / 100, so only do the conversion once
    temperature /= 100.0;

    if (temperature <= 66.0)
        *r = 1;
    else
        *r = 1.2929362 * qPow(temperature - 60.0, -0.1332047592);

    if (temperature <= 66.0)
        *g = 0.39008158 * qLn(temperature) - 0.63184144;
    else
        *g = 1.1298909 * qPow(temperature - 60, -0.0755148492);

    if (temperature >= 66)
        *b = 1;
    else if (temperature <= 19)
        *b = 0;
    else
        *b = 0.54320679 * qLn(temperature - 10) - 1.1962541;
}

#include "moc_temperatureelement.cpp"
