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

#ifndef AUDIOENCODERFAACELEMENT_H
#define AUDIOENCODERFAACELEMENT_H

#include <iak/akaudioencoder.h>

class AudioEncoderFaacElementPrivate;

class AudioEncoderFaacElement: public AkAudioEncoder
{
    Q_OBJECT
    Q_PROPERTY(MpegVersion mpegVersion
               READ mpegVersion
               WRITE setMpegVersion
               RESET resetMpegVersion
               NOTIFY mpegVersionChanged)
    Q_PROPERTY(OutputFormat outputFormat
               READ outputFormat
               WRITE setOutputFormat
               RESET resetOutputFormat
               NOTIFY outputFormatChanged)

    public:
        enum MpegVersion
        {
            MpegVersion_MPEG4,
            MpegVersion_MPEG2,
        };
        Q_ENUM(MpegVersion)

        enum OutputFormat
        {
            OutputFormat_Raw,
            OutputFormat_ADTS,
        };
        Q_ENUM(OutputFormat)

        AudioEncoderFaacElement();
        ~AudioEncoderFaacElement();

        Q_INVOKABLE AkAudioEncoderCodecID codec() const override;
        Q_INVOKABLE AkCompressedAudioCaps outputCaps() const override;
        Q_INVOKABLE AkCompressedPackets headers() const override;
        Q_INVOKABLE qint64 encodedTimePts() const override;
        Q_INVOKABLE MpegVersion mpegVersion() const;
        Q_INVOKABLE OutputFormat outputFormat() const;

    private:
        AudioEncoderFaacElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iAudioStream(const AkAudioPacket &packet) override;

    signals:
        void mpegVersionChanged(MpegVersion mpegVersion);
        void outputFormatChanged(OutputFormat outputFormat);

    public slots:
        void setMpegVersion(MpegVersion mpegVersion);
        void setOutputFormat(OutputFormat outputFormat);
        void resetMpegVersion();
        void resetOutputFormat();
        bool setState(AkElement::ElementState state) override;
};

Q_DECLARE_METATYPE(AudioEncoderFaacElement::MpegVersion)
Q_DECLARE_METATYPE(AudioEncoderFaacElement::OutputFormat)

#endif // AUDIOENCODERFAACELEMENT_H
