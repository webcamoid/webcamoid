/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef QBPACKET_H
#define QBPACKET_H

#include <QSharedPointer>

#include "qbcaps.h"
#include "qbfrac.h"

class QbPacketPrivate;

typedef QSharedPointer<char> QbBufferPtr;

class QbPacket: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QbCaps caps
               READ caps
               WRITE setCaps
               RESET resetCaps
               NOTIFY capsChanged)
    Q_PROPERTY(QVariant data
               READ data
               WRITE setData
               RESET resetData
               NOTIFY dataChanged)
    Q_PROPERTY(QbBufferPtr buffer
               READ buffer
               WRITE setBuffer
               RESET resetBuffer
               NOTIFY bufferChanged)
    Q_PROPERTY(ulong bufferSize
               READ bufferSize
               WRITE setBufferSize
               RESET resetBufferSize
               NOTIFY bufferSizeChanged)
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
    Q_PROPERTY(QbFrac timeBase
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
        explicit QbPacket(QObject *parent=NULL);

        QbPacket(const QbCaps &caps,
                 const QbBufferPtr &buffer=QbBufferPtr(),
                 ulong bufferSize=0,
                 qint64 pts=0,
                 const QbFrac &timeBase=QbFrac(),
                 int index=-1,
                 qint64 id=-1);

        QbPacket(const QbPacket &other);
        virtual ~QbPacket();
        QbPacket &operator =(const QbPacket &other);
        operator bool() const;

        Q_INVOKABLE QString toString() const;
        Q_INVOKABLE QbCaps caps() const;
        Q_INVOKABLE QbCaps &caps();
        Q_INVOKABLE QVariant data() const;
        Q_INVOKABLE QVariant &data();
        Q_INVOKABLE QbBufferPtr buffer() const;
        Q_INVOKABLE QbBufferPtr &buffer();
        Q_INVOKABLE ulong bufferSize() const;
        Q_INVOKABLE ulong &bufferSize();
        Q_INVOKABLE qint64 id() const;
        Q_INVOKABLE qint64 &id();
        Q_INVOKABLE qint64 pts() const;
        Q_INVOKABLE qint64 &pts();
        Q_INVOKABLE QbFrac timeBase() const;
        Q_INVOKABLE QbFrac &timeBase();
        Q_INVOKABLE int index() const;
        Q_INVOKABLE int &index();

    private:
        QbPacketPrivate *d;

    signals:
        void capsChanged();
        void dataChanged();
        void bufferChanged();
        void bufferSizeChanged();
        void ptsChanged();
        void timeBaseChanged();
        void indexChanged();
        void idChanged();

    public slots:
        void setCaps(const QbCaps &caps);
        void setData(const QVariant &data);
        void setBuffer(const QbBufferPtr &buffer);
        void setBufferSize(ulong bufferSize);
        void setId(qint64 id);
        void setPts(qint64 pts);
        void setTimeBase(const QbFrac &timeBase);
        void setIndex(int index);
        void resetCaps();
        void resetData();
        void resetBuffer();
        void resetBufferSize();
        void resetId();
        void resetPts();
        void resetTimeBase();
        void resetIndex();

    friend QDebug operator <<(QDebug debug, const QbPacket &packet);
};

QDebug operator <<(QDebug debug, const QbPacket &packet);

Q_DECLARE_METATYPE(QbPacket)

#endif // QBPACKET_H
