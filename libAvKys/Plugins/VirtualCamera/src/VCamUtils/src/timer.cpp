/* Webcamoid, webcam capture application.
 * Copyright (C) 2018  Gonzalo Exequiel Pedone
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

#include <chrono>
#include <thread>

#include "timer.h"

namespace AkVCam
{
    class TimerPrivate
    {
        public:
            Timer *self;
            std::thread m_thread;
            int m_interval;
            bool m_running;

            TimerPrivate(Timer *self);
            void timerLoop();
    };
}

AkVCam::Timer::Timer()
{
    this->d = new TimerPrivate(this);
}

AkVCam::Timer::~Timer()
{
    this->stop();
    delete this->d;
}

int AkVCam::Timer::interval() const
{
    return this->d->m_interval;
}

int &AkVCam::Timer::interval()
{
    return this->d->m_interval;
}

void AkVCam::Timer::setInterval(int msec)
{
    this->d->m_interval = msec;
}

void AkVCam::Timer::start()
{
    this->stop();
    this->d->m_running = true;
    this->d->m_thread = std::thread(&TimerPrivate::timerLoop, this->d);
}

void AkVCam::Timer::stop()
{
    if (!this->d->m_running)
        return;

    this->d->m_running = false;
    this->d->m_thread.join();
}

AkVCam::TimerPrivate::TimerPrivate(AkVCam::Timer *self):
    self(self),
    m_interval(0),
    m_running(false)
{

}

void AkVCam::TimerPrivate::timerLoop()
{
    while (this->m_running) {
        if (this->m_interval)
            std::this_thread::sleep_for(std::chrono::milliseconds(this->m_interval));

        AKVCAM_EMIT_NOARGS(this->self, Timeout);
    }
}
