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

#ifndef AKAUDIOPACKET_H
#define AKAUDIOPACKET_H

#include "akpacketbase.h"

class AkAudioPacketPrivate;
class AkAudioCaps;
class AkPacket;

class AKCOMMONS_EXPORT AkAudioPacket: public AkPacketBase
{
    Q_OBJECT
    Q_PROPERTY(AkAudioCaps caps
               READ caps
               CONSTANT)
    Q_PROPERTY(size_t size
               READ size
               CONSTANT)
    Q_PROPERTY(size_t samples
               READ samples
               CONSTANT)
    Q_PROPERTY(size_t planes
               READ planes
               CONSTANT)

    public:
        AkAudioPacket(QObject *parent=nullptr);
        AkAudioPacket(const AkAudioCaps &caps,
                      size_t samples,
                      bool initialized=false);
        AkAudioPacket(size_t size,
                      const AkAudioCaps &caps,
                      bool initialized=false);
        AkAudioPacket(const AkPacket &other);
        AkAudioPacket(const AkAudioPacket &other);
        ~AkAudioPacket();
        AkAudioPacket &operator =(const AkPacket &other);
        AkAudioPacket &operator =(const AkAudioPacket &other);
        AkAudioPacket operator +(const AkAudioPacket &other);
        AkAudioPacket& operator +=(const AkAudioPacket &other);
        operator bool() const;
        operator AkPacket() const;

        Q_INVOKABLE const AkAudioCaps &caps() const;
        Q_INVOKABLE size_t size() const;
        Q_INVOKABLE size_t samples() const;
        Q_INVOKABLE size_t planes() const;
        Q_INVOKABLE size_t planeSize(int plane) const;
        Q_INVOKABLE const char *constData() const;
        Q_INVOKABLE char *data();
        Q_INVOKABLE const quint8 *constPlane(int plane) const;
        Q_INVOKABLE quint8 *plane(int plane);
        Q_INVOKABLE const quint8 *constSample(int channel, int i) const;
        Q_INVOKABLE quint8 *sample(int channel, int i);
        Q_INVOKABLE void setSample(int channel, int i, const quint8 *sample);
        Q_INVOKABLE AkAudioPacket pop(int samples);

    private:
        AkAudioPacketPrivate *d;

    public Q_SLOTS:
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkAudioPacket &packet);

Q_DECLARE_METATYPE(AkAudioPacket)

#endif // AKAUDIOPACKET_H
