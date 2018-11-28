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

#include <map>
#include <algorithm>

#include "videoformat.h"
#include "../utils.h"

namespace AkVCam
{
    class VideoFormatPrivate
    {
        public:
            FourCC m_fourcc;
            int m_width;
            int m_height;
            std::vector<double> m_frameRates;

            VideoFormatPrivate();
            VideoFormatPrivate(FourCC fourcc,
                               int width,
                               int height,
                               const std::vector<double> &frameRates);
    };

    typedef size_t (*PlaneOffsetFunc)(size_t plane, size_t width, size_t height);
    typedef size_t (*ByplFunc)(size_t plane, size_t width);

    class VideoFormatGlobals
    {
        public:
            PixelFormat format;
            size_t bpp;
            size_t planes;
            PlaneOffsetFunc planeOffset;
            ByplFunc bypl;
            std::string str;

            inline static const std::vector<VideoFormatGlobals> &formats();
            static inline const VideoFormatGlobals *byPixelFormat(PixelFormat pixelFormat);
            static inline const VideoFormatGlobals *byStr(const std::string str);
            static size_t offsetNV(size_t plane, size_t width, size_t height);
            static size_t byplNV(size_t plane, size_t width);

            template<typename T>
            static inline T alignUp(const T &value, const T &align)
            {
                return (value + align - 1) & ~(align - 1);
            }

            template<typename T>
            static inline T align32(const T &value)
            {
                return alignUp<T>(value, 32);
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
    return this->isValid();
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

size_t AkVCam::VideoFormat::bypl(size_t plane) const
{
    auto vf = VideoFormatGlobals::byPixelFormat(PixelFormat(this->d->m_fourcc));

    if (!vf)
        return 0;

    if (vf->bypl)
        return vf->bypl(plane, size_t(this->d->m_width));

    return VideoFormatGlobals::align32(size_t(this->d->m_width) * vf->bpp) / 8;
}

size_t AkVCam::VideoFormat::size() const
{
    auto vf = VideoFormatGlobals::byPixelFormat(PixelFormat(this->d->m_fourcc));

    if (!vf)
        return 0;

    if (vf->planeOffset)
        return vf->planeOffset(vf->planes,
                               size_t(this->d->m_width),
                               size_t(this->d->m_height));

    return size_t(this->d->m_height)
           * VideoFormatGlobals::align32(size_t(this->d->m_width)
                                         * vf->bpp) / 8;
}

size_t AkVCam::VideoFormat::planes() const
{
    auto vf = VideoFormatGlobals::byPixelFormat(PixelFormat(this->d->m_fourcc));

    return vf? vf->planes: 0;
}

size_t AkVCam::VideoFormat::offset(size_t plane) const
{
    auto vf = VideoFormatGlobals::byPixelFormat(PixelFormat(this->d->m_fourcc));

    if (!vf)
        return 0;

    if (vf->planeOffset)
        return vf->planeOffset(plane,
                               size_t(this->d->m_width),
                               size_t(this->d->m_height));

    return 0;
}

size_t AkVCam::VideoFormat::planeSize(size_t plane) const
{
    return size_t(this->d->m_height) * this->bypl( plane);
}

bool AkVCam::VideoFormat::isValid() const
{
    if (this->size() <= 0)
        return false;

    if (this->d->m_frameRates.empty())
        return false;

    for (auto &fps: this->d->m_frameRates)
        if (fps < 1.0)
            return false;

    return true;
}

void AkVCam::VideoFormat::clear()
{
    this->d->m_fourcc = 0;
    this->d->m_width = 0;
    this->d->m_height = 0;
    this->d->m_frameRates.clear();
}

AkVCam::VideoFormat AkVCam::VideoFormat::nearest(const std::vector<VideoFormat> &formats) const
{
    VideoFormat nearestFormat;
    auto q = std::numeric_limits<uint64_t>::max();
    auto svf = VideoFormatGlobals::byPixelFormat(PixelFormat(this->d->m_fourcc));

    for (auto &format: formats) {
        auto vf = VideoFormatGlobals::byPixelFormat(PixelFormat(format.d->m_fourcc));
        uint64_t diffFourcc = format.d->m_fourcc == this->d->m_fourcc? 0: 1;
        auto diffWidth = format.d->m_width - this->d->m_width;
        auto diffHeight = format.d->m_height - this->d->m_height;
        auto diffBpp = vf->bpp - svf->bpp;
        auto diffPlanes = vf->planes - svf->planes;

        uint64_t k = diffFourcc
                   + uint64_t(diffWidth * diffWidth)
                   + uint64_t(diffHeight * diffHeight)
                   + diffBpp * diffBpp
                   + diffPlanes * diffPlanes;

        if (k < q) {
            nearestFormat = format;
            q = k;
        }
    }

    return nearestFormat;
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

std::wstring AkVCam::VideoFormat::wstringFromFourcc(AkVCam::FourCC fourcc)
{
    auto str = stringFromFourcc(fourcc);

    return std::wstring(str.begin(), str.end());
}

AkVCam::VideoFormatPrivate::VideoFormatPrivate():
    m_fourcc(0),
    m_width(0),
    m_height(0)
{
}

AkVCam::VideoFormatPrivate::VideoFormatPrivate(FourCC fourcc,
                                               int width,
                                               int height,
                                               const std::vector<double> &frameRates):
    m_fourcc(fourcc),
    m_width(width),
    m_height(height),
    m_frameRates(frameRates)
{
}

const std::vector<AkVCam::VideoFormatGlobals> &AkVCam::VideoFormatGlobals::formats()
{
    static const std::vector<VideoFormatGlobals> formats {
        {PixelFormatRGB32, 32, 1,  nullptr, nullptr, "RGB32"},
        {PixelFormatRGB24, 24, 1,  nullptr, nullptr, "RGB24"},
        {PixelFormatRGB16, 16, 1,  nullptr, nullptr, "RGB16"},
        {PixelFormatRGB15, 16, 1,  nullptr, nullptr, "RGB15"},
        {PixelFormatBGR32, 32, 1,  nullptr, nullptr, "BGR32"},
        {PixelFormatBGR24, 24, 1,  nullptr, nullptr, "BGR24"},
        {PixelFormatBGR16, 16, 1,  nullptr, nullptr, "BGR16"},
        {PixelFormatBGR15, 16, 1,  nullptr, nullptr, "BGR15"},
        {PixelFormatUYVY , 16, 1,  nullptr, nullptr,  "UYVY"},
        {PixelFormatYUY2 , 16, 1,  nullptr, nullptr,  "YUY2"},
        {PixelFormatNV12 , 12, 2, offsetNV,  byplNV,  "NV12"},
        {PixelFormatNV21 , 12, 2, offsetNV,  byplNV,  "NV21"}
    };

    return formats;
}

const AkVCam::VideoFormatGlobals *AkVCam::VideoFormatGlobals::byPixelFormat(PixelFormat pixelFormat)
{
    for (auto &format: formats())
        if (format.format == pixelFormat)
            return &format;

    return nullptr;
}

const AkVCam::VideoFormatGlobals *AkVCam::VideoFormatGlobals::byStr(const std::string str)
{
    for (auto &format: formats())
        if (format.str == str)
            return &format;

    return nullptr;
}

size_t AkVCam::VideoFormatGlobals::offsetNV(size_t plane, size_t width, size_t height)
{
    size_t offset[] = {
        0,
        align32(size_t(width)) * height,
        5 * align32(size_t(width)) * height / 4
    };

    return offset[plane];
}

size_t AkVCam::VideoFormatGlobals::byplNV(size_t plane, size_t width)
{
    UNUSED(plane)

    return align32(size_t(width));
}
