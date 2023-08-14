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

#ifndef SWIRLELEMENT_H
#define SWIRLELEMENT_H

#include <akelement.h>

class SwirlElementPrivate;

class SwirlElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(qreal degrees
               READ degrees
               WRITE setDegrees
               RESET resetDegrees
               NOTIFY degreesChanged)

    public:
        SwirlElement();
        ~SwirlElement();

        Q_INVOKABLE qreal degrees() const;

    private:
        SwirlElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void degreesChanged(qreal degrees);

    public slots:
        void setDegrees(qreal degrees);
        void resetDegrees();
};

#endif // SWIRLELEMENT_H
