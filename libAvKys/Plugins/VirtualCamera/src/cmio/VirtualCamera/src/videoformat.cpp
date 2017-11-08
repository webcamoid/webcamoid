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

Ak::VideoFormat::VideoFormat():
    m_codecType(0),
    m_width(0),
    m_height(0)
{

}

Ak::VideoFormat::VideoFormat(CMVideoCodecType codecType,
                             int32_t width,
                             int32_t height,
                             const std::vector<Float64> &frameRates):
    m_codecType(codecType),
    m_width(width),
    m_height(height),
    m_frameRates(frameRates)
{

}

Ak::VideoFormat::VideoFormat(const Ak::VideoFormat &other)
{
    this->m_codecType = other.m_codecType;
    this->m_width = other.m_width;
    this->m_height = other.m_height;
    this->m_frameRates = other.m_frameRates;
}

Ak::VideoFormat &Ak::VideoFormat::operator =(const Ak::VideoFormat &other)
{
    if (this != &other) {
        this->m_codecType = other.m_codecType;
        this->m_width = other.m_width;
        this->m_height = other.m_height;
        this->m_frameRates = other.m_frameRates;
    }

    return *this;
}

Ak::VideoFormat::~VideoFormat()
{

}

CMVideoCodecType Ak::VideoFormat::codecType() const
{
    return this->m_codecType;
}

CMVideoCodecType &Ak::VideoFormat::codecType()
{
    return this->m_codecType;
}

int32_t Ak::VideoFormat::width() const
{
    return this->m_width;
}

int32_t &Ak::VideoFormat::width()
{
    return this->m_width;
}

int32_t Ak::VideoFormat::height() const
{
    return this->m_height;
}

int32_t &Ak::VideoFormat::height()
{
    return this->m_height;
}

std::vector<Float64> Ak::VideoFormat::frameRates() const
{
    return this->m_frameRates;
}

std::vector<Float64> &Ak::VideoFormat::frameRates()
{
    return this->m_frameRates;
}

std::vector<AudioValueRange> Ak::VideoFormat::frameRateRanges() const
{
    std::vector<AudioValueRange> ranges;

    if (!this->m_frameRates.empty()) {
        auto min = *std::min_element(this->m_frameRates.begin(),
                                     this->m_frameRates.end());
        auto max = *std::max_element(this->m_frameRates.begin(),
                                     this->m_frameRates.end());
        ranges.push_back({min, max});
    }

    return ranges;
}

Float64 Ak::VideoFormat::minimumFrameRate() const
{
    if (this->m_frameRates.empty())
        return 0.0;

    return *std::min_element(this->m_frameRates.begin(),
                             this->m_frameRates.end());
}

CMFormatDescriptionRef Ak::VideoFormat::create() const
{
    CMFormatDescriptionRef formatDescription = nullptr;

    auto status =
            CMVideoFormatDescriptionCreate(kCFAllocatorDefault,
                                           this->m_codecType,
                                           this->m_width,
                                           this->m_height,
                                           nullptr,
                                           &formatDescription);

    if (status != noErr)
        return nullptr;

    return formatDescription;
}
