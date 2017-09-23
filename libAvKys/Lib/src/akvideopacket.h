/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include "akpacket.h"
#include "akvideocaps.h"

class AkVideoPacketPrivate;

class AKCOMMONS_EXPORT AkVideoPacket: public AkPacket
{
    Q_OBJECT
    Q_PROPERTY(AkVideoCaps caps
               READ caps
               WRITE setCaps
               RESET resetCaps
               NOTIFY capsChanged)

    public:
        explicit AkVideoPacket(QObject *parent=nullptr);
        AkVideoPacket(const AkVideoCaps &caps,
                      const QByteArray &buffer=QByteArray(),
                      qint64 pts=0,
                      const AkFrac &timeBase=AkFrac(),
                      int index=-1,
                      qint64 id=-1);
        AkVideoPacket(const AkPacket &other);
        AkVideoPacket(const AkVideoPacket &other);
        ~AkVideoPacket();
        AkVideoPacket &operator =(const AkPacket &other);
        AkVideoPacket &operator =(const AkVideoPacket &other);
        operator bool() const;

        Q_INVOKABLE AkVideoCaps caps() const;
        Q_INVOKABLE AkVideoCaps &caps();

        Q_INVOKABLE QString toString() const;
        Q_INVOKABLE AkPacket toPacket() const;

    private:
        AkVideoPacketPrivate *d;

    Q_SIGNALS:
        void capsChanged(const AkVideoCaps &caps);

    public Q_SLOTS:
        void setCaps(const AkVideoCaps &caps);
        void resetCaps();

        friend QDebug operator <<(QDebug debug, const AkVideoPacket &packet);
};

QDebug operator <<(QDebug debug, const AkVideoPacket &packet);

Q_DECLARE_METATYPE(AkVideoPacket)

#endif // AKVIDEOPACKET_H
