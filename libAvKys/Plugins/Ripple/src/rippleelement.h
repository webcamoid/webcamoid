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
    Q_PROPERTY(RippleMode mode
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
    Q_PROPERTY(int minDropSize
               READ minDropSize
               WRITE setMinDropSize
               RESET resetMinDropSize
               NOTIFY minDropSizeChanged)
    Q_PROPERTY(int maxDropSize
               READ maxDropSize
               WRITE setMaxDropSize
               RESET resetMaxDropSize
               NOTIFY maxDropSizeChanged)
    Q_PROPERTY(qreal dropSigma
               READ dropSigma
               WRITE setDropSigma
               RESET resetDropSigma
               NOTIFY dropSigmaChanged)
    Q_PROPERTY(qreal dropProbability
               READ dropProbability
               WRITE setDropProbability
               RESET resetDropProbability
               NOTIFY dropProbabilityChanged)

    public:
        enum RippleMode
        {
            RippleModeMotionDetect,
            RippleModeRain
        };
        Q_ENUM(RippleMode)

        RippleElement();
        ~RippleElement();

        Q_INVOKABLE RippleMode mode() const;
        Q_INVOKABLE int amplitude() const;
        Q_INVOKABLE int decay() const;
        Q_INVOKABLE int threshold() const;
        Q_INVOKABLE int lumaThreshold() const;
        Q_INVOKABLE int minDropSize() const;
        Q_INVOKABLE int maxDropSize() const;
        Q_INVOKABLE qreal dropSigma() const;
        Q_INVOKABLE qreal dropProbability() const;

    private:
        RippleElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void modeChanged(RippleMode mode);
        void amplitudeChanged(int amplitude);
        void decayChanged(int decay);
        void thresholdChanged(int threshold);
        void lumaThresholdChanged(int lumaThreshold);
        void minDropSizeChanged(int minDropSize);
        void maxDropSizeChanged(int maxDropSize);
        void dropSigmaChanged(qreal dropSigma);
        void dropProbabilityChanged(qreal dropProbability);

    public slots:
        void setMode(RippleMode mode);
        void setAmplitude(int amplitude);
        void setDecay(int decay);
        void setThreshold(int threshold);
        void setLumaThreshold(int lumaThreshold);
        void setMinDropSize(int minDropSize);
        void setMaxDropSize(int maxDropSize);
        void setDropSigma(qreal dropSigma);
        void setDropProbability(qreal dropProbability);
        void resetMode();
        void resetAmplitude();
        void resetDecay();
        void resetThreshold();
        void resetLumaThreshold();
        void resetMinDropSize();
        void resetMaxDropSize();
        void resetDropSigma();
        void resetDropProbability();
};

Q_DECL_EXPORT QDataStream &operator >>(QDataStream &istream, RippleElement::RippleMode &mode);
Q_DECL_EXPORT QDataStream &operator <<(QDataStream &ostream, RippleElement::RippleMode mode);

Q_DECLARE_METATYPE(RippleElement::RippleMode)

#endif // RIPPLEELEMENT_H
