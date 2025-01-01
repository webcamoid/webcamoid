/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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

#ifndef AKCOMPRESSEDVIDEOCAPS_H
#define AKCOMPRESSEDVIDEOCAPS_H

#include <QObject>

#include "akcommons.h"

class AkCompressedVideoCapsPrivate;
class AkCaps;
class AkCompressedCaps;
class AkVideoCaps;

class AKCOMMONS_EXPORT AkCompressedVideoCaps: public QObject
{
    Q_OBJECT
    Q_PROPERTY(VideoCodecID codec
               READ codec
               WRITE setCodec
               RESET resetCodec
               NOTIFY codecChanged)
    Q_PROPERTY(AkVideoCaps rawCaps
               READ rawCaps
               WRITE setRawCaps
               RESET resetRawCaps
               NOTIFY rawCapsChanged)
    Q_PROPERTY(int bitrate
               READ bitrate
               WRITE setBitrate
               RESET resetBitrate
               NOTIFY bitrateChanged)

    public:
        enum VideoCodecID
        {
            VideoCodecID_unknown = AK_MAKE_FOURCC(0, 0, 0, 0),
            VideoCodecID_av1     = AK_MAKE_FOURCC('A', 'V', '1', 0),
            VideoCodecID_h264    = AK_MAKE_FOURCC('A', 'V', 'C', 0),
            VideoCodecID_hevc    = AK_MAKE_FOURCC('H', 'E', 'V', 'C'),
            VideoCodecID_mjpeg   = AK_MAKE_FOURCC('M', 'J', 'P', 'G'),
            VideoCodecID_mpeg1   = AK_MAKE_FOURCC('M', 'P', 'G', '1'),
            VideoCodecID_mpeg2   = AK_MAKE_FOURCC('M', 'P', 'G', '2'),
            VideoCodecID_mpeg4   = AK_MAKE_FOURCC('M', 'P', 'G', '4'),
            VideoCodecID_theora  = AK_MAKE_FOURCC('T', 'H', 'E', 'O'),
            VideoCodecID_vp8     = AK_MAKE_FOURCC('V', 'P', '8', 0),
            VideoCodecID_vp9     = AK_MAKE_FOURCC('V', 'P', '9', 0),
        };
        Q_ENUM(VideoCodecID)

        AkCompressedVideoCaps(QObject *parent=nullptr);
        AkCompressedVideoCaps(VideoCodecID codec,
                              const AkVideoCaps &rawCaps,
                              int bitrate=0);
        AkCompressedVideoCaps(const AkCaps &other);
        AkCompressedVideoCaps(const AkCompressedCaps &other);
        AkCompressedVideoCaps(const AkCompressedVideoCaps &other);
        ~AkCompressedVideoCaps();
        AkCompressedVideoCaps &operator =(const AkCaps &other);
        AkCompressedVideoCaps &operator =(const AkCompressedCaps &other);
        AkCompressedVideoCaps &operator =(const AkCompressedVideoCaps &other);
        bool operator ==(const AkCompressedVideoCaps &other) const;
        bool operator !=(const AkCompressedVideoCaps &other) const;
        operator bool() const;
        operator AkCaps() const;
        operator AkCompressedCaps() const;

        Q_INVOKABLE static QObject *create();
        Q_INVOKABLE static QObject *create(const AkCaps &caps);
        Q_INVOKABLE static QObject *create(const AkCompressedCaps &caps);
        Q_INVOKABLE static QObject *create(const AkCompressedVideoCaps &caps);
        Q_INVOKABLE static QObject *create(VideoCodecID codec,
                                           const AkVideoCaps &rawCaps,
                                           int bitrate=0);
        Q_INVOKABLE QVariant toVariant() const;

        Q_INVOKABLE VideoCodecID codec() const;
        Q_INVOKABLE AkVideoCaps rawCaps() const;
        Q_INVOKABLE int bitrate() const;

        Q_INVOKABLE static QString videoCodecIDToString(AkCompressedVideoCaps::VideoCodecID codecID);

    private:
        AkCompressedVideoCapsPrivate *d;

    Q_SIGNALS:
        void codecChanged(VideoCodecID codec);
        void rawCapsChanged(const AkVideoCaps &rawCaps);
        void bitrateChanged(int bitrate);

    public Q_SLOTS:
        void setCodec(VideoCodecID codec);
        void setRawCaps(const AkVideoCaps &rawCaps);
        void setBitrate(int bitrate);
        void resetCodec();
        void resetRawCaps();
        void resetBitrate();
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkCompressedVideoCaps &caps);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkCompressedVideoCaps::VideoCodecID codecID);
AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkCompressedVideoCaps &caps);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, const AkCompressedVideoCaps &caps);

Q_DECLARE_METATYPE(AkCompressedVideoCaps)
Q_DECLARE_METATYPE(AkCompressedVideoCaps::VideoCodecID)

#endif // AKCOMPRESSEDVIDEOCAPS_H
