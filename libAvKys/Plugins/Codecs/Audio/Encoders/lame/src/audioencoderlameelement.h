/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#ifndef AUDIOENCODERLAMEELEMENT_H
#define AUDIOENCODERLAMEELEMENT_H

#include <iak/akaudioencoder.h>

class AudioEncoderLameElementPrivate;

class AudioEncoderLameElement: public AkAudioEncoder
{
    Q_OBJECT

    public:
        AudioEncoderLameElement();
        ~AudioEncoderLameElement();

        Q_INVOKABLE AkAudioEncoderCodecID codec() const override;

    private:
        AudioEncoderLameElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iAudioStream(const AkAudioPacket &packet) override;

    public slots:
        bool setState(AkElement::ElementState state) override;
};

#endif // AUDIOENCODERLAMEELEMENT_H
