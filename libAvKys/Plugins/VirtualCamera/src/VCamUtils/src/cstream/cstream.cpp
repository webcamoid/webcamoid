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

#include "cstream.h"

namespace AkVCam
{
    class CStreamReadPrivate
    {
        public:
            const char *m_stream;
            const char *m_ptr;
            size_t m_size;
            bool m_isBigEndian;
    };
}

AkVCam::CStreamRead::CStreamRead(const char *stream,
                                 size_t size,
                                 bool isBigEndian)
{
    this->d = new CStreamReadPrivate();
    this->d->m_stream = stream;
    this->d->m_ptr = stream;
    this->d->m_size = size;
    this->d->m_isBigEndian = isBigEndian;
}

AkVCam::CStreamRead::CStreamRead(const unsigned char *stream,
                                 size_t size,
                                 bool isBigEndian)
{
    this->d = new CStreamReadPrivate();
    this->d->m_stream = reinterpret_cast<const char *>(stream);
    this->d->m_ptr = reinterpret_cast<const char *>(stream);
    this->d->m_size = size;
    this->d->m_isBigEndian = isBigEndian;
}

AkVCam::CStreamRead::CStreamRead(const char *stream, bool isBigEndian)
{
    this->d = new CStreamReadPrivate();
    this->d->m_stream = stream;
    this->d->m_ptr = stream;
    this->d->m_size = 0;
    this->d->m_isBigEndian = isBigEndian;
}

AkVCam::CStreamRead::CStreamRead(const unsigned char *stream, bool isBigEndian)
{
    this->d = new CStreamReadPrivate();
    this->d->m_stream = reinterpret_cast<const char *>(stream);
    this->d->m_ptr = reinterpret_cast<const char *>(stream);
    this->d->m_size = 0;
    this->d->m_isBigEndian = isBigEndian;
}

AkVCam::CStreamRead::CStreamRead(const CStreamRead &other)
{
    this->d = new CStreamReadPrivate();
    this->d->m_stream = other.d->m_stream;
    this->d->m_ptr = other.d->m_stream;
    this->d->m_size = other.d->m_size;
    this->d->m_isBigEndian = other.d->m_isBigEndian;
}

AkVCam::CStreamRead::~CStreamRead()
{
    delete this->d;
}

AkVCam::CStreamRead &AkVCam::CStreamRead::operator =(const CStreamRead &other)
{
    if (this != &other) {
        this->d->m_stream = other.d->m_stream;
        this->d->m_ptr = other.d->m_stream;
        this->d->m_size = other.d->m_size;
        this->d->m_isBigEndian = other.d->m_isBigEndian;
    }

    return *this;
}

AkVCam::CStreamRead::operator bool() const
{
    return this->d->m_stream != nullptr;
}

const char *AkVCam::CStreamRead::data() const
{
    return this->d->m_ptr;
}

size_t AkVCam::CStreamRead::size() const
{
    return this->d->m_size;
}

bool AkVCam::CStreamRead::isBigEndian() const
{
    return this->d->m_isBigEndian;
}

void AkVCam::CStreamRead::seek(int offset, Seek origin)
{
    switch (origin) {
    case SeekSet:
        this->d->m_ptr = this->d->m_stream + offset;

        break;

    case SeekCur:
        this->d->m_ptr += offset;

        break;

    case SeekEnd:
        this->d->m_ptr =
                this->d->m_stream
                + (int64_t(this->d->m_size) + offset - 1)
                % int64_t(this->d->m_size);

        break;
    }
}
