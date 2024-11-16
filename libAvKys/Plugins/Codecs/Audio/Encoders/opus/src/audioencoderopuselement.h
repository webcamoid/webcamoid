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

#ifndef AUDIOENCODEROPUSELEMENT_H
#define AUDIOENCODEROPUSELEMENT_H

#include <iak/akaudioencoder.h>

class AudioEncoderOpusElementPrivate;

class AudioEncoderOpusElement: public AkAudioEncoder
{
    Q_OBJECT
    Q_PROPERTY(ApplicationType applicationType
               READ applicationType
               WRITE setApplicationType
               RESET resetApplicationType
               NOTIFY applicationTypeChanged)

    public:
        enum ApplicationType
        {
            ApplicationType_Unknown,
            ApplicationType_Voip,
            ApplicationType_Audio,
            ApplicationType_RestrictedLowdelay,
        };
        Q_ENUM(ApplicationType)

        AudioEncoderOpusElement();
        ~AudioEncoderOpusElement();

        Q_INVOKABLE AkAudioEncoderCodecID codec() const override;
        Q_INVOKABLE AkCompressedPackets headers() const override;
        Q_INVOKABLE ApplicationType applicationType() const;

    private:
        AudioEncoderOpusElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iAudioStream(const AkAudioPacket &packet) override;

    signals:
        void applicationTypeChanged(ApplicationType applicationType);

    public slots:
        void setApplicationType(ApplicationType applicationType);
        void resetApplicationType();
        bool setState(AkElement::ElementState state) override;
};

#endif // AUDIOENCODEROPUSELEMENT_H
