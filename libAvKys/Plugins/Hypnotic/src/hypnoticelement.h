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

#ifndef HYPNOTICELEMENT_H
#define HYPNOTICELEMENT_H

#include <akelement.h>

class HypnoticElementPrivate;

class HypnoticElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(OpticMode mode
               READ mode
               WRITE setMode
               RESET resetMode
               NOTIFY modeChanged)
    Q_PROPERTY(int speedInc
               READ speedInc
               WRITE setSpeedInc
               RESET resetSpeedInc
               NOTIFY speedIncChanged)
    Q_PROPERTY(int threshold
               READ threshold
               WRITE setThreshold
               RESET resetThreshold
               NOTIFY thresholdChanged)

    public:
        enum OpticMode
        {
            OpticModeSpiral1,
            OpticModeSpiral2,
            OpticModeParabola,
            OpticModeHorizontalStripe
        };
        Q_ENUM(OpticMode)

        HypnoticElement();
        ~HypnoticElement();

        Q_INVOKABLE OpticMode mode() const;
        Q_INVOKABLE int speedInc() const;
        Q_INVOKABLE int threshold() const;

    private:
        HypnoticElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void modeChanged(OpticMode mode);
        void speedIncChanged(int speedInc);
        void thresholdChanged(int threshold);

    public slots:
        void setMode(OpticMode mode);
        void setSpeedInc(int speedInc);
        void setThreshold(int threshold);
        void resetMode();
        void resetSpeedInc();
        void resetThreshold();
};

Q_DECL_EXPORT QDataStream &operator >>(QDataStream &istream, HypnoticElement::OpticMode &mode);
Q_DECL_EXPORT QDataStream &operator <<(QDataStream &ostream, HypnoticElement::OpticMode mode);

Q_DECLARE_METATYPE(HypnoticElement::OpticMode)

#endif // HYPNOTICELEMENT_H
