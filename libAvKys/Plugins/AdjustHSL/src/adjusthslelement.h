/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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

#ifndef ADJUSTHSLELEMENT_H
#define ADJUSTHSLELEMENT_H

#include <akelement.h>

class AdjustHSLElementPrivate;

class AdjustHSLElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(int hue
               READ hue
               WRITE setHue
               RESET resetHue
               NOTIFY hueChanged)
    Q_PROPERTY(int saturation
               READ saturation
               WRITE setSaturation
               RESET resetSaturation
               NOTIFY saturationChanged)
    Q_PROPERTY(int luminance
               READ luminance
               WRITE setLuminance
               RESET resetLuminance
               NOTIFY luminanceChanged)

    public:
        AdjustHSLElement();
        ~AdjustHSLElement();

        Q_INVOKABLE int hue() const;
        Q_INVOKABLE int saturation() const;
        Q_INVOKABLE int luminance() const;

    private:
        AdjustHSLElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void hueChanged(int hue);
        void saturationChanged(int saturation);
        void luminanceChanged(int luminance);

    public slots:
        void setHue(int hue);
        void setSaturation(int saturation);
        void setLuminance(int luminance);
        void resetHue();
        void resetSaturation();
        void resetLuminance();
};

#endif // ADJUSTHSLELEMENT_H
