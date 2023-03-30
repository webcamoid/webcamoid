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

#ifndef PHOTOCOPYELEMENT_H
#define PHOTOCOPYELEMENT_H

#include <akelement.h>

class PhotocopyElementPrivate;

class PhotocopyElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(int brightness
               READ brightness
               WRITE setBrightness
               RESET resetBrightness
               NOTIFY brightnessChanged)
    Q_PROPERTY(int contrast
               READ contrast
               WRITE setContrast
               RESET resetContrast
               NOTIFY contrastChanged)

    public:
        PhotocopyElement();
        ~PhotocopyElement();

        Q_INVOKABLE int brightness() const;
        Q_INVOKABLE int contrast() const;

    private:
        PhotocopyElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void brightnessChanged(int brightness);
        void contrastChanged(int contrast);

    public slots:
        void setBrightness(int brightness);
        void setContrast(int contrast);
        void resetBrightness();
        void resetContrast();
};

#endif // PHOTOCOPYELEMENT_H
