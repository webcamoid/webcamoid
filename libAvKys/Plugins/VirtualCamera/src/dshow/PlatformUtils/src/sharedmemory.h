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

#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <string>

namespace AkVCam
{
    class SharedMemoryPrivate;
    class Mutex;

    class SharedMemory
    {
        public:
            enum OpenMode
            {
                OpenModeRead,
                OpenModeWrite
            };

            SharedMemory();
            SharedMemory(const SharedMemory &other);
            ~SharedMemory();
            SharedMemory &operator =(const SharedMemory &other);

            std::wstring name() const;
            std::wstring &name();
            void setName(const std::wstring &name);
            bool open(size_t pageSize=0, OpenMode mode=OpenModeRead);
            bool isOpen() const;
            size_t pageSize() const;
            OpenMode mode() const;
            void *lock(Mutex *mutex=nullptr, int timeout=0);
            void unlock(Mutex *mutex=nullptr);
            void close();

        private:
            SharedMemoryPrivate *d;
    };
}

#endif // SHAREDMEMORY_H
