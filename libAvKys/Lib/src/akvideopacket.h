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

#include "akvideocaps.h"

class AkVideoPacketPrivate;
class AkPacket;

class AKCOMMONS_EXPORT AkVideoPacket: public QObject
{
    Q_OBJECT
    Q_PROPERTY(AkVideoCaps caps
               READ caps
               WRITE setCaps
               RESET resetCaps
               NOTIFY capsChanged)
    Q_PROPERTY(QByteArray buffer
               READ buffer
               WRITE setBuffer
               RESET resetBuffer
               NOTIFY bufferChanged)
    Q_PROPERTY(qint64 id
               READ id
               WRITE setId
               RESET resetId
               NOTIFY idChanged)
    Q_PROPERTY(qint64 pts
               READ pts
               WRITE setPts
               RESET resetPts
               NOTIFY ptsChanged)
    Q_PROPERTY(AkFrac timeBase
               READ timeBase
               WRITE setTimeBase
               RESET resetTimeBase
               NOTIFY timeBaseChanged)
    Q_PROPERTY(int index
               READ index
               WRITE setIndex
               RESET resetIndex
               NOTIFY indexChanged)

    public:
        AkVideoPacket(QObject *parent=nullptr);
        AkVideoPacket(const AkVideoCaps &caps);
        AkVideoPacket(const AkPacket &other);
        AkVideoPacket(const AkVideoPacket &other);
        ~AkVideoPacket();
        AkVideoPacket &operator =(const AkPacket &other);
        AkVideoPacket &operator =(const AkVideoPacket &other);
        operator bool() const;
        operator AkPacket() const;

        Q_INVOKABLE AkVideoCaps caps() const;
        Q_INVOKABLE AkVideoCaps &caps();
        Q_INVOKABLE QByteArray buffer() const;
        Q_INVOKABLE QByteArray &buffer();
        Q_INVOKABLE qint64 id() const;
        Q_INVOKABLE qint64 &id();
        Q_INVOKABLE qint64 pts() const;
        Q_INVOKABLE qint64 &pts();
        Q_INVOKABLE AkFrac timeBase() const;
        Q_INVOKABLE AkFrac &timeBase();
        Q_INVOKABLE int index() const;
        Q_INVOKABLE int &index();
        Q_INVOKABLE void copyMetadata(const AkVideoPacket &other);

        Q_INVOKABLE const quint8 *constLine(int plane, int y) const;
        Q_INVOKABLE quint8 *line(int plane, int y);
        Q_INVOKABLE QImage toImage() const;
        Q_INVOKABLE static AkVideoPacket fromImage(const QImage &image,
                                                   const AkVideoPacket &defaultPacket);
        Q_INVOKABLE static bool canConvert(AkVideoCaps::PixelFormat input,
                                           AkVideoCaps::PixelFormat output);
        Q_INVOKABLE bool canConvert(AkVideoCaps::PixelFormat output) const;
        Q_INVOKABLE AkVideoPacket convert(AkVideoCaps::PixelFormat format) const;
        Q_INVOKABLE AkVideoPacket convert(AkVideoCaps::PixelFormat format,
                                          int align) const;
        Q_INVOKABLE AkVideoPacket scaled(int width, int height) const;
        Q_INVOKABLE AkVideoPacket realign(int align) const;

    private:
        AkVideoPacketPrivate *d;

    Q_SIGNALS:
        void capsChanged(const AkVideoCaps &caps);
        void bufferChanged(const QByteArray &buffer);
        void idChanged(qint64 id);
        void ptsChanged(qint64 pts);
        void timeBaseChanged(const AkFrac &timeBase);
        void indexChanged(int index);

    public Q_SLOTS:
        void setCaps(const AkVideoCaps &caps);
        void setBuffer(const QByteArray &buffer);
        void setId(qint64 id);
        void setPts(qint64 pts);
        void setTimeBase(const AkFrac &timeBase);
        void setIndex(int index);
        void resetCaps();
        void resetBuffer();
        void resetId();
        void resetPts();
        void resetTimeBase();
        void resetIndex();
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkVideoPacket &packet);

Q_DECLARE_METATYPE(AkVideoPacket)

#endif // AKVIDEOPACKET_H
