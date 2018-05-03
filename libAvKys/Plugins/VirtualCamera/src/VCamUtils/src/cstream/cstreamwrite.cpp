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

#include <vector>

#include "cstreamwrite.h"

namespace AkVCam
{
    class CStreamWritePrivate
    {
        public:
            std::vector<char> m_data;
    };
}

AkVCam::CStreamWrite::CStreamWrite()
{
    this->d = new CStreamWritePrivate;
}

AkVCam::CStreamWrite::CStreamWrite(const CStreamWrite &other)
{
    this->d = new CStreamWritePrivate;
    this->d->m_data = other.d->m_data;
}

AkVCam::CStreamWrite::~CStreamWrite()
{
    delete this->d;
}

AkVCam::CStreamWrite &AkVCam::CStreamWrite::operator =(const CStreamWrite &other)
{
    if (this != &other) {
        this->d->m_data = other.d->m_data;
    }

    return *this;
}

AkVCam::CStreamWrite::operator bool() const
{
    return this->d->m_data.size() > 0;
}

const char *AkVCam::CStreamWrite::data() const
{
    return this->d->m_data.data();
}

size_t AkVCam::CStreamWrite::size() const
{
    return this->d->m_data.size();
}

void AkVCam::CStreamWrite::write(const std::string &data)
{
    auto oldSize = this->d->m_data.size();
    this->d->m_data.resize(this->d->m_data.size() + data.size() + 1);
    memcpy(this->d->m_data.data() + oldSize, data.data(), data.size());
    this->d->m_data[this->d->m_data.size() - 1] = 0;
}

void AkVCam::CStreamWrite::write(const char *data, size_t dataSize)
{
    auto oldSize = this->d->m_data.size();
    this->d->m_data.resize(this->d->m_data.size() + dataSize);
    memcpy(this->d->m_data.data() + oldSize, data, dataSize);
}
