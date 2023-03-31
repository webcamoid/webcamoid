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

#ifndef WARHOLELEMENT_H
#define WARHOLELEMENT_H

#include <qrgb.h>

#include <akelement.h>

class WarholElementPrivate;

class WarholElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(int frameLen
               READ frameLen
               WRITE setFrameLen
               RESET resetFrameLen
               NOTIFY frameLenChanged)
    Q_PROPERTY(int levels
               READ levels
               WRITE setLevels
               RESET resetLevels
               NOTIFY levelsChanged)
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
    Q_PROPERTY(int paletteOffset
               READ paletteOffset
               WRITE setPaletteOffset
               RESET resetPaletteOffset
               NOTIFY paletteOffsetChanged)
    Q_PROPERTY(int shadowThLow
               READ shadowThLow
               WRITE setShadowThLow
               RESET resetShadowThLow
               NOTIFY shadowThLowChanged)
    Q_PROPERTY(int shadowThHi
               READ shadowThHi
               WRITE setShadowThHi
               RESET resetShadowThHi
               NOTIFY shadowThHiChanged)
    Q_PROPERTY(QRgb shadowColor
               READ shadowColor
               WRITE setShadowColor
               RESET resetShadowColor
               NOTIFY shadowColorChanged)

    public:
        WarholElement();
        ~WarholElement();

        Q_INVOKABLE int frameLen() const;
        Q_INVOKABLE int levels() const;
        Q_INVOKABLE int saturation() const;
        Q_INVOKABLE int luminance() const;
        Q_INVOKABLE int paletteOffset() const;
        Q_INVOKABLE int shadowThLow() const;
        Q_INVOKABLE int shadowThHi() const;
        Q_INVOKABLE QRgb shadowColor() const;

    private:
        WarholElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void frameLenChanged(int frameLen);
        void levelsChanged(int levels);
        void saturationChanged(int saturation);
        void luminanceChanged(int luminance);
        void paletteOffsetChanged(int paletteOffset);
        void shadowThLowChanged(int shadowThLow);
        void shadowThHiChanged(int shadowThHi);
        void shadowColorChanged(QRgb shadowColor);

    public slots:
        void setFrameLen(int frameLen);
        void setLevels(int levels);
        void setSaturation(int saturation);
        void setLuminance(int luminance);
        void setPaletteOffset(int paletteOffset);
        void setShadowThLow(int shadowThLow);
        void setShadowThHi(int shadowThHi);
        void setShadowColor(QRgb shadowColor);
        void resetFrameLen();
        void resetLevels();
        void resetSaturation();
        void resetLuminance();
        void resetPaletteOffset();
        void resetShadowThLow();
        void resetShadowThHi();
        void resetShadowColor();
};

#endif // WARHOLELEMENT_H
