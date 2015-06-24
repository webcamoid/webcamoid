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

#include "videosyncelement.h"
#include "sleep.h"

VideoSyncElement::VideoSyncElement(): QbElement()
{
    this->m_run = false;
    this->m_outputThread = NULL;
    this->m_lastPts = 0;

    this->m_maxQueueSize = 3;
    this->m_showLog = false;
}

VideoSyncElement::~VideoSyncElement()
{
    this->uninit();
}

int VideoSyncElement::maxQueueSize() const
{
    return this->m_maxQueueSize;
}

bool VideoSyncElement::showLog() const
{
    return this->m_showLog;
}

void VideoSyncElement::stateChange(QbElement::ElementState from,
                                   QbElement::ElementState to)
{
    if (from == QbElement::ElementStateNull
        && to == QbElement::ElementStatePaused)
        this->init();
    else if (from == QbElement::ElementStatePaused
             && to == QbElement::ElementStateNull)
        this->uninit();
}

void VideoSyncElement::printLog(const QbPacket &packet, double diff)
{
    if (this->m_showLog) {
        QString logFmt("%1 %2 A-V: %3 q=%4");

        QString log = logFmt.arg(packet.caps().mimeType()[0])
                            .arg(this->m_elapsedTimer.elapsed() * 1.0e-3
                                 - this->m_timeDrift, 7, 'f', 2)
                            .arg(-diff, 7, 'f', 3)
                            .arg(this->m_queue.size());

        qDebug() << log.toStdString().c_str();
    }
}

void VideoSyncElement::setClock(double clock)
{
    this->m_timeDrift = this->m_elapsedTimer.elapsed() * 1.0e-3 - clock;
}

void VideoSyncElement::setMaxQueueSize(int maxQueueSize)
{
    if (this->m_maxQueueSize == maxQueueSize)
        return;

    this->m_maxQueueSize = maxQueueSize;
    emit this->maxQueueSizeChanged(maxQueueSize);
}

void VideoSyncElement::setShowLog(bool showLog)
{
    if (this->m_showLog == showLog)
        return;

    this->m_showLog = showLog;
    emit this->showLogChanged(showLog);
}

void VideoSyncElement::resetMaxQueueSize()
{
    this->setMaxQueueSize(3);
}

void VideoSyncElement::resetShowLog()
{
    this->setShowLog(false);
}

QbPacket VideoSyncElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() != "video/x-raw"
        || !this->m_run)
        return QbPacket();

    this->m_mutex.lock();

    if (this->m_queue.size() >= this->m_maxQueueSize)
        this->m_queueNotFull.wait(&this->m_mutex);

    this->m_queue.enqueue(packet);
    this->m_queueNotEmpty.wakeAll();

    this->m_mutex.unlock();

    return packet;
}

void VideoSyncElement::processFrame()
{
    while (this->m_run) {
        this->m_mutex.lock();

        if (this->m_queue.isEmpty())
            this->m_queueNotEmpty.wait(&this->m_mutex);

        if (!this->m_queue.isEmpty()) {
            // dequeue the picture
            QbPacket packet = this->m_queue.head();
            double pts = packet.pts() * packet.timeBase().value();

            double diff = pts - this->m_elapsedTimer.elapsed() * 1.0e-3
                              + this->m_timeDrift;

            double delay = pts - this->m_lastPts;

            // skip or repeat frame. We take into account the
            // delay to compute the threshold. I still don't know
            // if it is the best guess
            double syncThreshold = qBound(AV_SYNC_THRESHOLD_MIN,
                                          delay,
                                          AV_SYNC_THRESHOLD_MAX);

            if (!std::isnan(diff)
                && qAbs(diff) < AV_NOSYNC_THRESHOLD
                && delay < AV_SYNC_FRAMEDUP_THRESHOLD) {
                // video is backward the external clock.
                if (diff <= -syncThreshold) {
                    this->m_queue.removeFirst();
                    this->m_queueNotFull.wakeAll();
                    this->m_lastPts = pts;
                    this->m_mutex.unlock();

                    continue;
                } else if (diff > syncThreshold) {
                    // video is ahead the external clock.
                    Sleep::usleep(1e6 * (diff - syncThreshold));
                    this->m_mutex.unlock();

                    continue;
                }
            } else
                this->m_timeDrift = this->m_elapsedTimer.elapsed() * 1.0e-3
                                    - pts;

            this->printLog(packet, diff);
            emit this->oStream(packet);
            this->m_queue.removeFirst();
            this->m_queueNotFull.wakeAll();
            this->m_lastPts = pts;
        }

        this->m_mutex.unlock();
    }
}

void VideoSyncElement::init()
{
    this->m_timeDrift = 0;
    this->m_lastPts = 0;
    this->m_elapsedTimer.start();
    this->m_queue.clear();
    this->m_outputThread = new Thread();

    QObject::connect(this->m_outputThread,
                     SIGNAL(runTh()),
                     this,
                     SLOT(processFrame()),
                     Qt::DirectConnection);

    this->m_run = true;
    this->m_outputThread->start();
}

void VideoSyncElement::uninit()
{
    if (!this->m_run)
        return;

    this->m_run = false;
    this->m_mutex.lock();
    this->m_queue.clear();
    this->m_queueNotFull.wakeAll();
    this->m_queueNotEmpty.wakeAll();
    this->m_mutex.unlock();

    if (this->m_outputThread) {
        this->m_outputThread->wait();
        delete this->m_outputThread;
        this->m_outputThread = NULL;
    }
}
