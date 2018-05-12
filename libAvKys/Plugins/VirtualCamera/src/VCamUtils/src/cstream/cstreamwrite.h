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

#ifndef CSTREAMWRITE_H
#define CSTREAMWRITE_H

#include <string>

namespace AkVCam
{
    class CStreamWritePrivate;

    class CStreamWrite
    {
        public:
            CStreamWrite();
            CStreamWrite(const CStreamWrite &other);
            ~CStreamWrite();
            CStreamWrite &operator =(const CStreamWrite &other);
            operator bool() const;

            const char *data() const;

            template<typename T>
            inline const T *data() const
            {
                return reinterpret_cast<const T *>(this->data());
            }

            size_t size() const;
            void write(const std::string &data);
            void write(const char *data, size_t dataSize);
            template<typename T>
            void write(T data)
            {
                write(&data, sizeof(T));
            }

        private:
            CStreamWritePrivate *d;
    };
}

#endif // CSTREAMWRITE_H
