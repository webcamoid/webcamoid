/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include "akpacket.h"
#include "akaudiocaps.h"

class AkAudioPacketPrivate;

class AkAudioPacket: public AkPacket
{
    Q_OBJECT
    Q_PROPERTY(AkAudioCaps caps
               READ caps
               WRITE setCaps
               RESET resetCaps
               NOTIFY capsChanged)

    public:
        explicit AkAudioPacket(QObject *parent=NULL);
        AkAudioPacket(const AkAudioCaps &caps,
                      const QByteArray &buffer=QByteArray(),
                      qint64 pts=0,
                      const AkFrac &timeBase=AkFrac(),
                      int index=-1,
                      qint64 id=-1);
        AkAudioPacket(const AkPacket &other);
        AkAudioPacket(const AkAudioPacket &other);
        ~AkAudioPacket();
        AkAudioPacket &operator =(const AkPacket &other);
        AkAudioPacket &operator =(const AkAudioPacket &other);
        operator bool() const;

        Q_INVOKABLE AkAudioCaps caps() const;
        Q_INVOKABLE AkAudioCaps &caps();

        Q_INVOKABLE QString toString() const;
        Q_INVOKABLE AkPacket toPacket() const;

    private:
        AkAudioPacketPrivate *d;

    Q_SIGNALS:
        void capsChanged(const AkAudioCaps &caps);

    public Q_SLOTS:
        void setCaps(const AkAudioCaps &caps);
        void resetCaps();

        friend QDebug operator <<(QDebug debug, const AkAudioPacket &packet);
};

QDebug operator <<(QDebug debug, const AkAudioPacket &packet);

Q_DECLARE_METATYPE(AkAudioPacket)

#endif // AKAUDIOPACKET_H
