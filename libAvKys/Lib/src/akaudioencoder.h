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
#include "akcompressedaudiocaps.h"

class AkAudioEncoder;
class AkAudioEncoderPrivate;

using AkMultimediaSourceElementPtr = QSharedPointer<AkAudioEncoder>;
using AkAudioEncoderCodecID = AkCompressedAudioCaps::AudioCodecID;

class AKCOMMONS_EXPORT AkAudioEncoder: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QString description
               READ description
               CONSTANT)
    Q_PROPERTY(AkAudioEncoderCodecID codec
               READ codec
               CONSTANT)
    Q_PROPERTY(int bitrate
               READ bitrate
               WRITE setBitrate
               RESET resetBitrate
               NOTIFY bitrateChanged)

    public:
        explicit AkAudioEncoder(QObject *parent=nullptr);
        ~AkAudioEncoder();

        Q_INVOKABLE virtual QString description() const = 0;
        Q_INVOKABLE virtual AkAudioEncoderCodecID codec() const = 0;
        Q_INVOKABLE int bitrate() const;
        Q_INVOKABLE virtual QVariantList controls() const;
        Q_INVOKABLE virtual bool setControls(const QVariantMap &controls);

    private:
        AkAudioEncoderPrivate *d;

    Q_SIGNALS:
        void bitrateChanged(int bitrate);
        void controlsChanged(const QVariantList &controls);

    public Q_SLOTS:
        void setBitrate(int bitrate);
        void resetBitrate();
};

#endif // AKAUDIOENCODER_H
