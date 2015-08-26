/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef QBVIDEOPACKET_H
#define QBVIDEOPACKET_H

#include "qbpacket.h"
#include "qbvideocaps.h"

class QbVideoPacketPrivate;

class QbVideoPacket: public QbPacket
{
    Q_OBJECT
    Q_PROPERTY(QbVideoCaps caps
               READ caps
               WRITE setCaps
               RESET resetCaps
               NOTIFY capsChanged)

    public:
        explicit QbVideoPacket(QObject *parent=NULL);
        QbVideoPacket(const QbVideoCaps &caps,
                      const QbBufferPtr &buffer=QbBufferPtr(),
                      ulong bufferSize=0,
                      qint64 pts=0,
                      const QbFrac &timeBase=QbFrac(),
                      int index=-1,
                      qint64 id=-1);
        QbVideoPacket(const QbPacket &other);
        QbVideoPacket(const QbVideoPacket &other);
        ~QbVideoPacket();
        QbVideoPacket &operator =(const QbPacket &other);
        QbVideoPacket &operator =(const QbVideoPacket &other);
        operator bool() const;

        Q_INVOKABLE QbVideoCaps caps() const;
        Q_INVOKABLE QbVideoCaps &caps();

        Q_INVOKABLE QString toString() const;
        Q_INVOKABLE QbPacket toPacket() const;

    private:
        QbVideoPacketPrivate *d;

    signals:
        void capsChanged(const QbVideoCaps &caps);

    public slots:
        void setCaps(const QbVideoCaps &caps);
        void resetCaps();

        friend QDebug operator <<(QDebug debug, const QbVideoPacket &packet);
};

QDebug operator <<(QDebug debug, const QbVideoPacket &packet);

Q_DECLARE_METATYPE(QbVideoPacket)

#endif // QBVIDEOPACKET_H
