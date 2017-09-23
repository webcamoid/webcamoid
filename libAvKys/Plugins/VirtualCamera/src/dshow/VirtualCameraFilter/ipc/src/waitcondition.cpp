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

#include "waitcondition.h"

class WaitConditionPrivate
{
    public:
        HANDLE m_event;
        std::wstring m_name;
};

WaitCondition::WaitCondition(const std::wstring &name)
{
    this->d = new WaitConditionPrivate();
    this->d->m_event = CreateEvent(nullptr,
                                   TRUE,
                                   FALSE,
                                   name.empty()?
                                       nullptr: name.c_str());
    this->d->m_name = name;
}

WaitCondition::WaitCondition(Mutex *mutex)
{
    this->d = new WaitConditionPrivate();
    this->d->m_name = mutex->name().empty()?
                          std::wstring(): mutex->name() + L".wait";
    this->d->m_event = CreateEvent(nullptr,
                                   TRUE,
                                   FALSE,
                                   mutex->name().empty()?
                                       nullptr: this->d->m_name.c_str());
}

WaitCondition::WaitCondition(const WaitCondition &other)
{
    this->d = new WaitConditionPrivate();
    this->d->m_event = CreateEvent(nullptr,
                                   TRUE,
                                   FALSE,
                                   other.d->m_name.empty()?
                                       nullptr: other.d->m_name.c_str());
    this->d->m_name = other.d->m_name;
}

WaitCondition::~WaitCondition()
{
    if (this->d->m_event)
        CloseHandle(this->d->m_event);

    delete this->d;
}

WaitCondition &WaitCondition::operator =(const WaitCondition &other)
{
    if (this != &other) {
        if (this->d->m_event)
            CloseHandle(this->d->m_event);

        this->d->m_event = CreateEvent(nullptr,
                                       TRUE,
                                       FALSE,
                                       other.d->m_name.empty()?
                                           nullptr: other.d->m_name.c_str());
        this->d->m_name = other.d->m_name;
    }

    return *this;
}

bool WaitCondition::wait(Mutex *mutex, int timeout)
{
    if (!mutex || !this->d->m_event)
        return false;

    mutex->unlock();

    DWORD result = WaitForSingleObject(this->d->m_event,
                                       !timeout? INFINITE: DWORD(timeout));

    mutex->lock();
    ResetEvent(this->d->m_event);

    return result == WAIT_OBJECT_0;
}

void WaitCondition::wakeAll()
{
    if (this->d->m_event)
        SetEvent(this->d->m_event);
}
