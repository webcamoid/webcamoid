/* Webcamod, webcam capture plasmoid.
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

class QbPacket: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QbCaps caps READ caps WRITE setCaps RESET resetCaps)
    Q_PROPERTY(QSharedPointer<uchar> buffer READ buffer WRITE setBuffer RESET resetBuffer)
    Q_PROPERTY(ulong bufferSize READ bufferSize WRITE setBufferSize RESET resetBufferSize)
    Q_PROPERTY(int64_t pts READ pts WRITE setPts RESET resetPts)
    Q_PROPERTY(int duration READ duration WRITE setDuration RESET resetDuration)
    Q_PROPERTY(QbFrac timeBase READ timeBase WRITE setTimeBase RESET resetTimeBase)
    Q_PROPERTY(int index READ index WRITE setIndex RESET resetIndex)

    public:
        explicit QbPacket(QObject *parent=NULL);

        QbPacket(QbCaps caps,
                 const QSharedPointer<uchar> &buffer=QSharedPointer<uchar>(),
                 ulong bufferSize=0,
                 int64_t pts=0,
                 int duration=0,
                 QbFrac timeBase=QbFrac(),
                 int index=-1);

        QbPacket(const QbPacket &other);
        virtual ~QbPacket();
        QbPacket &operator =(const QbPacket &other);

        Q_INVOKABLE QString toString() const;
        Q_INVOKABLE QbCaps caps() const;
        Q_INVOKABLE QSharedPointer<uchar> buffer() const;
        Q_INVOKABLE ulong bufferSize() const;
        Q_INVOKABLE int64_t pts() const;
        Q_INVOKABLE int duration() const;
        Q_INVOKABLE QbFrac timeBase() const;
        Q_INVOKABLE int index() const;

    private:
        QbCaps m_caps;
        QSharedPointer<uchar> m_buffer;
        ulong m_bufferSize;
        int64_t m_pts;
        int m_duration;
        QbFrac m_timeBase;
        int m_index;

        friend QDebug operator <<(QDebug debug, const QbPacket &frac);

    public slots:
        void setCaps(QbCaps caps);
        void setBuffer(const QSharedPointer<uchar> &buffer);
        void setBufferSize(ulong bufferSize);
        void setPts(int64_t pts);
        void setDuration(int duration);
        void setTimeBase(QbFrac timeBase);
        void setIndex(int index);
        void resetCaps();
        void resetBuffer();
        void resetBufferSize();
        void resetPts();
        void resetDuration();
        void resetTimeBase();
        void resetIndex();
};

QDebug operator <<(QDebug debug, const QbPacket &frac);

Q_DECLARE_METATYPE(QbPacket)

#endif // QBPACKET_H
