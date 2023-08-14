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

#ifndef FIREELEMENT_H
#define FIREELEMENT_H

#include <akelement.h>

class FireElementPrivate;

class FireElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(FireMode mode
               READ mode
               WRITE setMode
               RESET resetMode
               NOTIFY modeChanged)
    Q_PROPERTY(int cool
               READ cool
               WRITE setCool
               RESET resetCool
               NOTIFY coolChanged)
    Q_PROPERTY(qreal dissolve
               READ dissolve
               WRITE setDissolve
               RESET resetDissolve
               NOTIFY dissolveChanged)
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
    Q_PROPERTY(int alphaVariation
               READ alphaVariation
               WRITE setAlphaVariation
               RESET resetAlphaVariation
               NOTIFY alphaVariationChanged)
    Q_PROPERTY(int nColors
               READ nColors
               WRITE setNColors
               RESET resetNColors
               NOTIFY nColorsChanged)

    public:
        enum FireMode
        {
            FireModeSoft,
            FireModeHard
        };
        Q_ENUM(FireMode)

        FireElement();
        ~FireElement();

        Q_INVOKABLE FireMode mode() const;
        Q_INVOKABLE int cool() const;
        Q_INVOKABLE qreal dissolve() const;
        Q_INVOKABLE int blur() const;
        Q_INVOKABLE qreal zoom() const;
        Q_INVOKABLE int threshold() const;
        Q_INVOKABLE int lumaThreshold() const;
        Q_INVOKABLE int alphaDiff() const;
        Q_INVOKABLE int alphaVariation() const;
        Q_INVOKABLE int nColors() const;

    private:
        FireElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void modeChanged(FireMode mode);
        void coolChanged(int cool);
        void dissolveChanged(qreal dissolve);
        void blurChanged(int blur);
        void zoomChanged(qreal zoom);
        void thresholdChanged(int threshold);
        void lumaThresholdChanged(int lumaThreshold);
        void alphaDiffChanged(int alphaDiff);
        void alphaVariationChanged(int alphaVariation);
        void nColorsChanged(int nColors);

    public slots:
        void setMode(const FireMode &mode);
        void setCool(int cool);
        void setDissolve(qreal dissolve);
        void setBlur(int blur);
        void setZoom(qreal zoom);
        void setThreshold(int threshold);
        void setLumaThreshold(int lumaThreshold);
        void setAlphaDiff(int alphaDiff);
        void setAlphaVariation(int alphaVariation);
        void setNColors(int nColors);
        void resetMode();
        void resetCool();
        void resetDissolve();
        void resetBlur();
        void resetZoom();
        void resetThreshold();
        void resetLumaThreshold();
        void resetAlphaDiff();
        void resetAlphaVariation();
        void resetNColors();
};

Q_DECL_EXPORT QDataStream &operator >>(QDataStream &istream, FireElement::FireMode &mode);
Q_DECL_EXPORT QDataStream &operator <<(QDataStream &ostream, FireElement::FireMode mode);

Q_DECLARE_METATYPE(FireElement::FireMode)

#endif // FIREELEMENT_H
