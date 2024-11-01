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
class AkFrac;

class AKCOMMONS_EXPORT AkCompressedAudioCaps: public QObject
{
    Q_OBJECT
    Q_PROPERTY(AudioCodecID codec
               READ codec
               WRITE setCodec
               RESET resetCodec
               NOTIFY codecChanged)
    Q_PROPERTY(int bps
               READ bps
               WRITE setBps
               RESET resetBps
               NOTIFY bpsChanged)
    Q_PROPERTY(int channels
               READ channels
               WRITE setChannels
               RESET resetChannels
               NOTIFY channelsChanged)
    Q_PROPERTY(int rate
               READ rate
               WRITE setRate
               RESET resetRate
               NOTIFY rateChanged)

    public:
        enum AudioCodecID
        {
            AudioCodecID_unknown = AK_MAKE_FOURCC(0, 0, 0, 0),
            AudioCodecID_aac     = AK_MAKE_FOURCC('A', 'A', 'C', 0),
            AudioCodecID_ac3     = AK_MAKE_FOURCC('A', 'C', '3', 0),
            AudioCodecID_flac    = AK_MAKE_FOURCC('F', 'L', 'A', 'C'),
            AudioCodecID_mp2     = AK_MAKE_FOURCC('M', 'P', '2', 0),
            AudioCodecID_mp3     = AK_MAKE_FOURCC('M', 'P', '3', 0),
            AudioCodecID_opus    = AK_MAKE_FOURCC('O', 'P', 'U', 'S'),
            AudioCodecID_speex   = AK_MAKE_FOURCC('S', 'P', 'E', 'X'),
            AudioCodecID_vorbis  = AK_MAKE_FOURCC('V', 'O', 'R', 'B'),
        };
        Q_ENUM(AudioCodecID)

        AkCompressedAudioCaps(QObject *parent=nullptr);
        AkCompressedAudioCaps(AudioCodecID codec,
                              int bps,
                              int channels,
                              int rate);
        AkCompressedAudioCaps(const AkCaps &other);
        AkCompressedAudioCaps(const AkCompressedAudioCaps &other);
        ~AkCompressedAudioCaps();
        AkCompressedAudioCaps &operator =(const AkCaps &other);
        AkCompressedAudioCaps &operator =(const AkCompressedAudioCaps &other);
        bool operator ==(const AkCompressedAudioCaps &other) const;
        bool operator !=(const AkCompressedAudioCaps &other) const;
        operator bool() const;
        operator AkCaps() const;

        Q_INVOKABLE static QObject *create();
        Q_INVOKABLE static QObject *create(const AkCaps &caps);
        Q_INVOKABLE static QObject *create(const AkCompressedAudioCaps &caps);
        Q_INVOKABLE static QObject *create(AudioCodecID codec,
                                           int bps,
                                           int channels,
                                           int rate);
        Q_INVOKABLE QVariant toVariant() const;

        Q_INVOKABLE AudioCodecID codec() const;
        Q_INVOKABLE int bps() const;
        Q_INVOKABLE int channels() const;
        Q_INVOKABLE int rate() const;

        Q_INVOKABLE static QString audioCodecIDToString(AkCompressedAudioCaps::AudioCodecID codecID);

    private:
        AkCompressedAudioCapsPrivate *d;

    Q_SIGNALS:
        void codecChanged(AudioCodecID codec);
        void bpsChanged(int bps);
        void channelsChanged(int channels);
        void rateChanged(int rate);

    public Q_SLOTS:
        void setCodec(AudioCodecID codec);
        void setBps(int bps);
        void setChannels(int channels);
        void setRate(int rate);
        void resetCodec();
        void resetBps();
        void resetChannels();
        void resetRate();
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkCompressedAudioCaps &caps);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkCompressedAudioCaps::AudioCodecID codecID);
AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkCompressedAudioCaps &caps);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, const AkCompressedAudioCaps &caps);

Q_DECLARE_METATYPE(AkCompressedAudioCaps)
Q_DECLARE_METATYPE(AkCompressedAudioCaps::AudioCodecID)

#endif // AKCOMPRESSEDAUDIOCAPS_H
