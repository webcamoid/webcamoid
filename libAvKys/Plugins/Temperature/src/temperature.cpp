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

#include <QMutex>
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "temperature.h"

class TemperaturePrivate
{
    public:
        Temperature *self {nullptr};
        QString m_description {QObject::tr("Temperature")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        qreal m_temperature {6500.0};
        quint8 m_tableR [256];
        quint8 m_tableG [256];
        quint8 m_tableB [256];
        QMutex m_mutex;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        explicit TemperaturePrivate(Temperature *self);
        inline void colorFromTemperature(qreal temperature,
                                         qreal *r,
                                         qreal *g,
                                         qreal *b) const;
        inline void updateTemperatureTable(qreal temperature);
};

Temperature::Temperature(QObject *parent):
      QObject(parent)
{
    this->d = new TemperaturePrivate(this);
}

Temperature::~Temperature()
{
    delete this->d;
}

QString Temperature::description() const
{
    return this->d->m_description;
}

AkElementType Temperature::type() const
{
    return this->d->m_type;
}

AkElementCategory Temperature::category() const
{
    return this->d->m_category;
}

void *Temperature::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Temperature::create(const QString &id)
{
    Q_UNUSED(id)

    return new Temperature;
}

int Temperature::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Temperature",
                            this->d->m_description,
                            pluginPath,
                            QStringList(),
                            this->d->m_type,
                            this->d->m_category,
                            0,
                            this);
    akPluginManager->registerPlugin(pluginInfo);

    return 0;
}

void Temperature::deleteThis(void *userData) const
{
    delete reinterpret_cast<Temperature *>(userData);
}

qreal Temperature::temperature() const
{
    return this->d->m_temperature;
}

QString Temperature::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Temperature/share/qml/main.qml");
}

void Temperature::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Temperature", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Temperature::iVideoStream(const AkVideoPacket &packet)
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
        this->oStream(dst);

    return dst;
}

void Temperature::setTemperature(qreal temperature)
{
    if (qFuzzyCompare(this->d->m_temperature, temperature))
        return;

    this->d->m_temperature = temperature;
    this->d->m_mutex.lock();
    this->d->updateTemperatureTable(temperature);
    this->d->m_mutex.unlock();
    emit this->temperatureChanged(temperature);
}

void Temperature::resetTemperature()
{
    this->setTemperature(6500);
}

TemperaturePrivate::TemperaturePrivate(Temperature *self):
      self(self)
{

}

void TemperaturePrivate::colorFromTemperature(qreal temperature,
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

void TemperaturePrivate::updateTemperatureTable(qreal temperature)
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

#include "moc_temperature.cpp"
