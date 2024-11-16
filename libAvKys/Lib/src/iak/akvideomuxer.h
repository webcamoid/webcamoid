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

#ifndef AKVIDEOMUXER_H
#define AKVIDEOMUXER_H

#include "akelement.h"
#include "../akcompressedcaps.h"
#include "../akcompressedpacket.h"

class AkVideoMuxer;
class AkVideoMuxerPrivate;

using AkVideoMuxerPtr = QSharedPointer<AkVideoMuxer>;
using AkCodecType = AkCompressedCaps::CapsType;

class AKCOMMONS_EXPORT AkVideoMuxer: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(FormatID formatID
               READ formatID
               CONSTANT)
    Q_PROPERTY(QString extension
               READ extension
               CONSTANT)
    Q_PROPERTY(QString location
               READ location
               WRITE setLocation
               RESET resetLocation
               NOTIFY locationChanged)

    public:
        enum FormatID
        {
            FormatID_unknown = AK_MAKE_FOURCC(0, 0, 0, 0),
            FormatID_webm    = AK_MAKE_FOURCC('W', 'E', 'B', 'M'),
            FormatID_ogg     = AK_MAKE_FOURCC('O', 'G', 'G', 0),
            FormatID_mp4     = AK_MAKE_FOURCC('M', 'P', 4, 0),
            FormatID_m4v     = AK_MAKE_FOURCC('M', 4, 'V', 0),
            FormatID_3gp     = AK_MAKE_FOURCC(3, 'G', 'P', 0),
            FormatID_3g2     = AK_MAKE_FOURCC(3, 'G', 2, 0),
            FormatID_mov     = AK_MAKE_FOURCC('M', 'O', 'V', 0),
        };
        Q_ENUM(FormatID)

        explicit AkVideoMuxer(QObject *parent=nullptr);
        ~AkVideoMuxer();

        Q_INVOKABLE virtual FormatID formatID() const = 0;
        Q_INVOKABLE virtual QString extension() const = 0;
        Q_INVOKABLE QString location() const;
        Q_INVOKABLE virtual QList<AkCodecID> supportedCodecs(AkCodecType type) const = 0;
        Q_INVOKABLE virtual AkCodecID defaultCodec(AkCodecType type) const = 0;
        Q_INVOKABLE AkCompressedCaps streamCaps(AkCodecType type) const;
        Q_INVOKABLE AkCompressedPackets streamHeaders(AkCodecType type) const;

    private:
        AkVideoMuxerPrivate *d;

    Q_SIGNALS:
        void locationChanged(const QString &location);
        void streamCapsUpdated();
        void streamHeadersUpdated();

    public Q_SLOTS:
        void setStreamCaps(const AkCompressedCaps &caps);
        void setStreamHeaders(AkCodecType type, const AkCompressedPackets &headers);
        void setLocation(const QString &location);
        void resetLocation();
        virtual void resetOptions();
};

#endif // AKVIDEOMUXER_H
