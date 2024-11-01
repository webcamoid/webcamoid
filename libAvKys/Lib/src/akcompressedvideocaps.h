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
class AkFrac;

class AKCOMMONS_EXPORT AkCompressedVideoCaps: public QObject
{
    Q_OBJECT
    Q_PROPERTY(VideoCodecID codec
               READ codec
               WRITE setCodec
               RESET resetCodec
               NOTIFY codecChanged)
    Q_PROPERTY(int width
               READ width
               WRITE setWidth
               RESET resetWidth
               NOTIFY widthChanged)
    Q_PROPERTY(int height
               READ height
               WRITE setHeight
               RESET resetHeight
               NOTIFY heightChanged)
    Q_PROPERTY(AkFrac fps
               READ fps
               WRITE setFps
               RESET resetFps
               NOTIFY fpsChanged)

    public:
        enum VideoCodecID
        {
            VideoCodecID_unknown = AK_MAKE_FOURCC(0, 0, 0, 0),
            VideoCodecID_av1     = AK_MAKE_FOURCC('A', 'V', '1', 0),
            VideoCodecID_h264    = AK_MAKE_FOURCC('H', '2', '6', '4'),
            VideoCodecID_hevc    = AK_MAKE_FOURCC('H', 'E', 'V', 'C'),
            VideoCodecID_mjpeg   = AK_MAKE_FOURCC('M', 'J', 'P', 'G'),
            VideoCodecID_mpeg1   = AK_MAKE_FOURCC('M', 'P', 'G', '1'),
            VideoCodecID_mpeg2   = AK_MAKE_FOURCC('M', 'P', 'G', '2'),
            VideoCodecID_mpeg4p2 = AK_MAKE_FOURCC('M', 'P', 'G', 42),
            VideoCodecID_theora  = AK_MAKE_FOURCC('T', 'H', 'E', 'O'),
            VideoCodecID_vp8     = AK_MAKE_FOURCC('V', 'P', '8', 0),
            VideoCodecID_vp9     = AK_MAKE_FOURCC('V', 'P', '9', 0),
        };
        Q_ENUM(VideoCodecID)

        AkCompressedVideoCaps(QObject *parent=nullptr);
        AkCompressedVideoCaps(VideoCodecID codec,
                              int width,
                              int height,
                              const AkFrac &fps);
        AkCompressedVideoCaps(VideoCodecID codec,
                              const QSize &size,
                              const AkFrac &fps);
        AkCompressedVideoCaps(const AkCaps &other);
        AkCompressedVideoCaps(const AkCompressedVideoCaps &other);
        ~AkCompressedVideoCaps();
        AkCompressedVideoCaps &operator =(const AkCaps &other);
        AkCompressedVideoCaps &operator =(const AkCompressedVideoCaps &other);
        bool operator ==(const AkCompressedVideoCaps &other) const;
        bool operator !=(const AkCompressedVideoCaps &other) const;
        operator bool() const;
        operator AkCaps() const;

        Q_INVOKABLE static QObject *create();
        Q_INVOKABLE static QObject *create(const AkCaps &caps);
        Q_INVOKABLE static QObject *create(const AkCompressedVideoCaps &caps);
        Q_INVOKABLE static QObject *create(VideoCodecID codec,
                                           int width,
                                           int height,
                                           const AkFrac &fps);
        Q_INVOKABLE static QObject *create(VideoCodecID codec,
                                           const QSize &size,
                                           const AkFrac &fps);
        Q_INVOKABLE QVariant toVariant() const;

        Q_INVOKABLE VideoCodecID codec() const;
        Q_INVOKABLE int width() const;
        Q_INVOKABLE int height() const;
        Q_INVOKABLE AkFrac fps() const;

        Q_INVOKABLE static QString videoCodecIDToString(AkCompressedVideoCaps::VideoCodecID codecID);

    private:
        AkCompressedVideoCapsPrivate *d;

    Q_SIGNALS:
        void codecChanged(VideoCodecID codec);
        void sizeChanged(const QSize &size);
        void widthChanged(int width);
        void heightChanged(int height);
        void fpsChanged(const AkFrac &fps);

    public Q_SLOTS:
        void setCodec(VideoCodecID codec);
        void setSize(const QSize &size);
        void setWidth(int width);
        void setHeight(int height);
        void setFps(const AkFrac &fps);
        void resetCodec();
        void resetSize();
        void resetWidth();
        void resetHeight();
        void resetFps();
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkCompressedVideoCaps &caps);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkCompressedVideoCaps::VideoCodecID codecID);
AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkCompressedVideoCaps &caps);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, const AkCompressedVideoCaps &caps);

Q_DECLARE_METATYPE(AkCompressedVideoCaps)
Q_DECLARE_METATYPE(AkCompressedVideoCaps::VideoCodecID)

#endif // AKCOMPRESSEDVIDEOCAPS_H
