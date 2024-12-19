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

#ifndef AUDIOENCODERFDKAACELEMENT_H
#define AUDIOENCODERFDKAACELEMENT_H

#include <iak/akaudioencoder.h>

class AudioEncoderFdkAacElementPrivate;

class AudioEncoderFdkAacElement: public AkAudioEncoder
{
    Q_OBJECT
    Q_PROPERTY(bool errorResilient
               READ errorResilient
               WRITE setErrorResilient
               RESET resetErrorResilient
               NOTIFY errorResilientChanged)
    Q_PROPERTY(OutputFormat outputFormat
               READ outputFormat
               WRITE setOutputFormat
               RESET resetOutputFormat
               NOTIFY outputFormatChanged)

    public:
        enum OutputFormat
        {
            OutputFormat_Unknown = -1,
            OutputFormat_Raw = 0,
            OutputFormat_ADIF = 1,
            OutputFormat_ADTS = 2,
            OutputFormat_LATM_MCP1 = 6,
            OutputFormat_LATM_MCP0 = 7,
            OutputFormat_LOAS = 10,
            OutputFormat_Drm = 12,
        };
        Q_ENUM(OutputFormat)

        AudioEncoderFdkAacElement();
        ~AudioEncoderFdkAacElement();

        Q_INVOKABLE AkAudioEncoderCodecID codec() const override;
        Q_INVOKABLE AkCompressedAudioCaps outputCaps() const override;
        Q_INVOKABLE AkCompressedPackets headers() const override;
        Q_INVOKABLE qint64 encodedTimePts() const override;
        Q_INVOKABLE bool errorResilient() const;
        Q_INVOKABLE OutputFormat outputFormat() const;

    private:
        AudioEncoderFdkAacElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iAudioStream(const AkAudioPacket &packet) override;

    signals:
        void errorResilientChanged(bool errorResilient);
        void outputFormatChanged(OutputFormat outputFormat);

    public slots:
        void setErrorResilient(bool errorResilient);
        void setOutputFormat(OutputFormat outputFormat);
        void resetErrorResilient();
        void resetOutputFormat();
        bool setState(AkElement::ElementState state) override;
};

Q_DECLARE_METATYPE(AudioEncoderFdkAacElement::OutputFormat)

#endif // AUDIOENCODERFDKAACELEMENT_H
