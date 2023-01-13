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

#ifndef CONTRASTELEMENT_H
#define CONTRASTELEMENT_H

#include <akelement.h>

class ContrastElementPrivate;

class ContrastElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(int contrast
               READ contrast
               WRITE setContrast
               RESET resetContrast
               NOTIFY contrastChanged)

    public:
        ContrastElement();
        ~ContrastElement();

        Q_INVOKABLE int contrast() const;

    private:
        ContrastElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void contrastChanged(int contrast);

    public slots:
        void setContrast(int contrast);
        void resetContrast();
};

#endif // CONTRASTELEMENT_H
