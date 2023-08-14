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

#ifndef SCANLINESELEMENT_H
#define SCANLINESELEMENT_H

#include <qrgb.h>
#include <akelement.h>

class ScanLinesElementPrivate;

class ScanLinesElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(int showSize
               READ showSize
               WRITE setShowSize
               RESET resetShowSize
               NOTIFY showSizeChanged)
    Q_PROPERTY(int hideSize
               READ hideSize
               WRITE setHideSize
               RESET resetHideSize
               NOTIFY hideSizeChanged)
    Q_PROPERTY(QRgb hideColor
               READ hideColor
               WRITE setHideColor
               RESET resetHideColor
               NOTIFY hideColorChanged)

    public:
        ScanLinesElement();
        ~ScanLinesElement();

        Q_INVOKABLE int showSize() const;
        Q_INVOKABLE int hideSize() const;
        Q_INVOKABLE QRgb hideColor() const;

    private:
        ScanLinesElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void showSizeChanged(int showSize);
        void hideSizeChanged(int hideSize);
        void hideColorChanged(QRgb hideColor);

    public slots:
        void setShowSize(int showSize);
        void setHideSize(int hideSize);
        void setHideColor(QRgb hideColor);
        void resetShowSize();
        void resetHideSize();
        void resetHideColor();
};

#endif // SCANLINESELEMENT_H
