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

#ifndef SATURATEDELEMENT_H
#define SATURATEDELEMENT_H

#include <akelement.h>

class SaturatedElementPrivate;

class SaturatedElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(int factor
               READ factor
               WRITE setFactor
               RESET resetFactor
               NOTIFY factorChanged)

    public:
        SaturatedElement();
        ~SaturatedElement();

        Q_INVOKABLE int factor() const;

    private:
        SaturatedElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void factorChanged(int factor);

    public slots:
        void setFactor(int factor);
        void resetFactor();
};

#endif // SATURATEDELEMENT_H
