/* Webcamoid, webcam capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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

#ifndef COLORKEYELEMENT_H
#define COLORKEYELEMENT_H

#include <qrgb.h>
#include <akelement.h>

class ColorKeyElementPrivate;

class ColorKeyElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QRgb colorKey
               READ colorKey
               WRITE setColorKey
               RESET resetColorKey
               NOTIFY colorKeyChanged)
    Q_PROPERTY(int colorDiff
               READ colorDiff
               WRITE setColorDiff
               RESET resetColorDiff
               NOTIFY colorDiffChanged)
    Q_PROPERTY(int smoothness
               READ smoothness
               WRITE setSmoothness
               RESET resetSmoothness
               NOTIFY smoothnessChanged)
    Q_PROPERTY(bool normalize
               READ normalize
               WRITE setNormalize
               RESET resetNormalize
               NOTIFY normalizeChanged)
    Q_PROPERTY(BackgroundType backgroundType
               READ backgroundType
               WRITE setBackgroundType
               RESET resetBackgroundType
               NOTIFY backgroundTypeChanged)
    Q_PROPERTY(QRgb backgroundColor
               READ backgroundColor
               WRITE setBackgroundColor
               RESET resetBackgroundColor
               NOTIFY backgroundColorChanged)
    Q_PROPERTY(QString background
               READ background
               WRITE setBackground
               RESET resetBackground
               NOTIFY backgroundChanged)

    public:
        enum BackgroundType
        {
            BackgroundTypeNoBackground,
            BackgroundTypeColor,
            BackgroundTypeImage
        };
        Q_ENUM(BackgroundType)

        ColorKeyElement();
        ~ColorKeyElement();

        Q_INVOKABLE QRgb colorKey() const;
        Q_INVOKABLE int colorDiff() const;
        Q_INVOKABLE int smoothness() const;
        Q_INVOKABLE bool normalize() const;
        Q_INVOKABLE BackgroundType backgroundType() const;
        Q_INVOKABLE QRgb backgroundColor() const;
        Q_INVOKABLE QString background() const;

    private:
        ColorKeyElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void colorKeyChanged(QRgb color);
        void colorDiffChanged(int colorDiff);
        void smoothnessChanged(int smoothness);
        void normalizeChanged(bool normalize);
        void backgroundTypeChanged(BackgroundType backgroundType);
        void backgroundColorChanged(QRgb backgroundColor);
        void backgroundChanged(const QString &background);

    public slots:
        void setColorKey(QRgb color);
        void setColorDiff(int colorDiff);
        void setSmoothness(int smoothness);
        void setNormalize(bool normalize);
        void setBackgroundType(BackgroundType backgroundType);
        void setBackgroundColor(QRgb backgroundColor);
        void setBackground(const QString &background);
        void resetColorKey();
        void resetColorDiff();
        void resetSmoothness();
        void resetNormalize();
        void resetBackgroundType();
        void resetBackgroundColor();
        void resetBackground();
};

Q_DECL_EXPORT QDataStream &operator >>(QDataStream &istream, ColorKeyElement::BackgroundType &mode);
Q_DECL_EXPORT QDataStream &operator <<(QDataStream &ostream, ColorKeyElement::BackgroundType mode);

Q_DECLARE_METATYPE(ColorKeyElement::BackgroundType)

#endif // COLORKEYELEMENT_H
