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
#include <QMutex>
#include <akpacket.h>
#include <akvideopacket.h>

#include "waveelement.h"

class WaveElementPrivate
{
    public:
        qreal m_amplitude {0.12};
        qreal m_frequency {8};
        qreal m_phase {0.0};
        QRgb m_background {qRgb(0, 0, 0)};
        QSize m_frameSize;
        QVector<int> m_sineMap;
        QMutex m_mutex;
};

WaveElement::WaveElement(): AkElement()
{
    this->d = new WaveElementPrivate;

    QObject::connect(this,
                     &WaveElement::amplitudeChanged,
                     this,
                     &WaveElement::updateSineMap);
    QObject::connect(this,
                     &WaveElement::frequencyChanged,
                     this,
                     &WaveElement::updateSineMap);
    QObject::connect(this,
                     &WaveElement::phaseChanged,
                     this,
                     &WaveElement::updateSineMap);
    QObject::connect(this,
                     &WaveElement::backgroundChanged,
                     this,
                     &WaveElement::updateSineMap);
    QObject::connect(this,
                     &WaveElement::frameSizeChanged,
                     this,
                     &WaveElement::updateSineMap);
}

WaveElement::~WaveElement()
{

    delete this->d;
}

qreal WaveElement::amplitude() const
{
    return this->d->m_amplitude;
}

qreal WaveElement::frequency() const
{
    return this->d->m_frequency;
}

qreal WaveElement::phase() const
{
    return this->d->m_phase;
}

QRgb WaveElement::background() const
{
    return this->d->m_background;
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
    auto src = packet.toImage();

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    qreal amplitude = this->d->m_amplitude;

    QImage oFrame(src.width(), src.height(), src.format());
    oFrame.fill(this->d->m_background);

    if (amplitude <= 0.0)
        akSend(packet)

    if (amplitude >= 1.0) {
        auto oPacket = AkVideoPacket::fromImage(oFrame, packet);
        akSend(oPacket)
    }

    if (src.size() != this->d->m_frameSize) {
        this->d->m_frameSize = src.size();
        emit this->frameSizeChanged(src.size());
    }

    this->d->m_mutex.lock();
    QVector<int> sineMap(this->d->m_sineMap);
    this->d->m_mutex.unlock();

    if (sineMap.isEmpty())
        akSend(packet)

    for (int y = 0; y < oFrame.height(); y++) {
        // Get input line.
        int yi = int(y / (1.0 - amplitude));

        if (yi < 0 || yi >= src.height())
            continue;

        auto iLine = reinterpret_cast<const QRgb *>(src.constScanLine(yi));

        for (int x = 0; x < oFrame.width(); x++) {
            // Get output line.
            int yo = y  + sineMap[x];

            if (yo < 0
                || yo >= src.height())
                continue;

            QRgb *oLine = reinterpret_cast<QRgb *>(oFrame.scanLine(yo));
            oLine[x] = iLine[x];
        }
    }

    auto oPacket = AkVideoPacket::fromImage(oFrame, packet);
    akSend(oPacket)
}

void WaveElement::setAmplitude(qreal amplitude)
{
    if (qFuzzyCompare(amplitude, this->d->m_amplitude))
        return;

    this->d->m_amplitude = amplitude;
    emit this->amplitudeChanged(amplitude);
}

void WaveElement::setFrequency(qreal frequency)
{
    if (qFuzzyCompare(frequency, this->d->m_frequency))
        return;

    this->d->m_frequency = frequency;
    emit this->frequencyChanged(frequency);
}

void WaveElement::setPhase(qreal phase)
{
    if (qFuzzyCompare(this->d->m_phase, phase))
        return;

    this->d->m_phase = phase;
    emit this->phaseChanged(phase);
}

void WaveElement::setBackground(QRgb background)
{
    if (background == this->d->m_background)
        return;

    this->d->m_background = background;
    emit this->backgroundChanged(background);
}

void WaveElement::resetAmplitude()
{
    this->setAmplitude(0.12);
}

void WaveElement::resetFrequency()
{
    this->setFrequency(8);
}

void WaveElement::resetPhase()
{
    this->setPhase(0.0);
}

void WaveElement::resetBackground()
{
    this->setBackground(qRgb(0, 0, 0));
}

void WaveElement::updateSineMap()
{
    if (this->d->m_frameSize.isEmpty())
        return;

    int width = this->d->m_frameSize.width();
    int height = this->d->m_frameSize.height();
    QVector<int> sineMap(width);
    qreal phase = 2.0 * M_PI * this->d->m_phase;

    for (int x = 0; x < width; x++)
        sineMap[x] = int(0.5 * this->d->m_amplitude * height
                         * (sin(this->d->m_frequency * 2.0 * M_PI * x / width
                                + phase)
                            + 1.0));

    this->d->m_mutex.lock();
    this->d->m_sineMap = sineMap;
    this->d->m_mutex.unlock();
}

#include "moc_waveelement.cpp"
