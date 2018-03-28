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

#include <map>
#include <algorithm>

#include "videoformat.h"

namespace AkVCam
{
    class VideoFormatPrivate
    {
        public:
            FourCC m_fourcc;
            int m_width;
            int m_height;
            std::vector<double> m_frameRates;

            VideoFormatPrivate():
                m_fourcc(0),
                m_width(0),
                m_height(0)
            {
            }

            VideoFormatPrivate(FourCC fourcc,
                               int width,
                               int height,
                               const std::vector<double> &frameRates):
                m_fourcc(fourcc),
                m_width(width),
                m_height(height),
                m_frameRates(frameRates)
            {
            }
    };

    class VideoFormatGlobals
    {
        public:
            PixelFormat format;
            size_t bpp;
            std::string str;

            inline static const std::vector<VideoFormatGlobals> &formats()
            {
                static const std::vector<VideoFormatGlobals> formats {
                    {PixelFormatRGB32, 32, "RGB32"},
                    {PixelFormatRGB24, 24, "RGB24"},
                    {PixelFormatRGB16, 16, "RGB16"},
                    {PixelFormatRGB15, 16, "RGB15"},
                    {PixelFormatBGR32, 32, "BGR32"},
                    {PixelFormatBGR24, 24, "BGR24"},
                    {PixelFormatBGR16, 16, "BGR16"},
                    {PixelFormatBGR15, 16, "BGR15"},
                    {PixelFormatUYVY , 16, "UYVY" },
                    {PixelFormatYUY2 , 16, "YUY2" },
                    {PixelFormatNV12 , 12, "NV12" },
                    {PixelFormatNV21 , 12, "NV21" }
                };

                return formats;
            }

            static inline const VideoFormatGlobals *byPixelFormat(PixelFormat pixelFormat)
            {
                for (auto &format: formats())
                    if (format.format == pixelFormat)
                        return &format;

                return nullptr;
            }

            static inline const VideoFormatGlobals *byStr(const std::string str)
            {
                for (auto &format: formats())
                    if (format.str == str)
                        return &format;

                return nullptr;
            }
    };
}

AkVCam::VideoFormat::VideoFormat()
{
    this->d = new VideoFormatPrivate;
}

AkVCam::VideoFormat::VideoFormat(FourCC fourcc,
                                 int width,
                                 int height,
                                 const std::vector<double> &frameRates)
{
    this->d = new VideoFormatPrivate(fourcc, width, height, frameRates);
}

AkVCam::VideoFormat::VideoFormat(const VideoFormat &other)
{
    this->d = new VideoFormatPrivate(other.d->m_fourcc,
                                     other.d->m_width,
                                     other.d->m_height,
                                     other.d->m_frameRates);
}

AkVCam::VideoFormat &AkVCam::VideoFormat::operator =(const VideoFormat &other)
{
    if (this != &other) {
        this->d->m_fourcc = other.d->m_fourcc;
        this->d->m_width = other.d->m_width;
        this->d->m_height = other.d->m_height;
        this->d->m_frameRates = other.d->m_frameRates;
    }

    return *this;
}

bool AkVCam::VideoFormat::operator ==(const AkVCam::VideoFormat &other) const
{
    return this->d->m_fourcc == other.d->m_fourcc
           && this->d->m_width == other.d->m_width
           && this->d->m_height == other.d->m_height
           && this->d->m_frameRates == other.d->m_frameRates;
}

bool AkVCam::VideoFormat::operator !=(const AkVCam::VideoFormat &other) const
{
    return this->d->m_fourcc != other.d->m_fourcc
           || this->d->m_width != other.d->m_width
           || this->d->m_height != other.d->m_height
           || this->d->m_frameRates != other.d->m_frameRates;
}

AkVCam::VideoFormat::operator bool() const
{
    return this->size() != 0;
}

AkVCam::VideoFormat::~VideoFormat()
{
    delete this->d;
}

AkVCam::FourCC AkVCam::VideoFormat::fourcc() const
{
    return this->d->m_fourcc;
}

AkVCam::FourCC &AkVCam::VideoFormat::fourcc()
{
    return this->d->m_fourcc;
}

int AkVCam::VideoFormat::width() const
{
    return this->d->m_width;
}

int &AkVCam::VideoFormat::width()
{
    return this->d->m_width;
}

int AkVCam::VideoFormat::height() const
{
    return this->d->m_height;
}

int &AkVCam::VideoFormat::height()
{
    return this->d->m_height;
}

std::vector<double> AkVCam::VideoFormat::frameRates() const
{
    return this->d->m_frameRates;
}

std::vector<double> &AkVCam::VideoFormat::frameRates()
{
    return this->d->m_frameRates;
}

std::vector<std::pair<double, double>> AkVCam::VideoFormat::frameRateRanges() const
{
    std::vector<std::pair<double, double>> ranges;

    if (!this->d->m_frameRates.empty()) {
        auto min = *std::min_element(this->d->m_frameRates.begin(),
                                     this->d->m_frameRates.end());
        auto max = *std::max_element(this->d->m_frameRates.begin(),
                                     this->d->m_frameRates.end());
        ranges.push_back({min, max});
    }

    return ranges;
}

double AkVCam::VideoFormat::minimumFrameRate() const
{
    if (this->d->m_frameRates.empty())
        return 0.0;

    return *std::min_element(this->d->m_frameRates.begin(),
                             this->d->m_frameRates.end());
}

size_t AkVCam::VideoFormat::bpp() const
{
    auto vf = VideoFormatGlobals::byPixelFormat(PixelFormat(this->d->m_fourcc));

    return vf? vf->bpp: 0;
}

size_t AkVCam::VideoFormat::size() const
{
    return size_t(this->d->m_width * this->d->m_height) * this->bpp() / 8;
}

void AkVCam::VideoFormat::clear()
{
    this->d->m_fourcc = 0;
    this->d->m_width = 0;
    this->d->m_height = 0;
    this->d->m_frameRates.clear();
}

void AkVCam::VideoFormat::roundNearest(int width, int height,
                                       int *owidth, int *oheight,
                                       int align)
{
    /* Explanation:
     *
     * When 'align' is a power of 2, the left most bit will be 1 (the pivot),
     * while all other bits be 0, if destination width is multiple of 'align'
     * all bits after pivot position will be 0, then we create a mask
     * substracting 1 to the align, so all bits after pivot position in the
     * mask will 1.
     * Then we negate all bits in the mask so all bits from pivot to the left
     * will be 1, and then we use that mask to get a width multiple of align.
     * This give us the lower (floor) width nearest to the original 'width' and
     * multiple of align. To get the rounded nearest value we add align / 2 to
     * 'width'.
     * This is the equivalent of:
     *
     * align * round(width / align)
     */
    *owidth = (width + (align >> 1)) & ~(align - 1);

    /* Find the nearest width:
     *
     * round(height * owidth / width)
     */
    *oheight = (2 * height * *owidth + width) / (2 * width);
}

AkVCam::FourCC AkVCam::VideoFormat::fourccFromString(const std::string &fourccStr)
{
    auto vf = VideoFormatGlobals::byStr(fourccStr);

    return vf? vf->format: 0;
}

std::string AkVCam::VideoFormat::stringFromFourcc(AkVCam::FourCC fourcc)
{
    auto vf = VideoFormatGlobals::byPixelFormat(PixelFormat(fourcc));

    return vf? vf->str: std::string();
}
