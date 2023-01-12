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

#ifndef CINEMAELEMENT_H
#define CINEMAELEMENT_H

#include <qrgb.h>
#include <akelement.h>

class CinemaElementPrivate;

class CinemaElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(qreal stripSize
               READ stripSize
               WRITE setStripSize
               RESET resetStripSize
               NOTIFY stripSizeChanged)
    Q_PROPERTY(QRgb stripColor
               READ stripColor
               WRITE setStripColor
               RESET resetStripColor
               NOTIFY stripColorChanged)

    public:
        CinemaElement();
        ~CinemaElement();

        Q_INVOKABLE qreal stripSize() const;
        Q_INVOKABLE QRgb stripColor() const;

    private:
        CinemaElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void stripSizeChanged(qreal stripSize);
        void stripColorChanged(QRgb stripColor);

    public slots:
        void setStripSize(qreal stripSize);
        void setStripColor(QRgb stripColor);
        void resetStripSize();
        void resetStripColor();
};

#endif // CINEMAELEMENT_H
