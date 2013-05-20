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

#include "sleep.h"
#include "syncelement.h"

SyncElement::SyncElement(): QbElement()
{
    this->m_ready = false;

    QObject::connect(&this->m_timer, SIGNAL(timeout()), this, SLOT(sendFrame()));

    this->resetNoQueue();
}

SyncElement::~SyncElement()
{
}

bool SyncElement::noQueue() const
{
    return this->m_noQueue;
}

void SyncElement::setNoQueue(bool noQueue)
{
    this->m_noQueue = noQueue;
}

void SyncElement::resetNoQueue()
{
    this->setNoQueue(true);
}

void SyncElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        this->state() != ElementStatePlaying)
        return;

    if (!this->m_ready)
    {
        this->m_ready = true;
        emit this->ready(packet.index());
    }

    if (packet.caps().property("sync").toBool())
    {
        if (this->noQueue())
            this->sendFrame(packet);
        else
        {
            this->m_mutex.lock();
            this->m_queue.enqueue(packet);
            this->m_mutex.unlock();

            if (!this->m_timer.isActive())
                this->m_timer.start();
        }
    }
    else
    {
        emit this->oStream(packet);

        if (this->m_timer.isActive())
            this->m_timer.stop();
    }
}

void SyncElement::setState(ElementState state)
{
    QbElement::ElementState preState = this->state();
    QbElement::setState(state);

    if (this->state() == QbElement::ElementStateReady ||
        this->state() == QbElement::ElementStateNull)
    {
        this->m_ready = false;
        this->m_timer.stop();
        this->m_fst = true;
        this->m_timer.setInterval(0);
        this->m_queue.clear();
    }

    if (this->state() == QbElement::ElementStatePaused)
        this->m_timer.stop();

    if (preState != QbElement::ElementStatePlaying &&
        this->state() == QbElement::ElementStatePlaying)
    {
        this->m_timer.start();
    }
}

// This probably will be removed
void SyncElement::sendFrame()
{
    if (this->m_fst)
    {
        this->m_mutex.lock();

        if (!this->m_queue.isEmpty())
        {
            this->m_lastPacket = this->m_queue.dequeue();
            this->m_mutex.unlock();

            this->m_iPts = this->m_lastPacket.pts() * this->m_lastPacket.timeBase().value();
            this->m_duration = this->m_lastPacket.duration() * this->m_lastPacket.timeBase().value();
            this->m_timer.setInterval(1e3 * this->m_duration);
            this->m_elapsedTimer.start();
            emit this->oStream(this->m_lastPacket);
            this->m_fst = false;
        }
        else
            this->m_mutex.unlock();
    }
    else
    {
        this->m_mutex.lock();

        if (this->m_queue.isEmpty())
        {
            this->m_mutex.unlock();
            this->m_timer.setInterval(this->m_duration);
        }
        else
        {
            double pts = this->m_queue[0].pts() * this->m_queue[0].timeBase().value();
            this->m_mutex.unlock();

            int clockN = this->m_elapsedTimer.nsecsElapsed() / 1.0e9 / this->m_duration;
            int packetN = (pts - this->m_iPts) / this->m_duration;

            if (packetN < clockN)
            {
                int n = clockN - packetN;
                int queueSize = this->m_queue.size();

                this->m_mutex.lock();

                for (int i = 0; i < n && !this->m_queue.isEmpty(); i++)
                    this->m_lastPacket = this->m_queue.dequeue();

                this->m_mutex.unlock();

                if (n > queueSize)
                {
                    double loadSpeed = 1.0e6 * (packetN + queueSize) / this->m_elapsedTimer.nsecsElapsed();
                    int waitLoad = (n - queueSize) / loadSpeed;

                    if (waitLoad < 0)
                        waitLoad = 0;

                    this->m_timer.setInterval(waitLoad);
                }
                else
                    this->m_timer.setInterval(0);
            }
            else if (packetN > clockN)
            {
                double duration = 1.0e3 * (pts - this->m_iPts) - this->m_elapsedTimer.nsecsElapsed() / 1.0e6;

                if (duration < 0)
                    duration = 0;

                this->m_timer.setInterval(duration);
            }
            else
            {
                this->m_mutex.lock();
                this->m_lastPacket = this->m_queue.dequeue();
                this->m_mutex.unlock();

                double duration = pts - this->m_iPts + this->m_duration - this->m_elapsedTimer.nsecsElapsed() / 1.0e9;

                if (duration < 0)
                    duration = 0;

                this->m_timer.setInterval(1e3 * duration);
                emit this->oStream(this->m_lastPacket);
            }
        }
    }
}

void SyncElement::sendFrame(const QbPacket &packet)
{
    if (this->m_fst)
    {
        this->m_iPts = packet.pts() * packet.timeBase().value();
        this->m_duration = packet.duration() * packet.timeBase().value();
        this->m_elapsedTimer.start();
        emit this->oStream(packet);
        Sleep::usleep(1e6 * this->m_duration);
        this->m_fst = false;
    }
    else
    {
        double pts = packet.pts() * packet.timeBase().value();

        int clockN = this->m_elapsedTimer.nsecsElapsed() / 1.0e9 / this->m_duration;
        int packetN = (pts - this->m_iPts) / this->m_duration;

        if (packetN < clockN)
        {
            int diff = clockN - packetN;

            if (diff > 5)
            {
                this->m_iPts = packet.pts() * packet.timeBase().value();
                this->m_duration = packet.duration() * packet.timeBase().value();
                this->m_elapsedTimer.restart();
                emit this->oStream(packet);
                Sleep::usleep(1e6 * this->m_duration);
            }
        }
        else if (packetN > clockN)
        {
            double duration = 1.0e6 * (pts - this->m_iPts) - this->m_elapsedTimer.nsecsElapsed() / 1.0e3;

            if (duration < 0)
                duration = 0;

            Sleep::usleep(duration);
            emit this->oStream(packet);
        }
        else
        {
            double duration = pts - this->m_iPts + this->m_duration - this->m_elapsedTimer.nsecsElapsed() / 1.0e9;

            if (duration < 0)
                duration = 0;

            emit this->oStream(packet);
            Sleep::usleep(1e3 * duration);
        }
    }
}
