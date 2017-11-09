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

#include "videoformat.h"

VideoFormat::VideoFormat():
    m_fourcc(0),
    m_width(0),
    m_height(0),
    m_frameRate(0)
{
}

VideoFormat::VideoFormat(FourCC fourcc,
                         int width,
                         int height,
                         int frameRate):
    m_fourcc(fourcc),
    m_width(width),
    m_height(height),
    m_frameRate(frameRate)
{

}

VideoFormat::VideoFormat(const VideoFormat &other)
{
    this->m_fourcc = other.m_fourcc;
    this->m_width = other.m_width;
    this->m_height = other.m_height;
    this->m_frameRate = other.m_frameRate;
}

VideoFormat &VideoFormat::operator =(const VideoFormat &other)
{
    if (this != &other) {
        this->m_fourcc = other.m_fourcc;
        this->m_width = other.m_width;
        this->m_height = other.m_height;
        this->m_frameRate = other.m_frameRate;
    }

    return *this;
}

VideoFormat::~VideoFormat()
{

}

VideoFormat::FourCC VideoFormat::fourcc() const
{
    return this->m_fourcc;
}

VideoFormat::FourCC &VideoFormat::fourcc()
{
    return this->m_fourcc;
}

int VideoFormat::width() const
{
    return this->m_width;
}

int &VideoFormat::width()
{
    return this->m_width;
}

int VideoFormat::height() const
{
    return this->m_height;
}

int &VideoFormat::height()
{
    return this->m_height;
}

int VideoFormat::frameRate() const
{
    return this->m_frameRate;
}

int &VideoFormat::frameRate()
{
    return this->m_frameRate;
}
