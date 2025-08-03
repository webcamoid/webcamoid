/* Webcamoid, camera capture application.
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

#ifndef AKPACKETBASE_H
#define AKPACKETBASE_H

#include <QObject>

#include "akcommons.h"

class AkPacketBase;
class AkPacketBasePrivate;
class AkFrac;

template<typename T>
inline T AkNoPts()
{
    return T(0x1) << (sizeof(T) - 1);
}

class AKCOMMONS_EXPORT AkPacketBase: public QObject
{
    Q_OBJECT
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
    Q_PROPERTY(qint64 dts
               READ dts
               WRITE setDts
               RESET resetDts
               NOTIFY dtsChanged)
    Q_PROPERTY(quint64 duration
               READ duration
               WRITE setDuration
               RESET resetDuration
               NOTIFY durationChanged)
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
    Q_PROPERTY(QByteArray extraData
               READ extraData
               WRITE setExtraData
               RESET resetExtraData
               NOTIFY extraDataChanged)

    public:
        AkPacketBase(QObject *parent=nullptr);
        AkPacketBase(const AkPacketBase &other);
        virtual ~AkPacketBase();

        Q_INVOKABLE qint64 id() const;
        Q_INVOKABLE qint64 pts() const;
        Q_INVOKABLE qint64 dts() const;
        Q_INVOKABLE quint64 duration() const;
        Q_INVOKABLE AkFrac timeBase() const;
        Q_INVOKABLE int index() const;
        Q_INVOKABLE QByteArray extraData() const;
        Q_INVOKABLE void copyMetadata(const AkPacketBase &other);

    private:
        AkPacketBasePrivate *d;

    Q_SIGNALS:
        void idChanged(qint64 id);
        void ptsChanged(qint64 pts);
        void dtsChanged(qint64 dts);
        void durationChanged(quint64 duration);
        void timeBaseChanged(const AkFrac &timeBase);
        void indexChanged(int index);
        void extraDataChanged(const QByteArray &extraData);

    public Q_SLOTS:
        void setId(qint64 id);
        void setPts(qint64 pts);
        void setDts(qint64 dts);
        void setDuration(quint64 duration);
        void setTimeBase(const AkFrac &timeBase);
        void setIndex(int index);
        void setExtraData(const QByteArray &extraData);
        void resetId();
        void resetPts();
        void resetDts();
        void resetDuration();
        void resetTimeBase();
        void resetIndex();
        void resetExtraData();
};

#endif // AKPACKETBASE_H
