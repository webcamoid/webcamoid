/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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

#ifndef AKVCAMUTILS_IMEMBUFFER_H
#define AKVCAMUTILS_IMEMBUFFER_H

#include <algorithm>
#include <streambuf>

namespace AkVCam
{
    class IMemBufferPrivate;

    class IMemBuffer: public std::streambuf
    {
        public:
            enum Mode
            {
                ModeRead,
                ModeHold,
                ModeCopy
            };

            IMemBuffer(const char *stream=nullptr,
                       size_t size=0,
                       bool isBigEndian=false,
                       Mode mode=ModeRead);
            IMemBuffer(const unsigned char *stream,
                       size_t size=0,
                       bool isBigEndian=false,
                       Mode mode=ModeRead);
            IMemBuffer(const char *stream, size_t size, Mode mode);
            IMemBuffer(const unsigned char *stream, size_t size, Mode mode);
            IMemBuffer(const char *stream, bool isBigEndian);
            IMemBuffer(const unsigned char *stream, bool isBigEndian);
            IMemBuffer(const IMemBuffer &other);
            ~IMemBuffer();
            IMemBuffer &operator =(const IMemBuffer &other);
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

                this->gbump(sizeof(T));

                return value;
            }

            template<typename T>
            inline std::streampos seek(std::ios_base::seekdir way=std::ios_base::cur)
            {
                return this->seekoff(sizeof(T), way);
            }

        private:
            IMemBufferPrivate *d;

        protected:
            std::streampos seekoff(std::streamoff off,
                                   std::ios_base::seekdir way=std::ios_base::cur,
                                   std::ios_base::openmode which=std::ios_base::in|std::ios_base::out);
            std::streampos seekpos(std::streampos sp,
                                   std::ios_base::openmode which=std::ios_base::in | std::ios_base::out);
            std::streamsize showmanyc();
            std::streamsize xsgetn(char *s, std::streamsize n);
    };

    template<>
    inline std::string IMemBuffer::read<std::string>()
    {
        if (!this->data())
            return {};

        std::string str(this->data());
        this->gbump(int(str.size() + 1));

        return str;
}}

#endif // AKVCAMUTILS_IMEMBUFFER_H
