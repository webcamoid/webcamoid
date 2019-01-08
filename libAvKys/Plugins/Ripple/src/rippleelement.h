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

#ifndef RIPPLEELEMENT_H
#define RIPPLEELEMENT_H

#include <akelement.h>

class RippleElementPrivate;

class RippleElement: public AkElement
{
    Q_OBJECT
    Q_ENUMS(RippleMode)
    Q_PROPERTY(QString mode
               READ mode
               WRITE setMode
               RESET resetMode
               NOTIFY modeChanged)
    Q_PROPERTY(int amplitude
               READ amplitude
               WRITE setAmplitude
               RESET resetAmplitude
               NOTIFY amplitudeChanged)
    Q_PROPERTY(int decay
               READ decay
               WRITE setDecay
               RESET resetDecay
               NOTIFY decayChanged)
    Q_PROPERTY(int threshold
               READ threshold
               WRITE setThreshold
               RESET resetThreshold
               NOTIFY thresholdChanged)
    Q_PROPERTY(int lumaThreshold
               READ lumaThreshold
               WRITE setLumaThreshold
               RESET resetLumaThreshold
               NOTIFY lumaThresholdChanged)

    public:
        enum RippleMode
        {
            RippleModeMotionDetect,
            RippleModeRain
        };

        RippleElement();
        ~RippleElement();

        Q_INVOKABLE QString mode() const;
        Q_INVOKABLE int amplitude() const;
        Q_INVOKABLE int decay() const;
        Q_INVOKABLE int threshold() const;
        Q_INVOKABLE int lumaThreshold() const;

    private:
        RippleElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const;

    signals:
        void modeChanged(const QString &mode);
        void amplitudeChanged(int amplitude);
        void decayChanged(int decay);
        void thresholdChanged(int threshold);
        void lumaThresholdChanged(int lumaThreshold);

    public slots:
        void setMode(const QString &mode);
        void setAmplitude(int amplitude);
        void setDecay(int decay);
        void setThreshold(int threshold);
        void setLumaThreshold(int lumaThreshold);
        void resetMode();
        void resetAmplitude();
        void resetDecay();
        void resetThreshold();
        void resetLumaThreshold();

        AkPacket iStream(const AkPacket &packet);
};

#endif // RIPPLEELEMENT_H
