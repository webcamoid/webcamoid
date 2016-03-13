/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#ifndef OILPAINTELEMENT_H
#define OILPAINTELEMENT_H

#include <QQmlComponent>
#include <QQmlContext>
#include <ak.h>
#include <akutils.h>

class OilPaintElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(int radius
               READ radius
               WRITE setRadius
               RESET resetRadius
               NOTIFY radiusChanged)

    public:
        explicit OilPaintElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE int radius() const;

    private:
        int m_radius;

    signals:
        void radiusChanged(int radius);

    public slots:
        void setRadius(int radius);
        void resetRadius();
        AkPacket iStream(const AkPacket &packet);
};

#endif // OILPAINTELEMENT_H
