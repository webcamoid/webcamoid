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

#include "syncelement.h"

SyncElement::SyncElement(): QbElement()
{
    this->m_fst = true;
    this->m_ready = false;

    this->m_log = true;

    this->resetVideoTh();
}

SyncElement::~SyncElement()
{
    this->setState(QbElement::ElementStateNull);
}

QString SyncElement::videoTh() const
{
    return this->m_videoTh;
}

void SyncElement::printLog(const QbPacket &packet, double diff)
{
    if (this->m_log)
    {
        QString logFmt("%1 %2 A-V: %3 aq=%4KB vq=%5KB");

        QString log = logFmt.arg(packet.caps().mimeType()[0])
                            .arg(this->m_extrnClock.clock(), 7, 'f', 2)
                            .arg(-diff, 7, 'f', 3)
                            .arg(this->m_avqueue.size("audio/x-raw") / 1024, 5)
                            .arg(this->m_avqueue.size("video/x-raw") / 1024, 5);

        qDebug() << log.toStdString().c_str();
    }
}

TimerPtr SyncElement::runThread(QThread *thread, const char *method)
{
    TimerPtr timer = TimerPtr(new QTimer());

    QObject::connect(timer.data(), SIGNAL(timeout()), this, method, Qt::DirectConnection);
    timer->moveToThread(thread);

    return timer;
}

void SyncElement::setAudioPts(double pts)
{
    double time = QDateTime::currentMSecsSinceEpoch() / 1.0e3;
    this->m_extrnClock.setClockAt(pts, time);
}

void SyncElement::setVideoTh(const QString &videoTh)
{
    QbElement::ElementState state = this->state();
    this->setState(QbElement::ElementStateNull);

    this->m_videoTh = videoTh;
    this->m_videoThread = Qb::requestThread(this->m_videoTh);

    if (this->m_videoTh == "MAIN")
        this->m_videoTimer = this->runThread(QCoreApplication::instance()->thread(),
                                             SLOT(processVideoFrame()));
    else if (!this->m_videoThread)
        this->m_videoTimer = this->runThread(this->thread(),
                                             SLOT(processVideoFrame()));
    else
        this->m_videoTimer = this->runThread(this->m_videoThread.data(),
                                             SLOT(processVideoFrame()));

    this->setState(state);
}

void SyncElement::resetVideoTh()
{
    this->setVideoTh("");
}

void SyncElement::iStream(const QbPacket &packet)
{
    if (!packet ||
        this->state() != ElementStatePlaying)
        return;

    if (!this->m_ready)
    {
        this->m_ready = true;
        emit this->ready(packet.index());
    }

    if (this->m_fst)
    {
        this->m_videoClock = Clock();
        this->m_extrnClock = Clock();

        this->m_fst = false;
    }

    this->m_avqueue.enqueue(packet);
}

void SyncElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);

    if (this->state() == QbElement::ElementStateNull)
    {
        this->m_ready = false;
        this->m_fst = true;
        this->m_avqueue.clear();
    }

    if (this->state() == QbElement::ElementStatePlaying)
    {
        if (this->m_videoTimer)
            QMetaObject::invokeMethod(this->m_videoTimer.data(), "start");
    }
    else
    {
        if (this->m_videoTimer)
            QMetaObject::invokeMethod(this->m_videoTimer.data(), "stop");
    }
}

void SyncElement::releaseAudioFrame(int frameSize)
{
    Q_UNUSED(frameSize)

    QbPacket packet = this->m_avqueue.dequeue("audio/x-raw");

    if (packet)
        emit this->oStream(packet);
}

void SyncElement::processVideoFrame()
{
    if (this->m_avqueue.size("video/x-raw") < 1)
        return;

    // dequeue the picture
    QbPacket packet = this->m_avqueue.read("video/x-raw");
    double pts = packet.pts() * packet.timeBase().value();
    double diff = pts - this->m_extrnClock.clock();
    double delay = packet.duration() * packet.timeBase().value();

    // skip or repeat frame. We take into account the
    // delay to compute the threshold. I still don't know
    // if it is the best guess
    double syncThreshold = qBound(AV_SYNC_THRESHOLD_MIN, delay, AV_SYNC_THRESHOLD_MAX);
    this->printLog(packet, diff);

    if (!isnan(diff) && fabs(diff) < 10.0)
    {
        // video is backward the external clock.
        if (diff <= -syncThreshold)
        {
            this->m_avqueue.dequeue("video/x-raw");

            return;
        }
        // video is ahead the external clock.
        else if (diff > syncThreshold)
            return;
    }

    this->printLog(packet, diff);
    emit this->oStream(packet);
    this->m_avqueue.dequeue("video/x-raw");
}

void SyncElement::updateVideoPts(double pts)
{
    this->m_videoClock.setClock(pts);
    this->m_extrnClock.syncTo(this->m_videoClock);
}
