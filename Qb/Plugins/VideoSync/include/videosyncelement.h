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

#ifndef VIDEOSYNCELEMENT_H
#define VIDEOSYNCELEMENT_H

#include <qb.h>

// no AV sync correction is done if below the minimum AV sync threshold
#define AV_SYNC_THRESHOLD_MIN 0.01

// AV sync correction is done if above the maximum AV sync threshold
#define AV_SYNC_THRESHOLD_MAX 0.1

// no AV correction is done if too big error
#define AV_NOSYNC_THRESHOLD 10.0

typedef QSharedPointer<QThread> ThreadPtr;

class VideoSyncElement: public QbElement
{
    Q_OBJECT

    Q_PROPERTY(QThread *outputThread READ outputThread
                                     WRITE setOutputThread
                                     RESET resetOutputThread)

    Q_PROPERTY(int maxQueueSize READ maxQueueSize
                                WRITE setMaxQueueSize
                                RESET resetMaxQueueSize)

    public:
        explicit VideoSyncElement();

        Q_INVOKABLE QThread *outputThread() const;
        Q_INVOKABLE int maxQueueSize() const;

    protected:
        void stateChange(QbElement::ElementState from, QbElement::ElementState to);

    private:
        int m_maxQueueSize;
        bool m_log;

        ThreadPtr m_outputThreadPtr;
        QThread *m_outputThread;
        QMutex m_mutex;
        QWaitCondition m_queueNotEmpty;
        QWaitCondition m_queueNotFull;
        QQueue<QbPacket> m_queue;

        QTimer m_timer;
        QElapsedTimer m_elapsedTimer;
        double m_timeDrift;

        void printLog(const QbPacket &packet, double diff);
        static void deleteThread(QThread *thread);

    public slots:
        void setClock(double clock);
        void setOutputThread(const QThread *outputThread);
        void setMaxQueueSize(int maxQueueSize);
        void resetOutputThread();
        void resetMaxQueueSize();
        void processFrame();
        void init();
        void uninit();

        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);
};

#endif // VIDEOSYNCELEMENT_H
