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

#ifndef ASPECTRATIOELEMENT_H
#define ASPECTRATIOELEMENT_H

#include <akelement.h>

class AspectRatioElementPrivate;

class AspectRatioElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(int width
               READ width
               WRITE setWidth
               RESET resetWidth
               NOTIFY widthChanged)
    Q_PROPERTY(int height
               READ height
               WRITE setHeight
               RESET resetHeight
               NOTIFY heightChanged)

    public:
        AspectRatioElement();
        ~AspectRatioElement();

        Q_INVOKABLE int width() const;
        Q_INVOKABLE int height() const;

    private:
        AspectRatioElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void widthChanged(int width);
        void heightChanged(int height);

    public slots:
        void setWidth(int width);
        void setHeight(int height);
        void resetWidth();
        void resetHeight();
};

#endif // ASPECTRATIOELEMENT_H
