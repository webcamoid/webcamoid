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

#include "lock.h"

Lock::Lock(QObject *parent): QObject(parent)
{
    this->init();
}

Lock::Lock(int nLocks)
{
    this->init(nLocks);
}

void Lock::init(int nLocks)
{
    this->m_nLocked = 0;
    this->m_nLocks = nLocks;
}

void Lock::lock()
{
    this->m_counterLock.lock();
    this->m_nLocked += 1;

    if (this->m_nLocked == this->m_nLocks)
        this->m_lock.lock();

    this->m_counterLock.unlock();
}

void Lock::unlock()
{
    this->m_counterLock.lock();

    if (this->m_nLocked == this->m_nLocks)
        this->m_lock.unlock();

    this->m_nLocked -= 1;
    this->m_counterLock.unlock();
}

void Lock::wait()
{
    this->m_lock.lock();
    this->m_lock.unlock();
}
