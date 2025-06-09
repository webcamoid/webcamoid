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

#ifndef AKCOMPRESSEDPACKET_H
#define AKCOMPRESSEDPACKET_H

#include "akpacketbase.h"

class AkCompressedPacketPrivate;
class AkCompressedCaps;
class AkCompressedPacket;
class AkCompressedAudioPacket;
class AkCompressedVideoPacket;

using AkCompressedPackets = QVector<AkCompressedPacket>;

class AKCOMMONS_EXPORT AkCompressedPacket: public AkPacketBase
{
    Q_OBJECT
    Q_PROPERTY(AkCompressedCaps caps
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
            PacketType_Unknown = -1,
            PacketType_Audio,
            PacketType_Video,
        };
        Q_ENUM(PacketType)

        AkCompressedPacket(QObject *parent=nullptr);
        AkCompressedPacket(const AkCompressedPacket &other);
        ~AkCompressedPacket();
        AkCompressedPacket &operator =(const AkCompressedPacket &other);
        operator bool() const;

        Q_INVOKABLE PacketType type() const;
        Q_INVOKABLE AkCompressedCaps caps() const;
        Q_INVOKABLE char *data() const;
        Q_INVOKABLE const char *constData() const;
        Q_INVOKABLE size_t size() const;

    private:
        AkCompressedPacketPrivate *d;

        using DataCopy = void *(*)(void *data);
        using DataDeleter = void (*)(void *data);
        void *privateData() const;
        void setPrivateData(void *data,
                            DataCopy copyFunc,
                            DataDeleter deleterFunc);
        void setType(PacketType type);

    public Q_SLOTS:
        static void registerTypes();

    friend QDebug operator <<(QDebug debug, const AkCompressedPacket &packet);
    friend class AkCompressedPacketPrivate;
    friend class AkCompressedAudioPacket;
    friend class AkCompressedVideoPacket;
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkCompressedPacket &packet);

Q_DECLARE_METATYPE(AkCompressedPacket)
Q_DECLARE_METATYPE(AkCompressedPacket::PacketType)

#endif // AKCOMPRESSEDPACKET_H
