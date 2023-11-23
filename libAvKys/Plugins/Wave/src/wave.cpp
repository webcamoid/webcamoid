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
#include <QSize>
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "wave.h"

class WavePrivate
{
    public:
        Wave *self {nullptr};
        QString m_description {QObject::tr("Wave")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        qreal m_amplitudeX {0.06};
        qreal m_amplitudeY {0.06};
        qreal m_frequencyX {4};
        qreal m_frequencyY {4};
        qreal m_phaseX {0.0};
        qreal m_phaseY {0.0};
        QSize m_frameSize;
        int *m_sineMapX {nullptr};
        int *m_sineMapY {nullptr};
        QMutex m_mutex;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        explicit WavePrivate(Wave *self);
        void updateSineMap();
};

Wave::Wave(QObject *parent):
      QObject(parent)
{
    this->d = new WavePrivate(this);
}

Wave::~Wave()
{
    if (this->d->m_sineMapX)
        delete [] this->d->m_sineMapX;

    if (this->d->m_sineMapY)
        delete [] this->d->m_sineMapY;

    delete this->d;
}

QString Wave::description() const
{
    return this->d->m_description;
}

AkElementType Wave::type() const
{
    return this->d->m_type;
}

AkElementCategory Wave::category() const
{
    return this->d->m_category;
}

void *Wave::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Wave::create(const QString &id)
{
    Q_UNUSED(id)

    return new Wave;
}

int Wave::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Wave",
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

qreal Wave::amplitudeX() const
{
    return this->d->m_amplitudeX;
}

qreal Wave::amplitudeY() const
{
    return this->d->m_amplitudeY;
}

qreal Wave::frequencyX() const
{
    return this->d->m_frequencyX;
}

qreal Wave::frequencyY() const
{
    return this->d->m_frequencyY;
}

qreal Wave::phaseX() const
{
    return this->d->m_phaseX;
}

qreal Wave::phaseY() const
{
    return this->d->m_phaseY;
}

void Wave::deleteThis(void *userData) const
{
    delete reinterpret_cast<Wave *>(userData);
}

QString Wave::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Wave/share/qml/main.qml");
}

void Wave::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Wave", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Wave::iVideoStream(const AkVideoPacket &packet)
{
    if (this->d->m_amplitudeX <= 0.0 && this->d->m_amplitudeY <= 0.0) {
        if (packet)
            this->oStream(packet);

        return packet;
    }

    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);
    QSize frameSize(src.caps().width(), src.caps().height());

    if (frameSize != this->d->m_frameSize) {
        this->d->m_frameSize = frameSize;
        this->d->updateSineMap();
    }

    this->d->m_mutex.lock();

    if (!this->d->m_sineMapY) {
        this->d->m_mutex.unlock();

        if (packet)
            this->oStream(packet);

        return packet;
    }

    for (int y = 0; y < dst.caps().height(); y++) {
        auto yOffset = y * dst.caps().width();
        auto sinLineX = this->d->m_sineMapX + yOffset;
        auto sinLineY = this->d->m_sineMapY + yOffset;
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < dst.caps().width(); x++) {
            int xi = sinLineX[x];
            int yi = sinLineY[x];

            if (xi >= 0 && xi < dst.caps().width()
                && yi >= 0 && yi < dst.caps().height())
                dstLine[x] = src.pixel<QRgb>(0, xi, yi);
            else
                dstLine[x] = qRgba(0, 0, 0, 0);
        }
    }

    this->d->m_mutex.unlock();

    if (dst)
        this->oStream(dst);

    return dst;
}

inline void Wave::setAmplitudeX(qreal amplitudeX)
{
    if (qFuzzyCompare(amplitudeX, this->d->m_amplitudeX))
        return;

    this->d->m_amplitudeX = amplitudeX;
    emit this->amplitudeXChanged(amplitudeX);
    this->d->updateSineMap();
}

void Wave::setAmplitudeY(qreal amplitudeY)
{
    if (qFuzzyCompare(amplitudeY, this->d->m_amplitudeY))
        return;

    this->d->m_amplitudeY = amplitudeY;
    emit this->amplitudeYChanged(amplitudeY);
    this->d->updateSineMap();
}

void Wave::setFrequencyX(qreal frequencyX)
{
    if (qFuzzyCompare(frequencyX, this->d->m_frequencyX))
        return;

    this->d->m_frequencyX = frequencyX;
    emit this->frequencyXChanged(frequencyX);
    this->d->updateSineMap();
}

void Wave::setFrequencyY(qreal frequencyY)
{
    if (qFuzzyCompare(frequencyY, this->d->m_frequencyY))
        return;

    this->d->m_frequencyY = frequencyY;
    emit this->frequencyYChanged(frequencyY);
    this->d->updateSineMap();
}

void Wave::setPhaseX(qreal phaseX)
{
    if (qFuzzyCompare(this->d->m_phaseX, phaseX))
        return;

    this->d->m_phaseX = phaseX;
    emit this->phaseXChanged(phaseX);
    this->d->updateSineMap();
}

void Wave::setPhaseY(qreal phaseY)
{
    if (qFuzzyCompare(this->d->m_phaseY, phaseY))
        return;

    this->d->m_phaseY = phaseY;
    emit this->phaseYChanged(phaseY);
    this->d->updateSineMap();
}

void Wave::resetAmplitudeX()
{
    this->setAmplitudeX(0.12);
}

void Wave::resetAmplitudeY()
{
    this->setAmplitudeY(0.12);
}

void Wave::resetFrequencyX()
{
    this->setFrequencyX(8);
}

void Wave::resetFrequencyY()
{
    this->setFrequencyY(8);
}

void Wave::resetPhaseX()
{
    this->setPhaseX(0.0);
}

void Wave::resetPhaseY()
{
    this->setPhaseY(0.0);
}

WavePrivate::WavePrivate(Wave *self):
      self(self)
{

}

void WavePrivate::updateSineMap()
{
    if (this->m_frameSize.isEmpty())
        return;

    int width = this->m_frameSize.width();
    int height = this->m_frameSize.height();
    auto amplitudeX = qRound(this->m_amplitudeX * width / 2);
    amplitudeX = qBound(0, amplitudeX, (width >> 1) - 1);
    auto amplitudeY = qRound(this->m_amplitudeY * height / 2);
    amplitudeY = qBound(0, amplitudeY, (height >> 1) - 1);
    qreal phaseX = 2.0 * M_PI * this->m_phaseX;
    qreal phaseY = 2.0 * M_PI * this->m_phaseY;

    this->m_mutex.lock();

    if (this->m_sineMapX)
        delete [] this->m_sineMapX;

    if (this->m_sineMapY)
        delete [] this->m_sineMapY;

    this->m_sineMapX = new int [width * height];
    this->m_sineMapY = new int [width * height];

    for (int yo = 0; yo < height; yo++) {
        auto yOffset = yo * width;
        auto sinLineX = this->m_sineMapX + yOffset;
        auto sinLineY = this->m_sineMapY + yOffset;
        int xoOffset = qRound(amplitudeX
                              * qSin(this->m_frequencyX * 2.0 * M_PI * yo / height
                                     + phaseX));

        for (int xo = 0; xo < width; xo++) {
            int yoOffset = qRound(amplitudeY
                                  * qSin(this->m_frequencyY * 2.0 * M_PI * xo / width
                                         + phaseY));

            int xi = (xo + xoOffset - amplitudeX) * (width - 1) / (width - 2 * amplitudeX - 1);
            int yi = (yo + yoOffset - amplitudeY) * (height - 1) / (height - 2 * amplitudeY - 1);

            sinLineX[xo] = xi;
            sinLineY[xo] = yi;
        }
    }

    this->m_mutex.unlock();
}

#include "moc_wave.cpp"
