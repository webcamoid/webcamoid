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

#ifndef AKVIDEOENCODER_H
#define AKVIDEOENCODER_H

#include "akelement.h"
#include "../akcompressedpacket.h"
#include "../akcompressedvideocaps.h"
#include "../akcompressedvideopacket.h"

class AkVideoEncoder;
class AkVideoEncoderPrivate;
class AkVideoCaps;

using AkVideoEncoderPtr = QSharedPointer<AkVideoEncoder>;
using AkVideoEncoderCodecID = AkCompressedVideoCaps::VideoCodecID;

class AKCOMMONS_EXPORT AkVideoEncoder: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(AkVideoEncoderCodecID codec
               READ codec
               CONSTANT)
    Q_PROPERTY(AkVideoCaps inputCaps
               READ inputCaps
               WRITE setInputCaps
               RESET resetInputCaps
               NOTIFY inputCapsChanged)
    Q_PROPERTY(AkCompressedVideoCaps outputCaps
               READ outputCaps
               NOTIFY outputCapsChanged)
    Q_PROPERTY(int bitrate
               READ bitrate
               WRITE setBitrate
               RESET resetBitrate
               NOTIFY bitrateChanged)
    Q_PROPERTY(int gop
               READ gop
               WRITE setGop
               RESET resetGop
               NOTIFY gopChanged)
    Q_PROPERTY(AkCompressedPackets headers
               READ headers
               NOTIFY headersChanged)
    Q_PROPERTY(bool fillGaps
               READ fillGaps
               WRITE setFillGaps
               RESET resetFillGaps
               NOTIFY fillGapsChanged)

    public:
        explicit AkVideoEncoder(QObject *parent=nullptr);
        ~AkVideoEncoder();

        Q_INVOKABLE virtual AkVideoEncoderCodecID codec() const = 0;
        Q_INVOKABLE AkVideoCaps inputCaps() const;
        Q_INVOKABLE virtual AkCompressedVideoCaps outputCaps() const = 0;
        Q_INVOKABLE int bitrate() const;
        Q_INVOKABLE int gop() const;
        Q_INVOKABLE virtual AkCompressedPackets headers() const;
        Q_INVOKABLE bool fillGaps() const;

    private:
        AkVideoEncoderPrivate *d;

    Q_SIGNALS:
        void inputCapsChanged(const AkVideoCaps &inputCaps);
        void outputCapsChanged(const AkCompressedVideoCaps &outputCaps);
        void bitrateChanged(int bitrate);
        void gopChanged(int gop);
        void headersChanged(const AkCompressedPackets &headers);
        void fillGapsChanged(bool fillGaps);

    public Q_SLOTS:
        void setInputCaps(const AkVideoCaps &inputCaps);
        void setBitrate(int bitrate);
        void setGop(int gop);
        void setFillGaps(bool fillGaps);
        void resetInputCaps();
        void resetBitrate();
        void resetGop();
        void resetFillGaps();
        virtual void resetOptions();
};

#endif // AKVIDEOENCODER_H
