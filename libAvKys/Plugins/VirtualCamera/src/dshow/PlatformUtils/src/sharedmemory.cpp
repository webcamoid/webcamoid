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

#include <windows.h>

#include "sharedmemory.h"
#include "mutex.h"
#include "utils.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class SharedMemoryPrivate
    {
        public:
            HANDLE m_sharedHandle;
            std::wstring m_name;
            void *m_buffer;
            size_t m_pageSize;
            SharedMemory::OpenMode m_mode;
            bool m_isOpen;
    };
}

AkVCam::SharedMemory::SharedMemory()
{
    this->d = new SharedMemoryPrivate;
    this->d->m_sharedHandle = nullptr;
    this->d->m_buffer = nullptr;
    this->d->m_pageSize = 0;
    this->d->m_mode = OpenModeRead;
    this->d->m_isOpen = false;
}

AkVCam::SharedMemory::SharedMemory(const SharedMemory &other)
{
    this->d = new SharedMemoryPrivate;
    this->d->m_sharedHandle = nullptr;
    this->d->m_name = other.d->m_name;
    this->d->m_buffer = nullptr;
    this->d->m_pageSize = 0;
    this->d->m_mode = OpenModeRead;
    this->d->m_isOpen = false;

    if (other.d->m_isOpen)
        this->open(other.d->m_pageSize, other.d->m_mode);
}

AkVCam::SharedMemory::~SharedMemory()
{
    this->close();
    delete this->d;
}

AkVCam::SharedMemory &AkVCam::SharedMemory::operator =(const SharedMemory &other)
{
    if (this != &other) {
        this->close();
        this->d->m_name = other.d->m_name;
        this->d->m_buffer = nullptr;
        this->d->m_pageSize = 0;
        this->d->m_mode = OpenModeRead;
        this->d->m_isOpen = false;

        if (other.d->m_isOpen)
            this->open(other.d->m_pageSize, other.d->m_mode);
    }

    return *this;
}

std::wstring AkVCam::SharedMemory::name() const
{
    return this->d->m_name;
}

std::wstring &AkVCam::SharedMemory::name()
{
    return this->d->m_name;
}

void AkVCam::SharedMemory::setName(const std::wstring &name)
{
    this->d->m_name = name;
}

bool AkVCam::SharedMemory::open(size_t pageSize, OpenMode mode)
{
    if (this->d->m_isOpen)
        return false;

    if (this->d->m_name.empty())
        return false;

    if (mode == OpenModeRead) {
        this->d->m_sharedHandle =
                OpenFileMapping(FILE_MAP_ALL_ACCESS,
                                FALSE,
                                this->d->m_name.c_str());
    } else {
        if (pageSize < 1)
            return false;

        this->d->m_sharedHandle =
                CreateFileMapping(INVALID_HANDLE_VALUE,
                                  nullptr,
                                  PAGE_READWRITE,
                                  0,
                                  DWORD(pageSize),
                                  this->d->m_name.c_str());
    }

    if (!this->d->m_sharedHandle) {
        AkLoggerLog("Error opening shared memory (",
                    std::string(this->d->m_name.begin(),
                                this->d->m_name.end()),
                    "): ",
                    errorToString(GetLastError()),
                    " (", GetLastError(), ")");

        return false;
    }

    this->d->m_buffer = MapViewOfFile(this->d->m_sharedHandle,
                                      FILE_MAP_ALL_ACCESS,
                                      0,
                                      0,
                                      pageSize);

    if (!this->d->m_buffer) {
        CloseHandle(this->d->m_sharedHandle);
        this->d->m_sharedHandle = nullptr;

        return false;
    }

    this->d->m_pageSize = pageSize;
    this->d->m_mode = mode;
    this->d->m_isOpen = true;

    return true;
}

bool AkVCam::SharedMemory::isOpen() const
{
    return this->d->m_isOpen;
}

size_t AkVCam::SharedMemory::pageSize() const
{
    return this->d->m_pageSize;
}

AkVCam::SharedMemory::OpenMode AkVCam::SharedMemory::mode() const
{
    return this->d->m_mode;
}

void *AkVCam::SharedMemory::lock(AkVCam::Mutex *mutex, int timeout)
{
    if (mutex && !mutex->tryLock(timeout))
        return nullptr;

    return this->d->m_buffer;
}

void AkVCam::SharedMemory::unlock(AkVCam::Mutex *mutex)
{
    if (mutex)
        mutex->unlock();
}

void AkVCam::SharedMemory::close()
{
    if (this->d->m_buffer) {
        UnmapViewOfFile(this->d->m_buffer);
        this->d->m_buffer = nullptr;
    }

    if (this->d->m_sharedHandle) {
        CloseHandle(this->d->m_sharedHandle);
        this->d->m_sharedHandle = nullptr;
    }

    this->d->m_pageSize = 0;
    this->d->m_mode = OpenModeRead;
    this->d->m_isOpen = false;
}
