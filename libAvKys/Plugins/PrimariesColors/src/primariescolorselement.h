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

#ifndef PRIMARIESCOLORSELEMENT_H
#define PRIMARIESCOLORSELEMENT_H

#include <akelement.h>

class PrimariesColorsElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(int factor
               READ factor
               WRITE setFactor
               RESET resetFactor
               NOTIFY factorChanged)

    public:
        explicit PrimariesColorsElement();

        Q_INVOKABLE int factor() const;

    private:
        int m_factor;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const;

    signals:
        void factorChanged(int factor);

    public slots:
        void setFactor(int factor);
        void resetFactor();

        AkPacket iStream(const AkPacket &packet);
};

#endif // PRIMARIESCOLORSELEMENT_H
