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

#ifndef ROTATEELEMENT_H
#define ROTATEELEMENT_H

#include <akelement.h>

class RotateElementPrivate;

class RotateElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(qreal angle
               READ angle
               WRITE setAngle
               RESET resetAngle
               NOTIFY angleChanged)
    Q_PROPERTY(bool keep
               READ keep
               WRITE setKeep
               RESET resetKeep
               NOTIFY keepChanged)

    public:
        RotateElement();
        ~RotateElement();

        Q_INVOKABLE qreal angle() const;
        Q_INVOKABLE bool keep() const;

    private:
        RotateElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void angleChanged(qreal angle);
        void keepChanged(bool keep);

    public slots:
        void setAngle(qreal angle);
        void setKeep(bool keep);
        void resetAngle();
        void resetKeep();
};

#endif // ROTATEELEMENT_H
