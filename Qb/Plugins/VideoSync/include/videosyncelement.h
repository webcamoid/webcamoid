/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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

#include "thread.h"

// no AV sync correction is done if below the minimum AV sync threshold
#define AV_SYNC_THRESHOLD_MIN 0.01

// AV sync correction is done if above the maximum AV sync threshold
#define AV_SYNC_THRESHOLD_MAX 0.1

// If a frame duration is longer than this, it will not be duplicated to compensate AV sync
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1

// no AV correction is done if too big error
#define AV_NOSYNC_THRESHOLD 10.0

class VideoSyncElement: public QbElement
{
    Q_OBJECT

    Q_PROPERTY(int maxQueueSize READ maxQueueSize
                                WRITE setMaxQueueSize
                                RESET resetMaxQueueSize)

    public:
        explicit VideoSyncElement();
        ~VideoSyncElement();

        Q_INVOKABLE int maxQueueSize() const;

    protected:
        void stateChange(QbElement::ElementState from, QbElement::ElementState to);

    private:
        int m_maxQueueSize;
        bool m_log;

        Thread *m_outputThread;
        bool m_run;

        QMutex m_mutex;
        QWaitCondition m_queueNotEmpty;
        QWaitCondition m_queueNotFull;
        QQueue<QbPacket> m_queue;

        QTimer m_timer;
        QElapsedTimer m_elapsedTimer;
        double m_timeDrift;
        double m_lastPts;

        void printLog(const QbPacket &packet, double diff);

    public slots:
        void setClock(double clock);
        void setMaxQueueSize(int maxQueueSize);
        void resetMaxQueueSize();
        void processFrame();
        void init();
        void uninit();

        void iStream(const QbPacket &packet);
};

#endif // VIDEOSYNCELEMENT_H
