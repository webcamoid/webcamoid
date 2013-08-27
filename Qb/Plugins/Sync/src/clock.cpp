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

#include "clock.h"

Clock::Clock(QObject *parent): QObject(parent)
{
    this->init();
}

Clock::Clock(bool slave): QObject(NULL)
{
    this->init(slave);
}

Clock::Clock(const Clock &other):
    QObject(other.parent()),
    m_lastClock(other.m_lastClock),
    m_drift(other.m_drift),
    m_slave(other.m_slave),
    m_clock0(other.m_clock0)
{
}

Clock &Clock::operator =(const Clock &other)
{
    if (this != &other)
    {
        this->m_lastClock = other.m_lastClock;
        this->m_drift = other.m_drift;
        this->m_slave = other.m_slave;
        this->m_clock0 = other.m_clock0;
    }

    return *this;
}

double Clock::clock(double pts)
{
    double clock = QDateTime::currentMSecsSinceEpoch() / 1.0e3;

    if (isnan(this->m_clock0))
        this->m_clock0 = this->m_slave? pts: clock;

    if (!this->m_slave)
        pts = clock;

    this->m_lastClock = pts - this->m_clock0 + this->m_drift;

    return this->m_lastClock;
}

void Clock::init(bool slave)
{
    this->m_lastClock = 0.0;
    this->m_drift = 0.0;
    this->m_slave = slave;
    this->m_clock0 = NAN;
}

void Clock::syncTo(double pts)
{
    this->m_drift += pts - this->m_lastClock;
}
