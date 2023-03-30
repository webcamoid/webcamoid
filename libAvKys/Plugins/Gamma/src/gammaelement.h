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

#ifndef GAMMAELEMENT_H
#define GAMMAELEMENT_H

#include <akelement.h>

class GammaElementPrivate;

class GammaElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(int gamma
               READ gamma
               WRITE setGamma
               RESET resetGamma
               NOTIFY gammaChanged)

    public:
        GammaElement();
        ~GammaElement();

        Q_INVOKABLE int gamma() const;

    private:
        GammaElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void gammaChanged(int gamma);

    public slots:
        void setGamma(int gamma);
        void resetGamma();
};

#endif // GAMMAELEMENT_H
