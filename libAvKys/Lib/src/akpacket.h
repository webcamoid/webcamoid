/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#ifndef AKPACKET_H
#define AKPACKET_H

#include "akpacketbase.h"

class AkPacketPrivate;
class AkAudioPacket;
class AkCaps;
class AkCompressedAudioPacket;
class AkCompressedVideoPacket;
class AkSubtitlePacket;
class AkVideoPacket;

class AKCOMMONS_EXPORT AkPacket: public AkPacketBase
{
    Q_OBJECT
    Q_PROPERTY(AkCaps caps
               READ caps
               CONSTANT)
    Q_PROPERTY(PacketType type
               READ type
               CONSTANT)
    Q_PROPERTY(size_t size
               READ size
               CONSTANT)

    public:
        enum PacketType
        {
            PacketUnknown = -1,
            PacketAudio,
            PacketAudioCompressed,
            PacketVideo,
            PacketVideoCompressed,
            PacketSubtitle
        };
        Q_ENUM(PacketType)

        AkPacket(QObject *parent=nullptr);
        AkPacket(const AkPacket &other);
        ~AkPacket();
        AkPacket &operator =(const AkPacket &other);
        operator bool() const;

        Q_INVOKABLE AkPacket::PacketType type() const;
        Q_INVOKABLE AkCaps caps() const;
        Q_INVOKABLE char *data() const;
        Q_INVOKABLE const char *constData() const;
        Q_INVOKABLE size_t size() const;

    private:
        AkPacketPrivate *d;

        using DataCopy = std::function<void *(void *data)>;
        using DataDeleter = std::function<void (void *data)>;
        void *privateData() const;
        void setPrivateData(void *data,
                            DataCopy copyFunc,
                            DataDeleter deleterFunc);
        void setType(AkPacket::PacketType type);

    public Q_SLOTS:
        static void registerTypes();

    friend QDebug operator <<(QDebug debug, const AkPacket &packet);
    friend class AkPacketPrivate;
    friend class AkAudioPacket;
    friend class AkCompressedAudioPacket;
    friend class AkCompressedVideoPacket;
    friend class AkSubtitlePacket;
    friend class AkVideoPacket;
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkPacket &packet);

Q_DECLARE_METATYPE(AkPacket)
Q_DECLARE_METATYPE(AkPacket::PacketType)

#endif // AKPACKET_H
