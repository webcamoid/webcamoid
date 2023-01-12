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

#ifndef RADIOACTIVEELEMENT_H
#define RADIOACTIVEELEMENT_H

#include <qrgb.h>
#include <akelement.h>

class RadioactiveElementPrivate;

class RadioactiveElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(RadiationMode mode
               READ mode
               WRITE setMode
               RESET resetMode
               NOTIFY modeChanged)
    Q_PROPERTY(int blur
               READ blur
               WRITE setBlur
               RESET resetBlur
               NOTIFY blurChanged)
    Q_PROPERTY(qreal zoom
               READ zoom
               WRITE setZoom
               RESET resetZoom
               NOTIFY zoomChanged)
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
    Q_PROPERTY(int alphaDiff
               READ alphaDiff
               WRITE setAlphaDiff
               RESET resetAlphaDiff
               NOTIFY alphaDiffChanged)
    Q_PROPERTY(QRgb radColor
               READ radColor
               WRITE setRadColor
               RESET resetRadColor
               NOTIFY radColorChanged)

    public:
        enum RadiationMode
        {
            RadiationModeSoftNormal,
            RadiationModeHardNormal,
            RadiationModeSoftColor,
            RadiationModeHardColor
        };
        Q_ENUM(RadiationMode)

        RadioactiveElement();
        ~RadioactiveElement();

        Q_INVOKABLE RadiationMode mode() const;
        Q_INVOKABLE int blur() const;
        Q_INVOKABLE qreal zoom() const;
        Q_INVOKABLE int threshold() const;
        Q_INVOKABLE int lumaThreshold() const;
        Q_INVOKABLE int alphaDiff() const;
        Q_INVOKABLE QRgb radColor() const;

    private:
        RadioactiveElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void modeChanged(RadiationMode mode);
        void blurChanged(int blur);
        void zoomChanged(qreal zoom);
        void thresholdChanged(int threshold);
        void lumaThresholdChanged(int lumaThreshold);
        void alphaDiffChanged(int alphaDiff);
        void radColorChanged(QRgb radColor);

    public slots:
        void setMode(RadiationMode mode);
        void setBlur(int blur);
        void setZoom(qreal zoom);
        void setThreshold(int threshold);
        void setLumaThreshold(int lumaThreshold);
        void setAlphaDiff(int alphaDiff);
        void setRadColor(QRgb radColor);
        void resetMode();
        void resetBlur();
        void resetZoom();
        void resetThreshold();
        void resetLumaThreshold();
        void resetAlphaDiff();
        void resetRadColor();
};

Q_DECL_EXPORT QDataStream &operator >>(QDataStream &istream, RadioactiveElement::RadiationMode &mode);
Q_DECL_EXPORT QDataStream &operator <<(QDataStream &ostream, RadioactiveElement::RadiationMode mode);

Q_DECLARE_METATYPE(RadioactiveElement::RadiationMode)

#endif // RADIOACTIVEELEMENT_H
