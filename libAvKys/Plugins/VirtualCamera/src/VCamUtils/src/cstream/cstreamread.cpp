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

#include <atomic>
#include <cstdint>

#include "cstreamread.h"

namespace AkVCam
{
    class CStreamReadPrivate
    {
        public:
            CStreamRead::Mode m_mode;
            char *m_stream;
            char *m_ptr;
            size_t m_size;
            uint64_t *m_ref;
            bool m_isBigEndian;
    };
}

AkVCam::CStreamRead::CStreamRead(const char *stream,
                                 size_t size,
                                 bool isBigEndian,
                                 Mode mode)
{
    this->d = new CStreamReadPrivate();
    this->d->m_mode = mode;

    if (mode == ModeRead) {
        this->d->m_stream = const_cast<char *>(stream);
        this->d->m_ref = nullptr;
    } else if (mode == ModeHold) {
        this->d->m_stream = const_cast<char *>(stream);
        this->d->m_ref = new uint64_t(1);
    } else {
        this->d->m_stream = new char[size];
        memcpy(this->d->m_stream, stream, size);
        this->d->m_ref = new uint64_t(1);
    }

    this->d->m_ptr = this->d->m_stream;
    this->d->m_size = size;
    this->d->m_isBigEndian = isBigEndian;
}

AkVCam::CStreamRead::CStreamRead(const unsigned char *stream,
                                 size_t size,
                                 bool isBigEndian,
                                 Mode mode)
{
    this->d = new CStreamReadPrivate();
    this->d->m_mode = mode;

    if (mode == ModeRead) {
        this->d->m_stream = reinterpret_cast<char *>(const_cast<unsigned char *>(stream));
        this->d->m_ref = nullptr;
    } else if (mode == ModeHold) {
        this->d->m_stream = reinterpret_cast<char *>(const_cast<unsigned char *>(stream));
        this->d->m_ref = new uint64_t(1);
    } else {
        this->d->m_stream = new char[size];
        memcpy(this->d->m_stream, stream, size);
        this->d->m_ref = new uint64_t(1);
    }

    this->d->m_ptr = this->d->m_stream;
    this->d->m_size = size;
    this->d->m_isBigEndian = isBigEndian;
}

AkVCam::CStreamRead::CStreamRead(const char *stream,
                                 size_t size,
                                 Mode mode)
{
    this->d = new CStreamReadPrivate();
    this->d->m_mode = mode;

    if (mode == ModeRead) {
        this->d->m_stream = const_cast<char *>(stream);
        this->d->m_ref = nullptr;
    } else if (mode == ModeHold) {
        this->d->m_stream = const_cast<char *>(stream);
        this->d->m_ref = new uint64_t(1);
    } else {
        this->d->m_stream = new char[size];
        memcpy(this->d->m_stream, stream, size);
        this->d->m_ref = new uint64_t(1);
    }

    this->d->m_ptr = this->d->m_stream;
    this->d->m_size = size;
    this->d->m_isBigEndian = false;
}

AkVCam::CStreamRead::CStreamRead(const unsigned char *stream,
                                 size_t size,
                                 Mode mode)
{
    this->d = new CStreamReadPrivate();
    this->d->m_mode = mode;

    if (mode == ModeRead) {
        this->d->m_stream = reinterpret_cast<char *>(const_cast<unsigned char *>(stream));
        this->d->m_ref = nullptr;
    } else if (mode == ModeHold) {
        this->d->m_stream = reinterpret_cast<char *>(const_cast<unsigned char *>(stream));
        this->d->m_ref = new uint64_t(1);
    } else {
        this->d->m_stream = new char[size];
        memcpy(this->d->m_stream, stream, size);
        this->d->m_ref = new uint64_t(1);
    }

    this->d->m_ptr = this->d->m_stream;
    this->d->m_size = size;
    this->d->m_isBigEndian = false;
}

AkVCam::CStreamRead::CStreamRead(const char *stream, bool isBigEndian)
{
    this->d = new CStreamReadPrivate();
    this->d->m_mode = ModeRead;
    this->d->m_stream = const_cast<char *>(stream);
    this->d->m_ptr = this->d->m_stream;
    this->d->m_size = 0;
    this->d->m_ref = nullptr;
    this->d->m_isBigEndian = isBigEndian;
}

AkVCam::CStreamRead::CStreamRead(const unsigned char *stream, bool isBigEndian)
{
    this->d = new CStreamReadPrivate();
    this->d->m_mode = ModeRead;
    this->d->m_stream = reinterpret_cast<char *>(const_cast<unsigned char *>(stream));
    this->d->m_ptr = this->d->m_stream;
    this->d->m_size = 0;
    this->d->m_ref = nullptr;
    this->d->m_isBigEndian = isBigEndian;
}

AkVCam::CStreamRead::CStreamRead(const CStreamRead &other)
{
    this->d = new CStreamReadPrivate();
    this->d->m_mode = other.d->m_mode;
    this->d->m_stream = other.d->m_stream;
    this->d->m_ptr = other.d->m_stream;
    this->d->m_size = other.d->m_size;
    this->d->m_ref = other.d->m_ref;
    this->d->m_isBigEndian = other.d->m_isBigEndian;

    if (this->d->m_mode != ModeRead)
        (*this->d->m_ref)++;
}

AkVCam::CStreamRead::~CStreamRead()
{
    if (this->d->m_mode != ModeRead) {
        (*this->d->m_ref)--;

        if (!*this->d->m_ref) {
            delete [] this->d->m_stream;
            delete this->d->m_ref;
        }
    }

    delete this->d;
}

AkVCam::CStreamRead &AkVCam::CStreamRead::operator =(const CStreamRead &other)
{
    if (this != &other) {
        if (this->d->m_mode != ModeRead) {
            (*this->d->m_ref)--;

            if (!*this->d->m_ref) {
                delete [] this->d->m_stream;
                delete this->d->m_ref;
            }
        }

        this->d->m_mode = other.d->m_mode;
        this->d->m_stream = other.d->m_stream;
        this->d->m_ptr = other.d->m_stream;
        this->d->m_size = other.d->m_size;
        this->d->m_ref = other.d->m_ref;
        this->d->m_isBigEndian = other.d->m_isBigEndian;

        if (this->d->m_mode != ModeRead)
            (*this->d->m_ref)++;
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
