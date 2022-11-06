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

#ifndef AKVIDEOPACKET_H
#define AKVIDEOPACKET_H

#include "akpacketbase.h"

class AkVideoPacketPrivate;
class AkVideoCaps;
class AkPacket;

class AKCOMMONS_EXPORT AkVideoPacket: public AkPacketBase
{
    Q_OBJECT
    Q_PROPERTY(AkVideoCaps caps
               READ caps
               CONSTANT)
    Q_PROPERTY(size_t size
               READ size
               CONSTANT)
    Q_PROPERTY(size_t planes
               READ planes
               CONSTANT)

    public:
        AkVideoPacket(QObject *parent=nullptr);
        AkVideoPacket(const AkVideoCaps &caps,
                      bool initialized=false,
                      size_t align=32);
        AkVideoPacket(const AkPacket &other);
        AkVideoPacket(const AkVideoPacket &other);
        ~AkVideoPacket();
        AkVideoPacket &operator =(const AkPacket &other);
        AkVideoPacket &operator =(const AkVideoPacket &other);
        operator bool() const;
        operator AkPacket() const;

        Q_INVOKABLE const AkVideoCaps &caps() const;
        Q_INVOKABLE size_t size() const;
        Q_INVOKABLE size_t planes() const;
        Q_INVOKABLE size_t planeSize(int plane) const;
        Q_INVOKABLE size_t lineSize(int plane) const;
        Q_INVOKABLE size_t bytesUsed(int plane) const;
        Q_INVOKABLE size_t widthDiv(int plane) const;
        Q_INVOKABLE size_t heightDiv(int plane) const;
        Q_INVOKABLE const char *constData() const;
        Q_INVOKABLE char *data();
        Q_INVOKABLE const quint8 *constPlane(int plane) const;
        Q_INVOKABLE quint8 *plane(int plane);
        Q_INVOKABLE const quint8 *constLine(int plane, int y) const;
        Q_INVOKABLE quint8 *line(int plane, int y);
        Q_INVOKABLE AkVideoPacket copy(int x,
                                       int y,
                                       int width,
                                       int height) const;

    private:
        AkVideoPacketPrivate *d;

    public Q_SLOTS:
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkVideoPacket &packet);

Q_DECLARE_METATYPE(AkVideoPacket)

#endif // AKVIDEOPACKET_H
