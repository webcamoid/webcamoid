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

#include "videosyncelement.h"

VideoSyncElement::VideoSyncElement(): QbElement()
{
    this->m_log = true;

    this->resetOutputThread();
    this->resetMaxQueueSize();
}

QThread *VideoSyncElement::outputThread() const
{
    return this->m_outputThread;
}

int VideoSyncElement::maxQueueSize() const
{
    return this->m_maxQueueSize;
}

void VideoSyncElement::stateChange(QbElement::ElementState from, QbElement::ElementState to)
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
    if (this->m_log)
    {
        QString logFmt("%1 %2 A-V: %3 q=%4KB");

        QString log = logFmt.arg(packet.caps().mimeType()[0])
                            .arg(this->m_elapsedTimer.elapsed() * 1.0e-3
                                 - this->m_timeDrift, 7, 'f', 2)
                            .arg(-diff, 7, 'f', 3)
                            .arg(this->m_queue.size());

        qDebug() << log.toStdString().c_str();
    }
}

void VideoSyncElement::deleteThread(QThread *thread)
{
    thread->quit();
    thread->wait();
    delete thread;
}

void VideoSyncElement::setClock(double clock)
{
    this->m_timeDrift = this->m_elapsedTimer.elapsed() * 1.0e-3 - clock;
}

void VideoSyncElement::setOutputThread(const QThread *outputThread)
{
    this->m_outputThread = const_cast<QThread *>(outputThread);
}

void VideoSyncElement::setMaxQueueSize(int maxQueueSize)
{
    this->m_maxQueueSize = maxQueueSize;
}

void VideoSyncElement::resetOutputThread()
{
    this->setOutputThread(NULL);
}

void VideoSyncElement::resetMaxQueueSize()
{
    this->setMaxQueueSize(3);
}

void VideoSyncElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() != "video/x-raw" ||
        this->state() != ElementStatePlaying)
        return;

    this->m_mutex.lock();

    if (this->m_queue.size() >= this->m_maxQueueSize)
        this->m_queueNotFull.wait(&this->m_mutex);

    this->m_queue.enqueue(packet);
    this->m_queueNotEmpty.wakeAll();
    this->m_mutex.unlock();
}

void VideoSyncElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);

    if (this->state() == QbElement::ElementStateNull)
        this->m_queue.clear();

    if (this->state() == QbElement::ElementStatePlaying)
    {
        this->m_elapsedTimer.start();
        QMetaObject::invokeMethod(&this->m_timer, "start");
    }
    else
        QMetaObject::invokeMethod(&this->m_timer, "stop");
}

void VideoSyncElement::processFrame()
{
    this->m_mutex.lock();

    if (this->m_queue.isEmpty())
        this->m_queueNotEmpty.wait(&this->m_mutex);

    // dequeue the picture
    QbPacket packet = this->m_queue.head();
    double pts = packet.pts() * packet.timeBase().value();

    double diff = pts - this->m_elapsedTimer.elapsed() * 1.0e-3
                      + this->m_timeDrift;

    double delay = packet.duration() * packet.timeBase().value();

    // skip or repeat frame. We take into account the
    // delay to compute the threshold. I still don't know
    // if it is the best guess
    double syncThreshold = qBound(AV_SYNC_THRESHOLD_MIN, delay, AV_SYNC_THRESHOLD_MAX);

    if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD)
    {
        // video is backward the external clock.
        if (diff <= -syncThreshold)
        {
            this->m_queue.removeFirst();
            this->m_queueNotFull.wakeAll();
            this->m_mutex.unlock();

            return;
        }
        // video is ahead the external clock.
        else if (diff > syncThreshold)
        {
            this->m_mutex.unlock();

            return;
        }
    }
    else
        this->m_timeDrift = this->m_elapsedTimer.elapsed() * 1.0e-3 - pts;

    this->printLog(packet, diff);
    emit this->oStream(packet);
    this->m_queue.removeFirst();
    this->m_queueNotFull.wakeAll();
    this->m_mutex.unlock();
}

void VideoSyncElement::init()
{
    if (this->m_outputThread)
        this->m_timer.moveToThread(const_cast<QThread *>(this->m_outputThread));
    else {
        this->m_outputThreadPtr = ThreadPtr(new QThread(), this->deleteThread);
        this->m_outputThreadPtr->start();
        this->m_timer.moveToThread(this->m_outputThreadPtr.data());
    }

    QObject::connect(&this->m_timer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(processFrame()),
                     Qt::DirectConnection);

    this->m_timeDrift = 0;
}

void VideoSyncElement::uninit()
{
    QObject::disconnect(&this->m_timer,
                        SIGNAL(timeout()),
                        this,
                        SLOT(processFrame()));

    this->m_timer.moveToThread(this->thread());
    this->m_outputThreadPtr.clear();
}
