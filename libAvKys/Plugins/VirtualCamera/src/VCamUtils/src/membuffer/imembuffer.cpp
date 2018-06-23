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

#include "imembuffer.h"
#include "../utils.h"

namespace AkVCam
{
    class IMemBufferPrivate
    {
        public:
            size_t m_size;
            uint64_t *m_ref;
            IMemBuffer::Mode m_mode;
            bool m_isBigEndian;
    };
}

AkVCam::IMemBuffer::IMemBuffer(const char *stream,
                               size_t size,
                               bool isBigEndian,
                               Mode mode)
{
    this->d = new IMemBufferPrivate;
    this->d->m_mode = mode;
    char *data = nullptr;

    if (mode == ModeRead) {
        data = const_cast<char *>(stream);
        this->d->m_ref = nullptr;
    } else if (mode == ModeHold) {
        data = const_cast<char *>(stream);
        this->d->m_ref = new uint64_t(1);
    } else {
        data = new char[size];
        memcpy(data, stream, size);
        this->d->m_ref = new uint64_t(1);
    }

    this->d->m_size = size;
    this->d->m_isBigEndian = isBigEndian;
    this->setg(data, data, data + size - 1);
}

AkVCam::IMemBuffer::IMemBuffer(const unsigned char *stream,
                               size_t size,
                               bool isBigEndian,
                               Mode mode)
{
    this->d = new IMemBufferPrivate;
    this->d->m_mode = mode;
    char *data = nullptr;

    if (mode == ModeRead) {
        data = reinterpret_cast<char *>(const_cast<unsigned char *>(stream));
        this->d->m_ref = nullptr;
    } else if (mode == ModeHold) {
        data = reinterpret_cast<char *>(const_cast<unsigned char *>(stream));
        this->d->m_ref = new uint64_t(1);
    } else {
        data = new char[size];
        memcpy(data, stream, size);
        this->d->m_ref = new uint64_t(1);
    }

    this->d->m_size = size;
    this->d->m_isBigEndian = isBigEndian;
    this->setg(data, data, data + size - 1);
}

AkVCam::IMemBuffer::IMemBuffer(const char *stream, size_t size, Mode mode)
{
    this->d = new IMemBufferPrivate();
    this->d->m_mode = mode;
    char *data = nullptr;

    if (mode == ModeRead) {
        data = const_cast<char *>(stream);
        this->d->m_ref = nullptr;
    } else if (mode == ModeHold) {
        data = const_cast<char *>(stream);
        this->d->m_ref = new uint64_t(1);
    } else {
        data = new char[size];
        memcpy(data, stream, size);
        this->d->m_ref = new uint64_t(1);
    }

    this->d->m_size = size;
    this->d->m_isBigEndian = false;
    this->setg(data, data, data + size - 1);
}

AkVCam::IMemBuffer::IMemBuffer(const unsigned char *stream,
                               size_t size,
                               Mode mode)
{
    this->d = new IMemBufferPrivate();
    this->d->m_mode = mode;
    char *data = nullptr;

    if (mode == ModeRead) {
        data = reinterpret_cast<char *>(const_cast<unsigned char *>(stream));
        this->d->m_ref = nullptr;
    } else if (mode == ModeHold) {
        data = reinterpret_cast<char *>(const_cast<unsigned char *>(stream));
        this->d->m_ref = new uint64_t(1);
    } else {
        data = new char[size];
        memcpy(data, stream, size);
        this->d->m_ref = new uint64_t(1);
    }

    this->d->m_size = size;
    this->d->m_isBigEndian = false;
    this->setg(data, data, data + size - 1);
}

AkVCam::IMemBuffer::IMemBuffer(const char *stream, bool isBigEndian)
{
    this->d = new IMemBufferPrivate();
    this->d->m_mode = ModeRead;
    this->d->m_size = 0;
    this->d->m_ref = nullptr;
    this->d->m_isBigEndian = isBigEndian;
    this->setg(const_cast<char *>(stream),
               const_cast<char *>(stream),
               const_cast<char *>(stream));
}

AkVCam::IMemBuffer::IMemBuffer(const unsigned char *stream, bool isBigEndian)
{
    this->d = new IMemBufferPrivate();
    this->d->m_mode = ModeRead;
    this->d->m_size = 0;
    this->d->m_ref = nullptr;
    this->d->m_isBigEndian = isBigEndian;
    this->setg(reinterpret_cast<char *>(const_cast<unsigned char *>(stream)),
               reinterpret_cast<char *>(const_cast<unsigned char *>(stream)),
               reinterpret_cast<char *>(const_cast<unsigned char *>(stream)));
}

AkVCam::IMemBuffer::IMemBuffer(const AkVCam::IMemBuffer &other)
{
    this->d = new IMemBufferPrivate();
    this->d->m_mode = other.d->m_mode;
    this->d->m_size = other.d->m_size;
    this->d->m_ref = other.d->m_ref;
    this->d->m_isBigEndian = other.d->m_isBigEndian;
    this->setg(other.eback(), other.gptr(), other.egptr());

    if (this->d->m_mode != ModeRead)
        (*this->d->m_ref)++;
}

AkVCam::IMemBuffer::~IMemBuffer()
{
    if (this->d->m_mode != ModeRead) {
        (*this->d->m_ref)--;

        if (!*this->d->m_ref) {
            delete [] this->eback();
            delete this->d->m_ref;
        }
    }

    delete this->d;
}

AkVCam::IMemBuffer &AkVCam::IMemBuffer::operator =(const AkVCam::IMemBuffer &other)
{
    if (this != &other) {
        if (this->d->m_mode != ModeRead) {
            (*this->d->m_ref)--;

            if (!*this->d->m_ref) {
                delete [] this->eback();
                delete this->d->m_ref;
            }
        }

        this->d->m_mode = other.d->m_mode;
        this->d->m_size = other.d->m_size;
        this->d->m_ref = other.d->m_ref;
        this->d->m_isBigEndian = other.d->m_isBigEndian;
        this->setg(other.eback(), other.gptr(), other.egptr());

        if (this->d->m_mode != ModeRead)
            (*this->d->m_ref)++;
    }

    return *this;
}

AkVCam::IMemBuffer::operator bool() const
{
    return this->eback() != nullptr;
}

const char *AkVCam::IMemBuffer::data() const
{
    return this->gptr();
}

size_t AkVCam::IMemBuffer::size() const
{
    return this->d->m_size;
}

bool AkVCam::IMemBuffer::isBigEndian() const
{
    return this->d->m_isBigEndian;
}

std::streampos AkVCam::IMemBuffer::seekoff(std::streamoff off,
                                           std::ios_base::seekdir way,
                                           std::ios_base::openmode which)
{
    UNUSED(which)

    switch (way) {
    case std::ios_base::beg:
        this->setg(this->eback(),
                   this->eback() + off,
                   this->eback() + this->d->m_size - 1);

        break;

    case std::ios_base::cur:
        this->gbump(off);

        break;

    case std::ios_base::end:
        this->setg(this->eback(),
                   this->eback()
                   + (int64_t(this->d->m_size) + off - 1)
                   % int64_t(this->d->m_size),
                   this->eback() + this->d->m_size - 1);

        break;

    default:
        break;
    }

    return this->gptr() - this->eback();
}

std::streampos AkVCam::IMemBuffer::seekpos(std::streampos sp,
                                           std::ios_base::openmode which)
{
    return this->seekoff(sp, std::ios_base::beg, which);
}

std::streamsize AkVCam::IMemBuffer::showmanyc()
{
    return this->d->m_size - size_t(this->gptr()) + size_t(this->eback());
}

std::streamsize AkVCam::IMemBuffer::xsgetn(char *s, std::streamsize n)
{
    auto available = this->showmanyc();

    if (available < 1)
        return 0;

    n = std::min(n, std::streamsize(available));
    memcpy(s, this->data(), size_t(n));
    this->gbump(n);

    return n;
}
