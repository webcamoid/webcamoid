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
#include <algorithm>

#include "videoframe.h"
#include "videoformat.h"
#include "../utils.h"

namespace AkVCam
{
    // Little Endian format
    struct RGB32
    {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t x;
    };

    struct RGB24
    {
        uint8_t b;
        uint8_t g;
        uint8_t r;
    };

    struct RGB16
    {
        uint16_t b: 5;
        uint16_t g: 6;
        uint16_t r: 5;
    };

    struct RGB15
    {
        uint16_t b: 5;
        uint16_t g: 5;
        uint16_t r: 5;
        uint16_t x: 1;
    };

    struct BGR32
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t x;
    };

    struct BGR24
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    struct BGR16
    {
        uint16_t r: 5;
        uint16_t g: 6;
        uint16_t b: 5;
    };

    struct BGR15
    {
        uint16_t r: 5;
        uint16_t g: 5;
        uint16_t b: 5;
        uint16_t x: 1;
    };

    struct UYVY
    {
        uint8_t y1;
        uint8_t v0;
        uint8_t y0;
        uint8_t u0;
    };

    struct YUY2
    {
        uint8_t y0;
        uint8_t u0;
        uint8_t y1;
        uint8_t v0;
    };

    struct UV
    {
        uint8_t u;
        uint8_t v;
    };

    struct VU
    {
        uint8_t v;
        uint8_t u;
    };

    typedef size_t (*VideoConvertFuntion)(void *dst, const void *src, int width, int height);

    struct VideoConvert
    {
        FourCC from;
        FourCC to;
        VideoConvertFuntion convert;
    };

    class VideoFormatPrivate
    {
        public:
            VideoFormat m_format;
            std::shared_ptr<uint8_t> m_data;
            size_t m_dataSize;
            std::vector<VideoConvert> m_convert;

            VideoFormatPrivate():
                m_dataSize(0)
            {
                this->m_convert = {
                    {MKFOURCC('R', 'G', 'B',  24), MKFOURCC('R', 'G', 'B',  32), rgb24_to_rgb32},
                    {MKFOURCC('R', 'G', 'B',  24), MKFOURCC('R', 'G', 'B',  16), rgb24_to_rgb16},
                    {MKFOURCC('R', 'G', 'B',  24), MKFOURCC('R', 'G', 'B',  15), rgb24_to_rgb15},
                    {MKFOURCC('R', 'G', 'B',  24), MKFOURCC('B', 'G', 'R',  32), rgb24_to_bgr32},
                    {MKFOURCC('R', 'G', 'B',  24), MKFOURCC('B', 'G', 'R',  24), rgb24_to_bgr24},
                    {MKFOURCC('R', 'G', 'B',  24), MKFOURCC('B', 'G', 'R',  15), rgb24_to_bgr16},
                    {MKFOURCC('R', 'G', 'B',  24), MKFOURCC('B', 'G', 'R',  16), rgb24_to_bgr15},
                    {MKFOURCC('R', 'G', 'B',  24), MKFOURCC('U', 'Y', 'V', 'Y'), rgb24_to_uyvy },
                    {MKFOURCC('R', 'G', 'B',  24), MKFOURCC('Y', 'U', 'Y', '2'), rgb24_to_yuy2 },
                    {MKFOURCC('R', 'G', 'B',  24), MKFOURCC('N', 'V', '1', '2'), rgb24_to_nv12 },
                    {MKFOURCC('R', 'G', 'B',  24), MKFOURCC('N', 'V', '2', '1'), rgb24_to_nv21 }
                };
            }

            template<typename T>
            inline static T bound(T min, T value, T max)
            {
                return value < min? min: value > max? max: value;
            }

            inline static uint8_t rgb_y(int r, int g, int b);
            inline static uint8_t rgb_u(int r, int g, int b);
            inline static uint8_t rgb_v(int r, int g, int b);
            inline static uint8_t yuv_r(int y, int u, int v);
            inline static uint8_t yuv_g(int y, int u, int v);
            inline static uint8_t yuv_b(int y, int u, int v);

            // RGB formats
            inline static size_t rgb24_to_rgb32(void *dst, const void *src, int width, int height);
            inline static size_t rgb24_to_rgb16(void *dst, const void *src, int width, int height);
            inline static size_t rgb24_to_rgb15(void *dst, const void *src, int width, int height);

            // BGR formats
            inline static size_t rgb24_to_bgr32(void *dst, const void *src, int width, int height);
            inline static size_t rgb24_to_bgr24(void *dst, const void *src, int width, int height);
            inline static size_t rgb24_to_bgr16(void *dst, const void *src, int width, int height);
            inline static size_t rgb24_to_bgr15(void *dst, const void *src, int width, int height);

            // Luminance+Chrominance formats
            inline static size_t rgb24_to_uyvy(void *dst, const void *src, int width, int height);
            inline static size_t rgb24_to_yuy2(void *dst, const void *src, int width, int height);

            // two planes -- one Y, one Cr + Cb interleaved
            inline static size_t rgb24_to_nv12(void *dst, const void *src, int width, int height);
            inline static size_t rgb24_to_nv21(void *dst, const void *src, int width, int height);

            inline static void extrapolateUp(int dstCoord,
                                             int srcLength, int dstLength,
                                             int *srcCoordMin, int *srcCoordMax,
                                             int *kNum, int *kDen);
            inline static void extrapolateDown(int dstCoord,
                                               int srcLength, int dstLength,
                                               int *srcCoordMin, int *srcCoordMax,
                                               int *kNum, int *kDen);
            inline RGB24 extrapolateColor(int xMin, int xMax,
                                          int kNumX, int kDenX,
                                          int yMin, int yMax,
                                          int kNumY, int kDenY) const;
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

AkVCam::VideoFrame AkVCam::VideoFrame::scaled(int width,
                                              int height,
                                              Scaling mode) const
{
    if (this->d->m_format.width() == width
        && this->d->m_format.height() == height)
        return *this;

    if (this->d->m_format.fourcc() != MKFOURCC('R', 'G', 'B',  24))
        return {};

    auto format = this->d->m_format;
    format.width() = width;
    format.height() = height;
    size_t dataSize = size_t(3 * width * height);
    auto data = std::shared_ptr<uint8_t>(new uint8_t[dataSize]);

    switch (mode) {
        case ScalingFast:
            for (int y = 0; y < height; y++) {
                auto srcY = (this->d->m_format.height() - 1) * y / (height - 1);
                auto srcLine = reinterpret_cast<RGB24 *>(this->d->m_data.get())
                             + srcY * this->d->m_format.width();
                auto destLine = reinterpret_cast<RGB24 *>(data.get())
                              + y * width;

                for (int x = 0; x < width; x++) {
                    auto srcX = (this->d->m_format.width() - 1) * x / (width - 1);
                    destLine[x] = srcLine[srcX];
                }
            }

            break;

        case ScalingLinear: {
            auto extrapolateX =
                    this->d->m_format.width() < width?
                        &VideoFormatPrivate::extrapolateUp:
                        &VideoFormatPrivate::extrapolateDown;
            auto extrapolateY =
                    this->d->m_format.height() < height?
                        &VideoFormatPrivate::extrapolateUp:
                        &VideoFormatPrivate::extrapolateDown;

            for (int y = 0; y < height; y++) {
                auto destLine = reinterpret_cast<RGB24 *>(data.get())
                              + y * width;
                int yMin;
                int yMax;
                int kNumY;
                int kDenY;
                extrapolateY(y,
                             this->d->m_format.height(), height,
                             &yMin, &yMax,
                             &kNumY, &kDenY);

                for (int x = 0; x < width; x++) {
                    int xMin;
                    int xMax;
                    int kNumX;
                    int kDenX;
                    extrapolateX(x,
                                 this->d->m_format.width(), width,
                                 &xMin, &xMax,
                                 &kNumX, &kDenX);

                    destLine[x] =
                            this->d->extrapolateColor(xMin, xMax,
                                                      kNumX, kDenX,
                                                      yMin, yMax,
                                                      kNumY, kDenY);
                }
            }

            break;
        }
    }

    return VideoFrame(format, data, dataSize);
}

std::vector<AkVCam::FourCC> &AkVCam::VideoFrame::inputFormats()
{
    static std::vector<AkVCam::FourCC> inputFormats {
        MKFOURCC('R', 'G', 'B',  24),
    };

    return inputFormats;
}

std::vector<AkVCam::FourCC> &AkVCam::VideoFrame::outputFormats()
{
    static std::vector<AkVCam::FourCC> outputFormats {
        // RGB formats
        MKFOURCC('R', 'G', 'B',  32),
        MKFOURCC('R', 'G', 'B',  24),
        MKFOURCC('R', 'G', 'B',  16),
        MKFOURCC('R', 'G', 'B',  15),

        // BGR formats
        MKFOURCC('B', 'G', 'R',  32),
        MKFOURCC('B', 'G', 'R',  24),
        MKFOURCC('B', 'G', 'R',  15),
        MKFOURCC('B', 'G', 'R',  16),

        // Luminance+Chrominance formats
        MKFOURCC('U', 'Y', 'V', 'Y'),
        MKFOURCC('Y', 'U', 'Y', '2'),

        // two planes -- one Y, one Cr + Cb interleaved
        MKFOURCC('N', 'V', '1', '2'),
        MKFOURCC('N', 'V', '2', '1')
    };

    return outputFormats;
}

AkVCam::VideoFrame AkVCam::VideoFrame::convert(AkVCam::FourCC fourcc) const
{
    if (this->d->m_format.fourcc() == fourcc)
        return *this;

    VideoConvert *converter = nullptr;

    for (auto &convert: this->d->m_convert)
        if (convert.from == this->d->m_format.fourcc()
            && convert.to == fourcc) {
            converter = &convert;

            break;
        }

    if (!converter)
        return {};

    auto format = this->d->m_format;
    format.fourcc() = fourcc;
    auto dataSize =
            converter->convert(nullptr,
                               nullptr,
                               this->d->m_format.width(),
                               this->d->m_format.height());
    auto data = std::shared_ptr<uint8_t>(new uint8_t[dataSize]);
    converter->convert(data.get(),
                       this->d->m_data.get(),
                       this->d->m_format.width(),
                       this->d->m_format.height());

    return VideoFrame(format, data, dataSize);
}

uint8_t AkVCam::VideoFormatPrivate::rgb_y(int r, int g, int b)
{
    return uint8_t(((66 * r + 129 * g + 25 * b + 128) >> 8) + 16);
}

uint8_t AkVCam::VideoFormatPrivate::rgb_u(int r, int g, int b)
{
    return uint8_t(((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128);
}

uint8_t AkVCam::VideoFormatPrivate::rgb_v(int r, int g, int b)
{
    return uint8_t(((112 * r - 94 * g - 18 * b + 128) >> 8) + 128);
}

uint8_t AkVCam::VideoFormatPrivate::yuv_r(int y, int u, int v)
{
    UNUSED(u)
    int r = (298 * (y - 16) + 409 * (v - 128) + 128) >> 8;

    return uint8_t(bound(0, r, 255));
}

uint8_t AkVCam::VideoFormatPrivate::yuv_g(int y, int u, int v)
{
    int g = (298 * (y - 16) - 100 * (u - 128) - 208 * (v - 128) + 128) >> 8;

    return uint8_t(bound(0, g, 255));
}

uint8_t AkVCam::VideoFormatPrivate::yuv_b(int y, int u, int v)
{
    UNUSED(v)
    int b = (298 * (y - 16) + 516 * (u - 128) + 128) >> 8;

    return uint8_t(bound(0, b, 255));
}

size_t AkVCam::VideoFormatPrivate::rgb24_to_rgb32(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 32 * len / 8;

    if (!dst || !src)
        return osize;

    auto _src = reinterpret_cast<const RGB24 *>(src);
    auto _dst = reinterpret_cast<RGB32 *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].x = 255;
        _dst[i].r = _src[i].r;
        _dst[i].g = _src[i].g;
        _dst[i].b = _src[i].b;
    }

    return osize;
}

size_t AkVCam::VideoFormatPrivate::rgb24_to_rgb16(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 16 * len / 8;

    if (!dst || !src)
        return osize;

    auto _src = reinterpret_cast<const RGB24 *>(src);
    auto _dst = reinterpret_cast<RGB16 *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].r = _src[i].r >> 3;
        _dst[i].g = _src[i].g >> 2;
        _dst[i].b = _src[i].b >> 3;
    }

    return osize;
}

size_t AkVCam::VideoFormatPrivate::rgb24_to_rgb15(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 16 * len / 8;

    if (!dst || !src)
        return osize;

    auto _src = reinterpret_cast<const RGB24 *>(src);
    auto _dst = reinterpret_cast<RGB15 *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].x = 1;
        _dst[i].r = _src[i].r >> 3;
        _dst[i].g = _src[i].g >> 3;
        _dst[i].b = _src[i].b >> 3;
    }

    return osize;
}

size_t AkVCam::VideoFormatPrivate::rgb24_to_bgr32(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 32 * len / 8;

    if (!dst || !src)
        return osize;

    auto _src = reinterpret_cast<const RGB24 *>(src);
    auto _dst = reinterpret_cast<BGR32 *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].x = 255;
        _dst[i].r = _src[i].r;
        _dst[i].g = _src[i].g;
        _dst[i].b = _src[i].b;
    }

    return osize;
}

size_t AkVCam::VideoFormatPrivate::rgb24_to_bgr24(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 24 * len / 8;

    if (!dst || !src)
        return osize;

    auto _src = reinterpret_cast<const RGB24 *>(src);
    auto _dst = reinterpret_cast<BGR24 *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].r = _src[i].r;
        _dst[i].g = _src[i].g;
        _dst[i].b = _src[i].b;
    }

    return osize;
}

size_t AkVCam::VideoFormatPrivate::rgb24_to_bgr16(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 16 * len / 8;

    if (!dst || !src)
        return osize;

    auto _src = reinterpret_cast<const RGB24 *>(src);
    auto _dst = reinterpret_cast<BGR16 *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].r = _src[i].r >> 3;
        _dst[i].g = _src[i].g >> 2;
        _dst[i].b = _src[i].b >> 3;
    }

    return osize;
}

size_t AkVCam::VideoFormatPrivate::rgb24_to_bgr15(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 16 * len / 8;

    if (!dst || !src)
        return osize;

    auto _src = reinterpret_cast<const RGB24 *>(src);
    auto _dst = reinterpret_cast<BGR15 *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].x = 1;
        _dst[i].r = _src[i].r >> 3;
        _dst[i].g = _src[i].g >> 3;
        _dst[i].b = _src[i].b >> 3;
    }

    return osize;
}

size_t AkVCam::VideoFormatPrivate::rgb24_to_uyvy(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 16 * len / 8;

    if (!dst || !src)
        return osize;

    auto _src = reinterpret_cast<const RGB24 *>(src);
    auto _dst = reinterpret_cast<UYVY *>(dst);

    size_t olen = len / 2;

    for (size_t i = 0; i < olen; i++) {
        size_t j = 2 * i;

        uint8_t r0 = _src[j].r;
        uint8_t g0 = _src[j].g;
        uint8_t b0 = _src[j].b;

        j++;

        uint8_t r1 = _src[j].r;
        uint8_t g1 = _src[j].g;
        uint8_t b1 = _src[j].b;

        _dst[i].u0 = rgb_u(r0, g0, b0);
        _dst[i].y0 = rgb_y(r0, g0, b0);
        _dst[i].v0 = rgb_v(r0, g0, b0);
        _dst[i].y1 = rgb_y(r1, g1, b1);
    }

    return osize;
}

size_t AkVCam::VideoFormatPrivate::rgb24_to_yuy2(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 16 * len / 8;

    if (!dst || !src)
        return osize;

    auto _src = reinterpret_cast<const RGB24 *>(src);
    auto _dst = reinterpret_cast<YUY2 *>(dst);

    size_t olen = len / 2;

    for (size_t i = 0; i < olen; i++) {
        size_t j = 2 * i;

        uint8_t r0 = _src[j].r;
        uint8_t g0 = _src[j].g;
        uint8_t b0 = _src[j].b;

        j++;

        uint8_t r1 = _src[j].r;
        uint8_t g1 = _src[j].g;
        uint8_t b1 = _src[j].b;

        _dst[i].y0 = rgb_y(r0, g0, b0);
        _dst[i].u0 = rgb_u(r0, g0, b0);
        _dst[i].y1 = rgb_y(r1, g1, b1);
        _dst[i].v0 = rgb_v(r0, g0, b0);
    }

    return osize;
}

size_t AkVCam::VideoFormatPrivate::rgb24_to_nv12(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 12 * len / 8;

    if (!dst || !src)
        return osize;

    auto _src = reinterpret_cast<const RGB24 *>(src);
    auto dst_y = reinterpret_cast<uint8_t *>(dst);
    auto dst_vu = reinterpret_cast<VU *>(dst_y + len);

    // Write Y plane.
    for (size_t i = 0; i < len; i++) {
        uint8_t r = _src[i].r;
        uint8_t g = _src[i].g;
        uint8_t b = _src[i].b;

        dst_y[i] = rgb_y(r, g, b);
    }

    // Write VU planes.
    int _width = width >> 1;
    int _height = height >> 1;

    for (int y = 0; y < _height; y++) {
        auto line = _src + 2 * y * width;
        auto line_vu = dst_vu + y * _width;

        for (int x = 0; x < _width; x++) {
            int j = 2 * x;

            uint8_t r = line[j].r;
            uint8_t g = line[j].g;
            uint8_t b = line[j].b;

            line_vu[x].v = rgb_v(r, g, b);
            line_vu[x].u = rgb_u(r, g, b);
        }
    }

    return osize;
}

size_t AkVCam::VideoFormatPrivate::rgb24_to_nv21(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 12 * len / 8;

    if (!dst || !src)
        return osize;

    auto _src = reinterpret_cast<const RGB24 *>(src);
    auto dst_y = reinterpret_cast<uint8_t *>(dst);
    auto dst_uv = reinterpret_cast<UV *>(dst_y + len);

    // Write Y plane.
    for (size_t i = 0; i < len; i++) {
        uint8_t r = _src[i].r;
        uint8_t g = _src[i].g;
        uint8_t b = _src[i].b;

        dst_y[i] = rgb_y(r, g, b);
    }

    // Write VU planes.
    int _width = width >> 1;
    int _height = height >> 1;

    for (int y = 0; y < _height; y++) {
        auto line = _src + 2 * y * width;
        auto line_uv = dst_uv + y * _width;

        for (int x = 0; x < _width; x++) {
            int j = 2 * x;

            uint8_t r = line[j].r;
            uint8_t g = line[j].g;
            uint8_t b = line[j].b;

            line_uv[x].u = rgb_u(r, g, b);
            line_uv[x].v = rgb_v(r, g, b);
        }
    }

    return osize;
}

void AkVCam::VideoFormatPrivate::extrapolateUp(int dstCoord,
                                               int srcLength, int dstLength,
                                               int *srcCoordMin, int *srcCoordMax,
                                               int *kNum, int *kDen)
{
    int srcLength_1 = srcLength - 2;
    int dstLength_1 = dstLength - 2;
    *srcCoordMin = srcLength_1 * dstCoord / dstLength_1;
    *srcCoordMax = *srcCoordMin + 1;
    int dstCoordMin = dstLength_1 * *srcCoordMin / srcLength_1;
    int dstCoordMax = dstLength_1 * *srcCoordMax / srcLength_1;
    *kNum = dstCoord - dstCoordMin;
    *kDen = dstCoordMax - dstCoordMin;
}

void AkVCam::VideoFormatPrivate::extrapolateDown(int dstCoord,
                                                 int srcLength, int dstLength,
                                                 int *srcCoordMin, int *srcCoordMax,
                                                 int *kNum, int *kDen)
{
    int srcLength_1 = srcLength - 1;
    int dstLength_1 = dstLength - 1;
    *srcCoordMin = srcLength_1 * dstCoord / dstLength_1;
    *srcCoordMax = *srcCoordMin;
    *kNum = 1;
    *kDen = 2;
}

AkVCam::RGB24 AkVCam::VideoFormatPrivate::extrapolateColor(int xMin, int xMax,
                                                           int kNumX, int kDenX,
                                                           int yMin, int yMax,
                                                           int kNumY, int kDenY) const
{
    auto minLine = reinterpret_cast<RGB24 *>(this->m_data.get())
                 + yMin * this->m_format.width();
    auto maxLine = reinterpret_cast<RGB24 *>(this->m_data.get())
                 + yMax * this->m_format.width();

    auto &color0 = minLine[xMin];
    auto &colorX = minLine[xMax];
    auto &colorY = maxLine[xMin];

    int diffXR = colorX.r - color0.r;
    int diffXG = colorX.g - color0.g;
    int diffXB = colorX.b - color0.b;

    int diffYR = colorY.r - color0.r;
    int diffYG = colorY.g - color0.g;
    int diffYB = colorY.b - color0.b;

    auto rx = uint8_t((kNumX * diffXR + kDenX * color0.r) / kDenX);
    auto gx = uint8_t((kNumX * diffXG + kDenX * color0.g) / kDenX);
    auto bx = uint8_t((kNumX * diffXB + kDenX * color0.b) / kDenX);

    auto ry = uint8_t((kNumY * diffYR + kDenY * color0.r) / kDenY);
    auto gy = uint8_t((kNumY * diffYG + kDenY * color0.g) / kDenY);
    auto by = uint8_t((kNumY * diffYB + kDenY * color0.b) / kDenY);

    return {
        uint8_t(bound(0, rx + ry, 255)),
        uint8_t(bound(0, gx + gy, 255)),
        uint8_t(bound(0, bx + by, 255))
    };
}
