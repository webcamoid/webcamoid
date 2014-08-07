/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
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
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#ifndef QBPACKET_H
#define QBPACKET_H

#include "qbcaps.h"
#include "qbfrac.h"

typedef QSharedPointer<char> QbBufferPtr;

class QbPacket: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QbCaps caps READ caps WRITE setCaps RESET resetCaps)
    Q_PROPERTY(QVariant data READ data WRITE setData RESET resetData)
    Q_PROPERTY(QbBufferPtr buffer READ buffer WRITE setBuffer RESET resetBuffer)
    Q_PROPERTY(ulong bufferSize READ bufferSize WRITE setBufferSize RESET resetBufferSize)
    Q_PROPERTY(qint64 id READ id WRITE setId RESET resetId)
    Q_PROPERTY(qint64 pts READ pts WRITE setPts RESET resetPts)
    Q_PROPERTY(QbFrac timeBase READ timeBase WRITE setTimeBase RESET resetTimeBase)
    Q_PROPERTY(int index READ index WRITE setIndex RESET resetIndex)

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
        Q_INVOKABLE QVariant data() const;
        Q_INVOKABLE QbBufferPtr buffer() const;
        Q_INVOKABLE ulong bufferSize() const;
        Q_INVOKABLE qint64 id() const;
        Q_INVOKABLE qint64 pts() const;
        Q_INVOKABLE QbFrac timeBase() const;
        Q_INVOKABLE int index() const;

    private:
        QbCaps m_caps;
        QVariant m_data;
        QbBufferPtr m_buffer;
        ulong m_bufferSize;
        qint64 m_pts;
        QbFrac m_timeBase;
        int m_index;
        qint64 m_id;

        friend QDebug operator <<(QDebug debug, const QbPacket &frac);

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
};

QDebug operator <<(QDebug debug, const QbPacket &frac);

Q_DECLARE_METATYPE(QbPacket)

#endif // QBPACKET_H
