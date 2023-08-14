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
#include <QQmlContext>
#include <QSize>
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "waveelement.h"

class WaveElementPrivate
{
    public:
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

        void updateSineMap();
};

WaveElement::WaveElement(): AkElement()
{
    this->d = new WaveElementPrivate;
}

WaveElement::~WaveElement()
{
    if (this->d->m_sineMapX)
        delete [] this->d->m_sineMapX;

    if (this->d->m_sineMapY)
        delete [] this->d->m_sineMapY;

    delete this->d;
}

qreal WaveElement::amplitudeX() const
{
    return this->d->m_amplitudeX;
}

qreal WaveElement::amplitudeY() const
{
    return this->d->m_amplitudeY;
}

qreal WaveElement::frequencyX() const
{
    return this->d->m_frequencyX;
}

qreal WaveElement::frequencyY() const
{
    return this->d->m_frequencyY;
}

qreal WaveElement::phaseX() const
{
    return this->d->m_phaseX;
}

qreal WaveElement::phaseY() const
{
    return this->d->m_phaseY;
}

QString WaveElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Wave/share/qml/main.qml");
}

void WaveElement::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Wave", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket WaveElement::iVideoStream(const AkVideoPacket &packet)
{
    if (this->d->m_amplitudeX <= 0.0 && this->d->m_amplitudeY <= 0.0) {
        if (packet)
            emit this->oStream(packet);

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
            emit this->oStream(packet);

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
        emit this->oStream(dst);

    return dst;
}

inline void WaveElement::setAmplitudeX(qreal amplitudeX)
{
    if (qFuzzyCompare(amplitudeX, this->d->m_amplitudeX))
        return;

    this->d->m_amplitudeX = amplitudeX;
    emit this->amplitudeXChanged(amplitudeX);
    this->d->updateSineMap();
}

void WaveElement::setAmplitudeY(qreal amplitudeY)
{
    if (qFuzzyCompare(amplitudeY, this->d->m_amplitudeY))
        return;

    this->d->m_amplitudeY = amplitudeY;
    emit this->amplitudeYChanged(amplitudeY);
    this->d->updateSineMap();
}

void WaveElement::setFrequencyX(qreal frequencyX)
{
    if (qFuzzyCompare(frequencyX, this->d->m_frequencyX))
        return;

    this->d->m_frequencyX = frequencyX;
    emit this->frequencyXChanged(frequencyX);
    this->d->updateSineMap();
}

void WaveElement::setFrequencyY(qreal frequencyY)
{
    if (qFuzzyCompare(frequencyY, this->d->m_frequencyY))
        return;

    this->d->m_frequencyY = frequencyY;
    emit this->frequencyYChanged(frequencyY);
    this->d->updateSineMap();
}

void WaveElement::setPhaseX(qreal phaseX)
{
    if (qFuzzyCompare(this->d->m_phaseX, phaseX))
        return;

    this->d->m_phaseX = phaseX;
    emit this->phaseXChanged(phaseX);
    this->d->updateSineMap();
}

void WaveElement::setPhaseY(qreal phaseY)
{
    if (qFuzzyCompare(this->d->m_phaseY, phaseY))
        return;

    this->d->m_phaseY = phaseY;
    emit this->phaseYChanged(phaseY);
    this->d->updateSineMap();
}

void WaveElement::resetAmplitudeX()
{
    this->setAmplitudeX(0.12);
}

void WaveElement::resetAmplitudeY()
{
    this->setAmplitudeY(0.12);
}

void WaveElement::resetFrequencyX()
{
    this->setFrequencyX(8);
}

void WaveElement::resetFrequencyY()
{
    this->setFrequencyY(8);
}

void WaveElement::resetPhaseX()
{
    this->setPhaseX(0.0);
}

void WaveElement::resetPhaseY()
{
    this->setPhaseY(0.0);
}

void WaveElementPrivate::updateSineMap()
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

#include "moc_waveelement.cpp"
