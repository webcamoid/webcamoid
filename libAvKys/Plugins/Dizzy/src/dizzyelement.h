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

#ifndef DIZZYELEMENT_H
#define DIZZYELEMENT_H

#include <akelement.h>

class DizzyElementPrivate;

class DizzyElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(qreal speed
               READ speed
               WRITE setSpeed
               RESET resetSpeed
               NOTIFY speedChanged)
    Q_PROPERTY(qreal zoomRate
               READ zoomRate
               WRITE setZoomRate
               RESET resetZoomRate
               NOTIFY zoomRateChanged)
    Q_PROPERTY(qreal strength
               READ strength
               WRITE setStrength
               RESET resetStrength
               NOTIFY strengthChanged)

    public:
        DizzyElement();
        ~DizzyElement();

        Q_INVOKABLE qreal speed() const;
        Q_INVOKABLE qreal zoomRate() const;
        Q_INVOKABLE qreal strength() const;

    private:
        DizzyElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void speedChanged(qreal speed);
        void zoomRateChanged(qreal zoomRate);
        void strengthChanged(qreal strength);

    public slots:
        void setSpeed(qreal speed);
        void setZoomRate(qreal zoomRate);
        void setStrength(qreal strength);
        void resetSpeed();
        void resetZoomRate();
        void resetStrength();
};

#endif // DIZZYELEMENT_H
