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

#include <algorithm>

#include "videoformat.h"

AkVCam::VideoFormat::VideoFormat():
    m_fourcc(0),
    m_width(0),
    m_height(0)
{
}

AkVCam::VideoFormat::VideoFormat(FourCC fourcc,
                                 int width,
                                 int height,
                                 const std::vector<double> &frameRates):
    m_fourcc(fourcc),
    m_width(width),
    m_height(height),
    m_frameRates(frameRates)
{

}

AkVCam::VideoFormat::VideoFormat(const VideoFormat &other)
{
    this->m_fourcc = other.m_fourcc;
    this->m_width = other.m_width;
    this->m_height = other.m_height;
    this->m_frameRates = other.m_frameRates;
}

AkVCam::VideoFormat &AkVCam::VideoFormat::operator =(const VideoFormat &other)
{
    if (this != &other) {
        this->m_fourcc = other.m_fourcc;
        this->m_width = other.m_width;
        this->m_height = other.m_height;
        this->m_frameRates = other.m_frameRates;
    }

    return *this;
}

AkVCam::VideoFormat::~VideoFormat()
{

}

AkVCam::FourCC AkVCam::VideoFormat::fourcc() const
{
    return this->m_fourcc;
}

AkVCam::FourCC &AkVCam::VideoFormat::fourcc()
{
    return this->m_fourcc;
}

int AkVCam::VideoFormat::width() const
{
    return this->m_width;
}

int &AkVCam::VideoFormat::width()
{
    return this->m_width;
}

int AkVCam::VideoFormat::height() const
{
    return this->m_height;
}

int &AkVCam::VideoFormat::height()
{
    return this->m_height;
}

std::vector<double> AkVCam::VideoFormat::frameRates() const
{
    return this->m_frameRates;
}

std::vector<double> &AkVCam::VideoFormat::frameRates()
{
    return this->m_frameRates;
}

std::vector<std::pair<double, double>> AkVCam::VideoFormat::frameRateRanges() const
{
    std::vector<std::pair<double, double>> ranges;

    if (!this->m_frameRates.empty()) {
        auto min = *std::min_element(this->m_frameRates.begin(),
                                     this->m_frameRates.end());
        auto max = *std::max_element(this->m_frameRates.begin(),
                                     this->m_frameRates.end());
        ranges.push_back({min, max});
    }

    return ranges;
}

double AkVCam::VideoFormat::minimumFrameRate() const
{
    if (this->m_frameRates.empty())
        return 0.0;

    return *std::min_element(this->m_frameRates.begin(),
                             this->m_frameRates.end());
}
