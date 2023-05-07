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

#ifndef AKSUBTITLEPACKET_H
#define AKSUBTITLEPACKET_H

#include "akpacketbase.h"

class AkSubtitlePacketPrivate;
class AkSubtitleCaps;
class AkPacket;

class AKCOMMONS_EXPORT AkSubtitlePacket: public AkPacketBase
{
    Q_OBJECT
    Q_PROPERTY(AkSubtitleCaps caps
               READ caps
               CONSTANT)
    Q_PROPERTY(size_t size
               READ size
               CONSTANT)

    public:
        AkSubtitlePacket(QObject *parent=nullptr);
        AkSubtitlePacket(const AkSubtitleCaps &caps,
                         size_t size,
                         bool initialized=false);
        AkSubtitlePacket(const AkPacket &other);
        AkSubtitlePacket(const AkSubtitlePacket &other);
        ~AkSubtitlePacket();
        AkSubtitlePacket &operator =(const AkPacket &other);
        AkSubtitlePacket &operator =(const AkSubtitlePacket &other);
        operator bool() const;
        operator AkPacket() const;

        Q_INVOKABLE const AkSubtitleCaps &caps() const;
        Q_INVOKABLE char *data() const;
        Q_INVOKABLE const char *constData() const;
        Q_INVOKABLE size_t size() const;

    private:
        AkSubtitlePacketPrivate *d;
        void dataChanged(const QByteArray &data);

    public Q_SLOTS:
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkSubtitlePacket &packet);

Q_DECLARE_METATYPE(AkSubtitlePacket)

#endif // AKSUBTITLEPACKET_H
