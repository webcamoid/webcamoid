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
    this->setClock(NAN);
}

Clock::Clock(const Clock &other):
    QObject(other.parent()),
    m_pts(other.m_pts),
    m_ptsDrift(other.m_ptsDrift)
{
}

Clock &Clock::operator =(const Clock &other)
{
    if (this != &other)
    {
        this->m_pts = other.m_pts;
        this->m_ptsDrift = other.m_ptsDrift;
    }

    return *this;
}

double Clock::clock() const
{
    double time = QDateTime::currentMSecsSinceEpoch() / 1.0e3;

    return time + this->m_ptsDrift;
}

void Clock::setClockAt(double pts, double time)
{
    this->m_pts = pts;
    this->m_ptsDrift = this->m_pts - time;
}

void Clock::setClock(double pts)
{
    double time = QDateTime::currentMSecsSinceEpoch() / 1.0e3;
    this->setClockAt(pts, time);
}

void Clock::syncTo(const Clock &slave)
{
    double clock = this->clock();
    double slaveClock = slave.clock();

    if (!isnan(slaveClock) &&
        (isnan(clock) ||
         fabs(clock - slaveClock) > AV_NOSYNC_THRESHOLD))
        this->setClock(slaveClock);
}
