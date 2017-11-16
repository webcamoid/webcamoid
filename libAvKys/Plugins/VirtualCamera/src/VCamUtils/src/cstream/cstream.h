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

#ifndef CSTREAM_H
#define CSTREAM_H

#include <algorithm>
#include <cstring>

namespace AkVCam
{
    class CStreamReadPrivate;

    class CStreamRead
    {
        public:
            enum Seek
            {
                SeekSet,
                SeekCur,
                SeekEnd
            };

            CStreamRead(const char *stream=nullptr,
                        size_t size=0,
                        bool isBigEndian=false);
            CStreamRead(const unsigned char *stream,
                        size_t size=0,
                        bool isBigEndian=false);
            CStreamRead(const char *stream,
                        bool isBigEndian);
            CStreamRead(const unsigned char *stream,
                        bool isBigEndian);
            CStreamRead(const CStreamRead &other);
            ~CStreamRead();
            CStreamRead &operator =(const CStreamRead &other);
            operator bool() const;

            const char *data() const;

            template<typename T>
            inline const T *data() const
            {
                return reinterpret_cast<const T *>(this->data());
            }

            size_t size() const;
            bool isBigEndian() const;

            template<typename T>
            inline T read()
            {
                T value;
                memcpy(&value, this->data(), sizeof(T));

                if (this->isBigEndian())
                    std::reverse(reinterpret_cast<char *>(&value),
                                 reinterpret_cast<char *>(&value) + sizeof(T));

                this->seek(sizeof(T));

                return value;
            }

            void seek(int offset, Seek origin=SeekCur);

            template<typename T>
            inline void seek(Seek origin=SeekCur)
            {
                this->seek(sizeof(T), origin);
            }

        private:
            CStreamReadPrivate *d;
    };
}

#endif // CSTREAM_H
