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

#ifndef ZOOMELEMENT_H
#define ZOOMELEMENT_H

#include <akelement.h>

class ZoomElementPrivate;

class ZoomElement: public AkElement
{
    Q_OBJECT
    Q_DISABLE_COPY(ZoomElement)
    Q_PROPERTY(qreal zoom
               READ zoom
               WRITE setZoom
               RESET resetZoom
               NOTIFY zoomChanged)

    public:
        ZoomElement();
        ~ZoomElement();

        Q_INVOKABLE qreal zoom() const;

    private:
        ZoomElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void zoomChanged(qreal zoom);

    public slots:
        void setZoom(qreal zoom);
        void resetZoom();
};

#endif // ZOOMELEMENT_H
