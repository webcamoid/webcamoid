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
            int m_interval;
            TimerTimeoutCallback m_timerTimeoutCallback;
            void *m_userData;
            std::thread m_thread;
            bool m_running;

            void timerLoop();
    };
}

AkVCam::Timer::Timer()
{
    this->d = new TimerPrivate;
    this->d->m_interval = 0;
    this->d->m_timerTimeoutCallback = nullptr;
    this->d->m_userData = nullptr;
    this->d->m_running = false;
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

void AkVCam::Timer::setTimeoutCallback(TimerTimeoutCallback callback,
                                       void *userData)
{
    this->d->m_timerTimeoutCallback = callback;
    this->d->m_userData = userData;
}

void AkVCam::TimerPrivate::timerLoop()
{
    while (this->m_running) {
        if (this->m_interval)
            std::this_thread::sleep_for(std::chrono::milliseconds(this->m_interval));

        if (this->m_timerTimeoutCallback)
            this->m_timerTimeoutCallback(this->m_userData);
    }
}
