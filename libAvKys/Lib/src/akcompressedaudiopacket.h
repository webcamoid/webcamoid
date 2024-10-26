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

#ifndef ACOMPRESSEDKAUDIOPACKET_H
#define ACOMPRESSEDKAUDIOPACKET_H

#include "akpacketbase.h"

class AkCompressedAudioPacketPrivate;
class AkCompressedAudioCaps;
class AkPacket;

class AKCOMMONS_EXPORT AkCompressedAudioPacket: public AkPacketBase
{
    Q_OBJECT
    Q_PROPERTY(AkCompressedAudioCaps caps
               READ caps
               CONSTANT)
    Q_PROPERTY(size_t size
               READ size
               CONSTANT)

    public:
        AkCompressedAudioPacket(QObject *parent=nullptr);
        AkCompressedAudioPacket(const AkCompressedAudioCaps &caps,
                                size_t size,
                                bool initialized=false);
        AkCompressedAudioPacket(const AkPacket &other);
        AkCompressedAudioPacket(const AkCompressedAudioPacket &other);
        ~AkCompressedAudioPacket();
        AkCompressedAudioPacket &operator =(const AkPacket &other);
        AkCompressedAudioPacket &operator =(const AkCompressedAudioPacket &other);
        operator bool() const;
        operator AkPacket() const;

        Q_INVOKABLE const AkCompressedAudioCaps &caps() const;
        Q_INVOKABLE char *data() const;
        Q_INVOKABLE const char *constData() const;
        Q_INVOKABLE size_t size() const;

    private:
        AkCompressedAudioPacketPrivate *d;

    public Q_SLOTS:
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkCompressedAudioPacket &packet);

Q_DECLARE_METATYPE(AkCompressedAudioPacket)

#endif // AKCOMPRESSEDAUDIOPACKET_H
