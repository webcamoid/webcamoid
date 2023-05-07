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

#ifndef ACOMPRESSEDKVIDEOPACKET_H
#define ACOMPRESSEDKVIDEOPACKET_H

#include "akpacketbase.h"

class AkCompressedVideoPacketPrivate;
class AkCompressedVideoCaps;
class AkPacket;

class AKCOMMONS_EXPORT AkCompressedVideoPacket: public AkPacketBase
{
    Q_OBJECT
    Q_PROPERTY(AkCompressedVideoCaps caps
               READ caps
               CONSTANT)
    Q_PROPERTY(size_t size
               READ size
               CONSTANT)

    public:
        AkCompressedVideoPacket(QObject *parent=nullptr);
        AkCompressedVideoPacket(const AkCompressedVideoCaps &caps,
                                size_t size,
                                bool initialized=false);
        AkCompressedVideoPacket(const AkPacket &other);
        AkCompressedVideoPacket(const AkCompressedVideoPacket &other);
        ~AkCompressedVideoPacket();
        AkCompressedVideoPacket &operator =(const AkPacket &other);
        AkCompressedVideoPacket &operator =(const AkCompressedVideoPacket &other);
        operator bool() const;
        operator AkPacket() const;

        Q_INVOKABLE const AkCompressedVideoCaps &caps() const;
        Q_INVOKABLE char *data() const;
        Q_INVOKABLE const char *constData() const;
        Q_INVOKABLE size_t size() const;

    private:
        AkCompressedVideoPacketPrivate *d;

    public Q_SLOTS:
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkCompressedVideoPacket &packet);

Q_DECLARE_METATYPE(AkCompressedVideoPacket)

#endif // AKCOMPRESSEDVIDEOPACKET_H
