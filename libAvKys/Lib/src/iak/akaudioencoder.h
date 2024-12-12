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

#ifndef AKAUDIOENCODER_H
#define AKAUDIOENCODER_H

#include "akelement.h"
#include "../akcompressedaudiocaps.h"
#include "../akcompressedaudiopacket.h"
#include "../akcompressedpacket.h"

class AkAudioEncoder;
class AkAudioEncoderPrivate;
class AkAudioCaps;

using AkAudioEncoderPtr = QSharedPointer<AkAudioEncoder>;
using AkAudioEncoderCodecID = AkCompressedAudioCaps::AudioCodecID;

class AKCOMMONS_EXPORT AkAudioEncoder: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(AkAudioEncoderCodecID codec
               READ codec
               CONSTANT)
    Q_PROPERTY(AkAudioCaps inputCaps
               READ inputCaps
               WRITE setInputCaps
               RESET resetInputCaps
               NOTIFY inputCapsChanged)
    Q_PROPERTY(AkCompressedAudioCaps outputCaps
               READ outputCaps
               NOTIFY outputCapsChanged)
    Q_PROPERTY(int bitrate
               READ bitrate
               WRITE setBitrate
               RESET resetBitrate
               NOTIFY bitrateChanged)
    Q_PROPERTY(AkCompressedPackets headers
               READ headers
               NOTIFY headersChanged)
    Q_PROPERTY(qint64 encodedTimePts
               READ encodedTimePts
               NOTIFY encodedTimePtsChanged)

    public:
        explicit AkAudioEncoder(QObject *parent=nullptr);
        ~AkAudioEncoder();

        Q_INVOKABLE virtual AkAudioEncoderCodecID codec() const = 0;
        Q_INVOKABLE AkAudioCaps inputCaps() const;
        Q_INVOKABLE virtual AkCompressedAudioCaps outputCaps() const = 0;
        Q_INVOKABLE int bitrate() const;
        Q_INVOKABLE virtual AkCompressedPackets headers() const;
        Q_INVOKABLE virtual qint64 encodedTimePts() const = 0;

    private:
        AkAudioEncoderPrivate *d;

    Q_SIGNALS:
        void inputCapsChanged(const AkAudioCaps &inputCaps);
        void outputCapsChanged(const AkCompressedAudioCaps &outputCaps);
        void bitrateChanged(int bitrate);
        void headersChanged(const AkCompressedPackets &headers);
        void encodedTimePtsChanged(qint64 encodedTimePts);

    public Q_SLOTS:
        void setInputCaps(const AkAudioCaps &inputCaps);
        void setBitrate(int bitrate);
        void resetInputCaps();
        void resetBitrate();
        virtual void resetOptions();
};

#endif // AKAUDIOENCODER_H
