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

#ifndef COLORFILTERELEMENT_H
#define COLORFILTERELEMENT_H

#include <qrgb.h>
#include <akelement.h>

class ColorFilterElementPrivate;

class ColorFilterElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QRgb colorf
               READ color
               WRITE setColor
               RESET resetColor
               NOTIFY colorChanged)
    Q_PROPERTY(int radius
               READ radius
               WRITE setRadius
               RESET resetRadius
               NOTIFY radiusChanged)
    Q_PROPERTY(bool soft
               READ soft
               WRITE setSoft
               RESET resetSoft
               NOTIFY softChanged)
    Q_PROPERTY(bool disable
               READ disable
               WRITE setDisable
               RESET resetDisable
               NOTIFY disableChanged)

    public:
        ColorFilterElement();
        ~ColorFilterElement();

        Q_INVOKABLE QRgb color() const;
        Q_INVOKABLE int radius() const;
        Q_INVOKABLE bool soft() const;
        Q_INVOKABLE bool disable() const;

    private:
        ColorFilterElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void colorChanged(QRgb color);
        void radiusChanged(int radius);
        void softChanged(bool soft);
        void disableChanged(bool disable);

    public slots:
        void setColor(QRgb color);
        void setRadius(int radius);
        void setSoft(bool soft);
        void setDisable(bool disable);
        void resetColor();
        void resetRadius();
        void resetSoft();
        void resetDisable();
};

#endif // COLORFILTERELEMENT_H
