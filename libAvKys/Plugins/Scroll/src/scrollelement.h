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

#ifndef SCROLLELEMENT_H
#define SCROLLELEMENT_H

#include <akelement.h>

class ScrollElementPrivate;

class ScrollElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(qreal speed
               READ speed
               WRITE setSpeed
               RESET resetSpeed
               NOTIFY speedChanged)
    Q_PROPERTY(qreal noise
               READ noise
               WRITE setNoise
               RESET resetNoise
               NOTIFY noiseChanged)

    public:
        explicit ScrollElement();
        ~ScrollElement();

        Q_INVOKABLE qreal speed() const;
        Q_INVOKABLE qreal noise() const;

    private:
        ScrollElementPrivate *d;

        QImage generateNoise(const QSize &size, qreal persent);

    protected:
        QString controlInterfaceProvide(const QString &controlId) const;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const;

    signals:
        void speedChanged(qreal speed);
        void noiseChanged(qreal noise);

    public slots:
        void setSpeed(qreal speed);
        void setNoise(qreal noise);
        void resetSpeed();
        void resetNoise();
        AkPacket iStream(const AkPacket &packet);
};

#endif // SCROLLELEMENT_H
