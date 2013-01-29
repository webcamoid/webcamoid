/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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

class QbPacket: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QbCaps caps READ caps WRITE setCaps RESET resetCaps)
    Q_PROPERTY(const void *data READ data WRITE setData RESET resetData)
    Q_PROPERTY(ulong dataSize READ dataSize WRITE setDataSize RESET resetDataSize)
    Q_PROPERTY(int64_t dts READ dts WRITE setDts RESET resetDts)
    Q_PROPERTY(int64_t pts READ pts WRITE setPts RESET resetPts)
    Q_PROPERTY(int duration READ duration WRITE setDuration RESET resetDuration)
    Q_PROPERTY(int index READ index WRITE setIndex RESET resetIndex)

    public:
        explicit QbPacket(QObject *parent=NULL);

        QbPacket(QbCaps caps, const void *data=NULL,
                 ulong dataSize=0,
                 int64_t dts=0,
                 int64_t pts=0,
                 int duration=0,
                 int index=-1);

        QbPacket(const QbPacket &other);
        QbPacket &operator =(const QbPacket &other);

        Q_INVOKABLE QbCaps caps() const;
        Q_INVOKABLE const void *data() const;
        Q_INVOKABLE ulong dataSize() const;
        Q_INVOKABLE int64_t dts() const;
        Q_INVOKABLE int64_t pts() const;
        Q_INVOKABLE int duration() const;
        Q_INVOKABLE int index() const;

    private:
        QbCaps m_caps;
        const void *m_data;
        ulong m_dataSize;
        int64_t m_dts;
        int64_t m_pts;
        int m_duration;
        int m_index;

    public slots:
        void setCaps(QbCaps caps);
        void setData(const void *data);
        void setDataSize(ulong dataSize);
        void setDts(int64_t dts);
        void setPts(int64_t pts);
        void setDuration(int duration);
        void setIndex(int index);
        void resetCaps();
        void resetData();
        void resetDataSize();
        void resetDts();
        void resetPts();
        void resetDuration();
        void resetIndex();
};

#endif // QBPACKET_H
