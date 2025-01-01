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

#ifndef AKCOMPRESSEDAUDIOCAPS_H
#define AKCOMPRESSEDAUDIOCAPS_H

#include <QObject>

#include "akcommons.h"

class AkCompressedAudioCapsPrivate;
class AkCaps;
class AkCompressedCaps;
class AkAudioCaps;

class AKCOMMONS_EXPORT AkCompressedAudioCaps: public QObject
{
    Q_OBJECT
    Q_PROPERTY(AudioCodecID codec
               READ codec
               WRITE setCodec
               RESET resetCodec
               NOTIFY codecChanged)
    Q_PROPERTY(AkAudioCaps rawCaps
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
        enum AudioCodecID
        {
            AudioCodecID_unknown = AK_MAKE_FOURCC(0, 0, 0, 0),
            AudioCodecID_aac     = AK_MAKE_FOURCC('A', 'A', 'C', 0),
            AudioCodecID_ac3     = AK_MAKE_FOURCC('A', 'C', '3', 0),
            AudioCodecID_amrnb   = AK_MAKE_FOURCC('A', 'M', 'R', 'N'),
            AudioCodecID_flac    = AK_MAKE_FOURCC('F', 'L', 'A', 'C'),
            AudioCodecID_mpeg1   = AK_MAKE_FOURCC('M', 'P', '1', 0),
            AudioCodecID_mpeg2   = AK_MAKE_FOURCC('M', 'P', '2', 0),
            AudioCodecID_mp3     = AK_MAKE_FOURCC('M', 'P', '3', 0),
            AudioCodecID_opus    = AK_MAKE_FOURCC('O', 'P', 'U', 'S'),
            AudioCodecID_speex   = AK_MAKE_FOURCC('S', 'P', 'E', 'X'),
            AudioCodecID_vorbis  = AK_MAKE_FOURCC('V', 'O', 'R', 'B'),
        };
        Q_ENUM(AudioCodecID)

        AkCompressedAudioCaps(QObject *parent=nullptr);
        AkCompressedAudioCaps(AudioCodecID codec,
                              const AkAudioCaps &rawCaps,
                              int bitrate=0);
        AkCompressedAudioCaps(const AkCaps &other);
        AkCompressedAudioCaps(const AkCompressedCaps &other);
        AkCompressedAudioCaps(const AkCompressedAudioCaps &other);
        ~AkCompressedAudioCaps();
        AkCompressedAudioCaps &operator =(const AkCaps &other);
        AkCompressedAudioCaps &operator =(const AkCompressedCaps &other);
        AkCompressedAudioCaps &operator =(const AkCompressedAudioCaps &other);
        bool operator ==(const AkCompressedAudioCaps &other) const;
        bool operator !=(const AkCompressedAudioCaps &other) const;
        operator bool() const;
        operator AkCaps() const;
        operator AkCompressedCaps() const;

        Q_INVOKABLE static QObject *create();
        Q_INVOKABLE static QObject *create(const AkCaps &caps);
        Q_INVOKABLE static QObject *create(const AkCompressedCaps &caps);
        Q_INVOKABLE static QObject *create(const AkCompressedAudioCaps &caps);
        Q_INVOKABLE static QObject *create(AudioCodecID codec,
                                           const AkAudioCaps &rawCaps,
                                           int bitrate=0);
        Q_INVOKABLE QVariant toVariant() const;

        Q_INVOKABLE AudioCodecID codec() const;
        Q_INVOKABLE AkAudioCaps rawCaps() const;
        Q_INVOKABLE int bitrate() const;

        Q_INVOKABLE static QString audioCodecIDToString(AkCompressedAudioCaps::AudioCodecID codecID);

    private:
        AkCompressedAudioCapsPrivate *d;

    Q_SIGNALS:
        void codecChanged(AudioCodecID codec);
        void rawCapsChanged(const AkAudioCaps &rawCaps);
        void bitrateChanged(int bitrate);

    public Q_SLOTS:
        void setCodec(AudioCodecID codec);
        void setRawCaps(const AkAudioCaps &rawCaps);
        void setBitrate(int bitrate);
        void resetCodec();
        void resetRawCaps();
        void resetBitrate();
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkCompressedAudioCaps &caps);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkCompressedAudioCaps::AudioCodecID codecID);
AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkCompressedAudioCaps &caps);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, const AkCompressedAudioCaps &caps);

Q_DECLARE_METATYPE(AkCompressedAudioCaps)
Q_DECLARE_METATYPE(AkCompressedAudioCaps::AudioCodecID)

#endif // AKCOMPRESSEDAUDIOCAPS_H
