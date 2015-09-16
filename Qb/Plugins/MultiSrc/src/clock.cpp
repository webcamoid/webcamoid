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

#include <QDateTime>

#include "clock.h"

Clock::Clock(QObject *parent): QObject(parent)
{
    this->m_timeDrift = 0.0;
}

qreal Clock::clock()
{
    this->m_mutex.lockForRead();
    qreal clock = QDateTime::currentMSecsSinceEpoch() * 1.0e-3
                  - this->m_timeDrift;
    this->m_mutex.unlock();

    return clock;
}

void Clock::setClock(qreal clock)
{
    this->m_mutex.lockForWrite();
    this->m_timeDrift = QDateTime::currentMSecsSinceEpoch() * 1.0e-3
                        - clock;
    this->m_mutex.unlock();
}

void Clock::resetClock()
{
    this->m_mutex.lockForWrite();
    this->m_timeDrift = 0.0;
    this->m_mutex.unlock();
}
