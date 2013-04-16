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

#ifndef WORKER_H
#define WORKER_H

#include <qb.h>

class Worker: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QbElement::ElementState state READ state WRITE setState RESET resetState)
    Q_PROPERTY(bool waitUnlock READ waitUnlock WRITE setWaitUnlock RESET resetWaitUnlock)
    Q_PROPERTY(bool noFps READ noFps WRITE setNoFps RESET resetNoFps)

    public:
        explicit Worker(QObject *parent=NULL);
        ~Worker();

        Q_INVOKABLE QbElement::ElementState state() const;
        Q_INVOKABLE bool waitUnlock() const;
        Q_INVOKABLE bool noFps() const;

    private:
        QbElement::ElementState m_state;
        QbPacket m_packet;
        QQueue<QbPacket> m_queue;
        QTimer m_timer;
        double m_fps;
        double m_t;
        double m_dt;
        quint64 m_ti;
        quint64 m_dti;
        bool m_waitUnlock;
        bool m_unlocked;
        bool m_noFps;
        QMutex m_mutex;

    signals:
        void oStream(const QbPacket &packet);
        void oDiscardFrames(int nFrames);

    public slots:
        void setWaitUnlock(bool waitUnlock);
        void setNoFps(bool noFps);
        void resetWaitUnlock();
        void resetNoFps();
        void doWork();
        void appendPacketInfo(const QbPacket &packet);
        void dropBuffers();
        void unlock();
        void setState(QbElement::ElementState state);
        void resetState();
        void iDiscardFrames(int nFrames);

    private slots:
        void resetTime();
};

#endif // WORKER_H
