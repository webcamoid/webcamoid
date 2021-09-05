/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#include <QDateTime>
#include <QReadWriteLock>

#include "clock.h"

class ClockPrivate
{
    public:
        QReadWriteLock m_mutex;
        qreal m_timeDrift {0.0};
};

Clock::Clock(QObject *parent): QObject(parent)
{
    this->d = new ClockPrivate;
}

Clock::~Clock()
{
    delete this->d;
}

qreal Clock::clock()
{
    this->d->m_mutex.lockForRead();
    qreal clock = QDateTime::currentMSecsSinceEpoch() * 1.0e-3
                  - this->d->m_timeDrift;
    this->d->m_mutex.unlock();

    return clock;
}

void Clock::setClock(qreal clock)
{
    this->d->m_mutex.lockForWrite();
    this->d->m_timeDrift = QDateTime::currentMSecsSinceEpoch() * 1.0e-3
                           - clock;
    this->d->m_mutex.unlock();
}

void Clock::resetClock()
{
    this->d->m_mutex.lockForWrite();
    this->d->m_timeDrift = 0.0;
    this->d->m_mutex.unlock();
}

#include "moc_clock.cpp"
