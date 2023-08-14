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

#ifndef COLORREPLACEELEMENT_H
#define COLORREPLACEELEMENT_H

#include <qrgb.h>
#include <akelement.h>

class ColorReplaceElementPrivate;

class ColorReplaceElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QRgb from
               READ from
               WRITE setFrom
               RESET resetFrom
               NOTIFY fromChanged)
    Q_PROPERTY(QRgb to
               READ to
               WRITE setTo
               RESET resetTo
               NOTIFY toChanged)
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
        ColorReplaceElement();
        ~ColorReplaceElement();

        Q_INVOKABLE QRgb from() const;
        Q_INVOKABLE QRgb to() const;
        Q_INVOKABLE int radius() const;
        Q_INVOKABLE bool soft() const;
        Q_INVOKABLE bool disable() const;

    private:
        ColorReplaceElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void fromChanged(QRgb from);
        void toChanged(QRgb to);
        void radiusChanged(int radius);
        void softChanged(bool soft);
        void disableChanged(bool disable);

    public slots:
        void setFrom(QRgb from);
        void setTo(QRgb to);
        void setRadius(int radius);
        void setSoft(bool soft);
        void setDisable(bool disable);
        void resetFrom();
        void resetTo();
        void resetRadius();
        void resetSoft();
        void resetDisable();
};

#endif // COLORREPLACEELEMENT_H
