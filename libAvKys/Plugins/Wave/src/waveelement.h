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

#ifndef WAVEELEMENT_H
#define WAVEELEMENT_H

#include <qrgb.h>
#include <akelement.h>

class WaveElementPrivate;

class WaveElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(qreal amplitudeX
               READ amplitudeX
               WRITE setAmplitudeX
               RESET resetAmplitudeX
               NOTIFY amplitudeXChanged)
    Q_PROPERTY(qreal amplitudeY
               READ amplitudeY
               WRITE setAmplitudeY
               RESET resetAmplitudeY
               NOTIFY amplitudeYChanged)
    Q_PROPERTY(qreal frequencyX
               READ frequencyX
               WRITE setFrequencyX
               RESET resetFrequencyX
               NOTIFY frequencyXChanged)
    Q_PROPERTY(qreal frequencyY
               READ frequencyY
               WRITE setFrequencyY
               RESET resetFrequencyY
               NOTIFY frequencyYChanged)
    Q_PROPERTY(qreal phaseX
               READ phaseX
               WRITE setPhaseX
               RESET resetPhaseX
               NOTIFY phaseXChanged)
    Q_PROPERTY(qreal phaseY
               READ phaseY
               WRITE setPhaseY
               RESET resetPhaseY
               NOTIFY phaseYChanged)

    public:
        WaveElement();
        ~WaveElement();

        Q_INVOKABLE qreal amplitudeX() const;
        Q_INVOKABLE qreal amplitudeY() const;
        Q_INVOKABLE qreal frequencyX() const;
        Q_INVOKABLE qreal frequencyY() const;
        Q_INVOKABLE qreal phaseX() const;
        Q_INVOKABLE qreal phaseY() const;

    private:
        WaveElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void amplitudeXChanged(qreal amplitudeX);
        void amplitudeYChanged(qreal amplitudeY);
        void frequencyXChanged(qreal frequencyX);
        void frequencyYChanged(qreal frequencyY);
        void phaseXChanged(qreal phaseX);
        void phaseYChanged(qreal phaseY);

    public slots:
        void setAmplitudeX(qreal amplitudeX);
        void setAmplitudeY(qreal amplitudeY);
        void setFrequencyX(qreal frequencyX);
        void setFrequencyY(qreal frequencyY);
        void setPhaseX(qreal phaseX);
        void setPhaseY(qreal phaseY);
        void resetAmplitudeX();
        void resetAmplitudeY();
        void resetFrequencyX();
        void resetFrequencyY();
        void resetPhaseX();
        void resetPhaseY();
};

#endif // WAVEELEMENT_H
