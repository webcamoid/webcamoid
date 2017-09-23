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

#ifndef WAVEELEMENT_H
#define WAVEELEMENT_H

#include <QMutex>
#include <ak.h>
#include <akutils.h>

class WaveElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(qreal amplitude
               READ amplitude
               WRITE setAmplitude
               RESET resetAmplitude
               NOTIFY amplitudeChanged)
    Q_PROPERTY(qreal frequency
               READ frequency
               WRITE setFrequency
               RESET resetFrequency
               NOTIFY frequencyChanged)
    Q_PROPERTY(qreal phase
               READ phase
               WRITE setPhase
               RESET resetPhase
               NOTIFY phaseChanged)
    Q_PROPERTY(QRgb background
               READ background
               WRITE setBackground
               RESET resetBackground
               NOTIFY backgroundChanged)

    public:
        explicit WaveElement();

        Q_INVOKABLE qreal amplitude() const;
        Q_INVOKABLE qreal frequency() const;
        Q_INVOKABLE qreal phase() const;
        Q_INVOKABLE QRgb background() const;

    private:
        qreal m_amplitude;
        qreal m_frequency;
        qreal m_phase;
        QRgb m_background;
        QSize m_frameSize;
        QVector<int> m_sineMap;
        QMutex m_mutex;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const;

    signals:
        void amplitudeChanged(qreal amplitude);
        void frequencyChanged(qreal frequency);
        void phaseChanged(qreal phase);
        void backgroundChanged(QRgb background);
        void frameSizeChanged(const QSize &frameSize);

    public slots:
        void setAmplitude(qreal amplitude);
        void setFrequency(qreal frequency);
        void setPhase(qreal phase);
        void setBackground(QRgb background);
        void resetAmplitude();
        void resetFrequency();
        void resetPhase();
        void resetBackground();
        AkPacket iStream(const AkPacket &packet);

    private slots:
        void updateSineMap();
};

#endif // WAVEELEMENT_H
