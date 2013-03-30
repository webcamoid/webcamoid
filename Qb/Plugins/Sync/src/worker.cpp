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

#include "worker.h"
#include "sleep.h"

Worker::Worker(QObject *parent): QObject(parent)
{
    this->m_fps = 0;
    this->m_state = QbElement::ElementStateNull;
    this->m_unlocked = true;

    QObject::connect(&this->m_timer, SIGNAL(timeout()), this, SLOT(doWork()));

    this->resetWaitUnlock();
    this->resetTime();
}

Worker::~Worker()
{
    this->setState(QbElement::ElementStateNull);
}

QbElement::ElementState Worker::state() const
{
    return this->m_state;
}

bool Worker::waitUnlock() const
{
    return this->m_waitUnlock;
}

void Worker::setWaitUnlock(bool waitUnlock)
{
    this->m_waitUnlock = waitUnlock;
}

void Worker::resetWaitUnlock()
{
    this->setWaitUnlock(false);
}

void Worker::doWork()
{
    if (!this->m_unlocked)
        return;

    if (this->m_queue.isEmpty())
    {
        if (!this->m_packet.caps().isValid() || this->m_data.isEmpty())
            return;
    }
    else
    {
        QSharedPointer<PacketInfo> packetInfo = this->m_queue.dequeue();
        this->m_packet = packetInfo->packet();
        this->m_data = packetInfo->data();
    }

    QString fpsString = this->m_packet.caps().property("fps").toString();
    double fps = QbFrac(fpsString).value();

    if (this->m_fps != fps)
    {
        this->m_fps = fps;
        this->resetTime();
    }

    this->m_packet.setData(this->m_data.constData());
    emit this->oStream(this->m_packet);

    quint64 diff = this->m_t - this->m_ti;
    quint64 wait = this->m_dti + diff;

    Sleep::usleep(wait);
    this->m_t += this->m_dt;
    this->m_ti += wait;
}

void Worker::appendPacketInfo(const PacketInfo &packetInfo)
{
    this->m_queue.enqueue(QSharedPointer<PacketInfo>(new PacketInfo(packetInfo)));
}

void Worker::dropBuffers()
{
    this->m_queue.clear();
    this->m_packet = QbPacket();
    this->m_data = QByteArray();
}

void Worker::unlock()
{
    this->m_unlocked = true;
}

void Worker::setState(QbElement::ElementState state)
{
    QbElement::ElementState preState = this->m_state;
    this->m_state = state;

    if (state == QbElement::ElementStateReady ||
        state == QbElement::ElementStateNull)
    {
        this->m_timer.stop();
        this->m_unlocked = !this->m_waitUnlock;
        this->resetTime();
        this->dropBuffers();
    }

    if (state == QbElement::ElementStatePaused)
        this->m_timer.stop();

    if (preState != QbElement::ElementStatePlaying &&
        state == QbElement::ElementStatePlaying)
        this->m_timer.start();
}

void Worker::resetState()
{
    this->setState(QbElement::ElementStateNull);
}

void Worker::resetTime()
{
    this->m_t = 0;
    this->m_dt = 1e6 / this->m_fps;
    this->m_ti = 0;
    this->m_dti = this->m_dt;
}
