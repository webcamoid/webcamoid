/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include <windows.h>

#include "mutex.h"

class MutexPrivate
{
    public:
        HANDLE m_mutex;
        std::wstring m_name;
};

Mutex::Mutex(const std::wstring &name)
{
    this->d = new MutexPrivate();
    this->d->m_mutex = CreateMutex(nullptr,
                                   FALSE,
                                   name.empty()?
                                       nullptr: name.c_str());
    this->d->m_name = name;
}

Mutex::Mutex(const Mutex &other)
{
    this->d = new MutexPrivate();
    this->d->m_mutex = CreateMutex(nullptr,
                                   FALSE,
                                   other.d->m_name.empty()?
                                       nullptr: other.d->m_name.c_str());
    this->d->m_name = other.d->m_name;
}

Mutex::~Mutex()
{
    if (this->d->m_mutex)
        CloseHandle(this->d->m_mutex);

    delete this->d;
}

Mutex &Mutex::operator =(const Mutex &other)
{
    if (this != &other) {
        this->unlock();

        if (this->d->m_mutex)
            CloseHandle(this->d->m_mutex);

        this->d->m_mutex = CreateMutex(nullptr,
                                       FALSE,
                                       other.d->m_name.empty()?
                                           nullptr: other.d->m_name.c_str());
        this->d->m_name = other.d->m_name;
    }

    return *this;
}

std::wstring Mutex::name() const
{
    return this->d->m_name;
}

void Mutex::lock()
{
    if (!this->d->m_mutex)
        return;

    WaitForSingleObject(this->d->m_mutex, INFINITE);
}

bool Mutex::tryLock(int timeout)
{
    if (!this->d->m_mutex)
        return false;

    DWORD waitResult = WaitForSingleObject(this->d->m_mutex,
                                           !timeout? INFINITE: DWORD(timeout));

    return waitResult != WAIT_FAILED && waitResult != WAIT_TIMEOUT;
}

void Mutex::unlock()
{
    if (!this->d->m_mutex)
        return;

    ReleaseMutex(this->d->m_mutex);
}
