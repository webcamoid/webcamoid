/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

#ifndef FLIPELEMENT_H
#define FLIPELEMENT_H

#include <akelement.h>

class FlipElementPrivate;

class FlipElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(bool horizontalFlip
               READ horizontalFlip
               WRITE setHorizontalFlip
               RESET resetHorizontalFlip
               NOTIFY horizontalFlipChanged)
    Q_PROPERTY(bool verticalFlip
               READ verticalFlip
               WRITE setVerticalFlip
               RESET resetVerticalFlip
               NOTIFY verticalFlipChanged)

    public:
        FlipElement();
        ~FlipElement();

        Q_INVOKABLE bool horizontalFlip() const;
        Q_INVOKABLE bool verticalFlip() const;

    private:
        FlipElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void horizontalFlipChanged(bool horizontalFlip);
        void verticalFlipChanged(bool verticalFlip);

    public slots:
        void setHorizontalFlip(bool horizontalFlip);
        void setVerticalFlip(bool verticalFlip);
        void resetHorizontalFlip();
        void resetVerticalFlip();
};

#endif // FLIPELEMENT_H
