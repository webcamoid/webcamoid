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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *     Copyright (c) 2012, Tanner Helland
 *     All rights reserved.
 *
 *     Redistribution and use in source and binary forms, with or without
 *     modification, are permitted provided that the following conditions are met:
 *
 *     1. Redistributions of source code must retain the above copyright notice, this
 *        list of conditions and the following disclaimer.
 *
 *     2. Redistributions in binary form must reproduce the above copyright notice,
 *        this list of conditions and the following disclaimer in the documentation
 *        and/or other materials provided with the distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *     AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *     IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *     FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *     DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *     SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *     OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *     OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <QMutex>
#include <QQmlContext>
#include <QtMath>
#include <qrgb.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "temperatureelement.h"
class TemperatureElementPrivate {
    public:
        qreal m_temperature {6500.0};
        quint8 m_tableR [256];
        quint8 m_tableG [256];
        quint8 m_tableB [256];
        QMutex m_mutex;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        inline void colorFromTemperature(qreal temperature,
                                         qreal *r,
                                         qreal *g,
                                         qreal *b) const;
        inline void updateTemperatureTable(qreal temperature);
};

TemperatureElement::TemperatureElement(): AkElement()
{
    this->d = new TemperatureElementPrivate;
    this->d->updateTemperatureTable(this->d->m_temperature);
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
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    this->d->m_mutex.lock();

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto destLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto &pixel = srcLine[x];
            destLine[x] = qRgba(this->d->m_tableR[qRed(pixel)],
                                this->d->m_tableG[qGreen(pixel)],
                                this->d->m_tableB[qBlue(pixel)],
                                qAlpha(pixel));
        }
    }

    this->d->m_mutex.unlock();

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void TemperatureElement::setTemperature(qreal temperature)
{
    if (qFuzzyCompare(this->d->m_temperature, temperature))
        return;

    this->d->m_temperature = temperature;
    this->d->m_mutex.lock();
    this->d->updateTemperatureTable(temperature);
    this->d->m_mutex.unlock();
    emit this->temperatureChanged(temperature);
}

void TemperatureElement::resetTemperature()
{
    this->setTemperature(6500);
}

void TemperatureElementPrivate::colorFromTemperature(qreal temperature,
                                                     qreal *r,
                                                     qreal *g,
                                                     qreal *b) const
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

void TemperatureElementPrivate::updateTemperatureTable(qreal temperature)
{
    qreal kr = 0.0;
    qreal kg = 0.0;
    qreal kb = 0.0;
    this->colorFromTemperature(temperature, &kr, &kg, &kb);

    for (int i = 0; i < 256; i++) {
        this->m_tableR[i] = qBound(0, qRound(kr * i), 255);
        this->m_tableG[i] = qBound(0, qRound(kg * i), 255);
        this->m_tableB[i] = qBound(0, qRound(kb * i), 255);
    }
}

#include "moc_temperatureelement.cpp"
