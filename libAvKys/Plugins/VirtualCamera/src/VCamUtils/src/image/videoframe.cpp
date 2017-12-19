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

#include <cstring>

#include "videoframe.h"
#include "videoformat.h"

namespace AkVCam
{
    class VideoFormatPrivate
    {
        public:
            VideoFormat m_format;
            std::shared_ptr<uint8_t> m_data;
            size_t m_dataSize;

            VideoFormatPrivate():
                m_dataSize(0)
            {

            }
    };
}

AkVCam::VideoFrame::VideoFrame()
{
    this->d = new VideoFormatPrivate;
}

AkVCam::VideoFrame::VideoFrame(const AkVCam::VideoFormat &format,
                               const std::shared_ptr<uint8_t> &data,
                               size_t dataSize)
{
    this->d = new VideoFormatPrivate;
    this->d->m_format = format;
    this->d->m_data = data;
    this->d->m_dataSize = dataSize;
}

AkVCam::VideoFrame::VideoFrame(const AkVCam::VideoFormat &format,
                               const uint8_t *data,
                               size_t dataSize)
{
    this->d = new VideoFormatPrivate;
    this->d->m_format = format;
    this->d->m_data = std::shared_ptr<uint8_t>(new uint8_t[dataSize]);
    memcpy(this->d->m_data.get(), data, dataSize);
    this->d->m_dataSize = dataSize;
}

AkVCam::VideoFrame::VideoFrame(const AkVCam::VideoFrame &other)
{
    this->d = new VideoFormatPrivate;
    this->d->m_format = other.d->m_format;
    this->d->m_data = other.d->m_data;
    this->d->m_dataSize = other.d->m_dataSize;
}

AkVCam::VideoFrame &AkVCam::VideoFrame::operator =(const AkVCam::VideoFrame &other)
{
    if (this != &other) {
        this->d->m_format = other.d->m_format;
        this->d->m_data = other.d->m_data;
        this->d->m_dataSize = other.d->m_dataSize;
    }

    return *this;
}

AkVCam::VideoFrame::~VideoFrame()
{
    delete this->d;
}

AkVCam::VideoFormat AkVCam::VideoFrame::format() const
{
    return this->d->m_format;
}

AkVCam::VideoFormat &AkVCam::VideoFrame::format()
{
    return this->d->m_format;
}

std::shared_ptr<uint8_t> AkVCam::VideoFrame::data() const
{
    return this->d->m_data;
}

std::shared_ptr<uint8_t> &AkVCam::VideoFrame::data()
{
    return this->d->m_data;
}

size_t AkVCam::VideoFrame::dataSize() const
{
    return this->d->m_dataSize;
}

size_t &AkVCam::VideoFrame::dataSize()
{
    return this->d->m_dataSize;
}
