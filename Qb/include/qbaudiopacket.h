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

#ifndef QBAUDIOPACKET_H
#define QBAUDIOPACKET_H

#include "qbpacket.h"
#include "qbaudiocaps.h"

class QbAudioPacketPrivate;

class QbAudioPacket: public QbPacket
{
    Q_OBJECT
    Q_PROPERTY(QbAudioCaps caps
               READ caps
               WRITE setCaps
               RESET resetCaps
               NOTIFY capsChanged)

    public:
        explicit QbAudioPacket(QObject *parent=NULL);
        QbAudioPacket(const QbAudioCaps &caps,
                      const QbBufferPtr &buffer=QbBufferPtr(),
                      ulong bufferSize=0,
                      qint64 pts=0,
                      const QbFrac &timeBase=QbFrac(),
                      int index=-1,
                      qint64 id=-1);
        QbAudioPacket(const QbPacket &other);
        QbAudioPacket(const QbAudioPacket &other);
        ~QbAudioPacket();
        QbAudioPacket &operator =(const QbPacket &other);
        QbAudioPacket &operator =(const QbAudioPacket &other);
        operator bool() const;

        Q_INVOKABLE QbAudioCaps caps() const;
        Q_INVOKABLE QbAudioCaps &caps();

        Q_INVOKABLE QString toString() const;
        Q_INVOKABLE QbPacket toPacket() const;

    private:
        QbAudioPacketPrivate *d;

    signals:
        void capsChanged(const QbAudioCaps &caps);

    public slots:
        void setCaps(const QbAudioCaps &caps);
        void resetCaps();

        friend QDebug operator <<(QDebug debug, const QbAudioPacket &packet);
};

QDebug operator <<(QDebug debug, const QbAudioPacket &packet);

Q_DECLARE_METATYPE(QbAudioPacket)

#endif // QBAUDIOPACKET_H
