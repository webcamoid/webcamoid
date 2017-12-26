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
#include "../cstream/cstream.h"
#include "../resources/rcloader.h"

namespace AkVCam
{
    // Little Endian format
    struct RGB32
    {
        uint8_t x;
        uint8_t b;
        uint8_t g;
        uint8_t r;
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
        uint8_t x;
        uint8_t r;
        uint8_t g;
        uint8_t b;
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
        uint8_t v0;
        uint8_t y0;
        uint8_t u0;
        uint8_t y1;
    };

    struct YUY2
    {
        uint8_t y0;
        uint8_t v0;
        uint8_t y1;
        uint8_t u0;
    };

    struct UV
    {
        uint8_t v;
        uint8_t u;
    };

    struct VU
    {
        uint8_t u;
        uint8_t v;
    };

    typedef size_t (*VideoConvertFuntion)(void *dst, const void *src, int width, int height);

    struct VideoConvert
    {
        FourCC from;
        FourCC to;
        VideoConvertFuntion convert;
    };

    class VideoFramePrivate
    {
        public:
            VideoFormat m_format;
            std::shared_ptr<uint8_t> m_data;
            size_t m_dataSize;
            std::vector<VideoConvert> m_convert;

            VideoFramePrivate():
                m_dataSize(0)
            {
                this->m_convert = {
                    {PixelFormatRGB24, PixelFormatRGB32, rgb24_to_rgb32},
                    {PixelFormatRGB24, PixelFormatRGB16, rgb24_to_rgb16},
                    {PixelFormatRGB24, PixelFormatRGB15, rgb24_to_rgb15},
                    {PixelFormatRGB24, PixelFormatBGR32, rgb24_to_bgr32},
                    {PixelFormatRGB24, PixelFormatBGR24, rgb24_to_bgr24},
                    {PixelFormatRGB24, PixelFormatBGR15, rgb24_to_bgr16},
                    {PixelFormatRGB24, PixelFormatBGR16, rgb24_to_bgr15},
                    {PixelFormatRGB24, PixelFormatUYVY , rgb24_to_uyvy },
                    {PixelFormatRGB24, PixelFormatYUY2 , rgb24_to_yuy2 },
                    {PixelFormatRGB24, PixelFormatNV12 , rgb24_to_nv12 },
                    {PixelFormatRGB24, PixelFormatNV21 , rgb24_to_nv21 }
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
            inline uint8_t extrapolateComponent(uint8_t min, uint8_t max,
                                               int kNum, int kDen) const;
            inline RGB24 extrapolateColor(const RGB24 &colorMin,
                                          const RGB24 &colorMax,
                                          int kNum,
                                          int kDen) const;
            inline RGB24 extrapolateColor(int xMin, int xMax,
                                          int kNumX, int kDenX,
                                          int yMin, int yMax,
                                          int kNumY, int kDenY) const;
    };
}

AkVCam::VideoFrame::VideoFrame()
{
    this->d = new VideoFramePrivate;
}

AkVCam::VideoFrame::VideoFrame(const std::string &bmpResource)
{
    this->d = new VideoFramePrivate;

    auto bitmapStream = RcLoader::load(bmpResource);

    if (!bitmapStream.data())
        return;

    if (bitmapStream.read<char>() != 'B'
        || bitmapStream.read<char>() != 'M')
        return;

    bitmapStream.seek(8);
    auto pixelsOffset = bitmapStream.read<uint32_t>();
    bitmapStream.seek(4);
    auto width = int(bitmapStream.read<uint32_t>());
    auto height = int(bitmapStream.read<uint32_t>());
    bitmapStream.seek(2);
    auto depth = bitmapStream.read<uint16_t>();
    bitmapStream.seek(int(pixelsOffset), CStreamRead::SeekSet);

    size_t dataSize = size_t(sizeof(RGB24) * width * height);

    if (!dataSize)
        return;

    auto data = std::shared_ptr<uint8_t>(new uint8_t[dataSize]);

    switch (depth) {
        case 24:
            for (int y = 0; y < height; y++) {
                auto line = reinterpret_cast<RGB24 *>(data.get()) + width * (height - y - 1);

                for (int x = 0; x < width; x++) {
                    line[x].r = bitmapStream.read<uint8_t>();
                    line[x].g = bitmapStream.read<uint8_t>();
                    line[x].b = bitmapStream.read<uint8_t>();
                }
            }

            this->d->m_format = {PixelFormatRGB24, width, height};
            this->d->m_data = data;
            this->d->m_dataSize = dataSize;

            break;

        case 32:
            for (int y = 0; y < height; y++) {
                auto line = reinterpret_cast<RGB24 *>(data.get()) + width * (height - y - 1);

                for (int x = 0; x < width; x++) {
                    bitmapStream.seek<uint8_t>();
                    line[x].r = bitmapStream.read<uint8_t>();
                    line[x].g = bitmapStream.read<uint8_t>();
                    line[x].b = bitmapStream.read<uint8_t>();
                }
            }

            this->d->m_format = {PixelFormatRGB24, width, height};
            this->d->m_data = data;
            this->d->m_dataSize = dataSize;

            break;

        default:
            break;
    }
}

AkVCam::VideoFrame::VideoFrame(const AkVCam::VideoFormat &format,
                               const std::shared_ptr<uint8_t> &data,
                               size_t dataSize)
{
    this->d = new VideoFramePrivate;
    this->d->m_format = format;
    this->d->m_data = data;
    this->d->m_dataSize = dataSize;
}

AkVCam::VideoFrame::VideoFrame(const AkVCam::VideoFormat &format,
                               const uint8_t *data,
                               size_t dataSize)
{
    this->d = new VideoFramePrivate;
    this->d->m_format = format;
    this->d->m_data = std::shared_ptr<uint8_t>(new uint8_t[dataSize]);
    memcpy(this->d->m_data.get(), data, dataSize);
    this->d->m_dataSize = dataSize;
}

AkVCam::VideoFrame::VideoFrame(const AkVCam::VideoFrame &other)
{
    this->d = new VideoFramePrivate;
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
                                              Scaling mode,
                                              bool keepAspect) const
{
    if (this->d->m_format.width() == width
        && this->d->m_format.height() == height)
        return *this;

    if (this->d->m_format.fourcc() != PixelFormatRGB24)
        return {};

    auto format = this->d->m_format;
    format.width() = width;
    format.height() = height;
    size_t dataSize = sizeof(RGB24) * size_t(width * height);
    auto data = std::shared_ptr<uint8_t>(new uint8_t[dataSize]);
    memset(data.get(), 0, dataSize);
    auto dataSrc = reinterpret_cast<RGB24 *>(this->d->m_data.get());
    auto dataDst = reinterpret_cast<RGB24 *>(data.get());

    switch (mode) {
        case ScalingFast:
            for (int y = 0; y < height; y++) {
                auto srcY = (this->d->m_format.height() - 1) * y / (height - 1);
                auto srcLine = dataSrc + srcY * this->d->m_format.width();
                auto dstLine = dataDst + y * width;

                for (int x = 0; x < width; x++) {
                    auto srcX = (this->d->m_format.width() - 1) * x / (width - 1);
                    dstLine[x] = srcLine[srcX];
                }
            }

            break;

        case ScalingLinear: {
            auto extrapolateX =
                    this->d->m_format.width() < width?
                        &VideoFramePrivate::extrapolateUp:
                        &VideoFramePrivate::extrapolateDown;
            auto extrapolateY =
                    this->d->m_format.height() < height?
                        &VideoFramePrivate::extrapolateUp:
                        &VideoFramePrivate::extrapolateDown;

            for (int y = 0; y < height; y++) {
                auto dstLine = dataDst + y * width;
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

                    dstLine[x] =
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

bool AkVCam::VideoFrame::canConvert(FourCC input, FourCC output) const
{
    for (auto &convert: this->d->m_convert)
        if (convert.from == input
            && convert.to == output) {
            return true;
        }

    return false;
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

uint8_t AkVCam::VideoFramePrivate::rgb_y(int r, int g, int b)
{
    return uint8_t(((66 * r + 129 * g + 25 * b + 128) >> 8) + 16);
}

uint8_t AkVCam::VideoFramePrivate::rgb_u(int r, int g, int b)
{
    return uint8_t(((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128);
}

uint8_t AkVCam::VideoFramePrivate::rgb_v(int r, int g, int b)
{
    return uint8_t(((112 * r - 94 * g - 18 * b + 128) >> 8) + 128);
}

uint8_t AkVCam::VideoFramePrivate::yuv_r(int y, int u, int v)
{
    UNUSED(u)
    int r = (298 * (y - 16) + 409 * (v - 128) + 128) >> 8;

    return uint8_t(bound(0, r, 255));
}

uint8_t AkVCam::VideoFramePrivate::yuv_g(int y, int u, int v)
{
    int g = (298 * (y - 16) - 100 * (u - 128) - 208 * (v - 128) + 128) >> 8;

    return uint8_t(bound(0, g, 255));
}

uint8_t AkVCam::VideoFramePrivate::yuv_b(int y, int u, int v)
{
    UNUSED(v)
    int b = (298 * (y - 16) + 516 * (u - 128) + 128) >> 8;

    return uint8_t(bound(0, b, 255));
}

size_t AkVCam::VideoFramePrivate::rgb24_to_rgb32(void *dst, const void *src, int width, int height)
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

size_t AkVCam::VideoFramePrivate::rgb24_to_rgb16(void *dst, const void *src, int width, int height)
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

size_t AkVCam::VideoFramePrivate::rgb24_to_rgb15(void *dst, const void *src, int width, int height)
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

size_t AkVCam::VideoFramePrivate::rgb24_to_bgr32(void *dst, const void *src, int width, int height)
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

size_t AkVCam::VideoFramePrivate::rgb24_to_bgr24(void *dst, const void *src, int width, int height)
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

size_t AkVCam::VideoFramePrivate::rgb24_to_bgr16(void *dst, const void *src, int width, int height)
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

size_t AkVCam::VideoFramePrivate::rgb24_to_bgr15(void *dst, const void *src, int width, int height)
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

size_t AkVCam::VideoFramePrivate::rgb24_to_uyvy(void *dst, const void *src, int width, int height)
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

size_t AkVCam::VideoFramePrivate::rgb24_to_yuy2(void *dst, const void *src, int width, int height)
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

size_t AkVCam::VideoFramePrivate::rgb24_to_nv12(void *dst, const void *src, int width, int height)
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

size_t AkVCam::VideoFramePrivate::rgb24_to_nv21(void *dst, const void *src, int width, int height)
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

void AkVCam::VideoFramePrivate::extrapolateUp(int dstCoord,
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

void AkVCam::VideoFramePrivate::extrapolateDown(int dstCoord,
                                                 int srcLength, int dstLength,
                                                 int *srcCoordMin, int *srcCoordMax,
                                                 int *kNum, int *kDen)
{
    int srcLength_1 = srcLength - 1;
    int dstLength_1 = dstLength - 1;
    *srcCoordMin = srcLength_1 * dstCoord / dstLength_1;
    *srcCoordMax = *srcCoordMin;
    *kNum = 0;
    *kDen = 1;
}

uint8_t AkVCam::VideoFramePrivate::extrapolateComponent(uint8_t min, uint8_t max,
                                                         int kNum, int kDen) const
{
    return uint8_t((kNum * (max - min) + kDen * min) / kDen);
}

AkVCam::RGB24 AkVCam::VideoFramePrivate::extrapolateColor(const RGB24 &colorMin,
                                                           const RGB24 &colorMax,
                                                           int kNum,
                                                           int kDen) const
{
    return RGB24 {
        extrapolateComponent(colorMin.b, colorMax.b, kNum, kDen),
        extrapolateComponent(colorMin.g, colorMax.g, kNum, kDen),
        extrapolateComponent(colorMin.r, colorMax.r, kNum, kDen)
    };
}

AkVCam::RGB24 AkVCam::VideoFramePrivate::extrapolateColor(int xMin, int xMax,
                                                           int kNumX, int kDenX,
                                                           int yMin, int yMax,
                                                           int kNumY, int kDenY) const
{
    auto minLine = reinterpret_cast<RGB24 *>(this->m_data.get())
                 + yMin * this->m_format.width();
    auto maxLine = reinterpret_cast<RGB24 *>(this->m_data.get())
                 + yMax * this->m_format.width();
    auto colorMin = extrapolateColor(minLine[xMin], minLine[xMax], kNumX, kDenX);
    auto colorMax = extrapolateColor(maxLine[xMin], maxLine[xMax], kNumX, kDenX);

    return extrapolateColor(colorMin, colorMax, kNumY, kDenY);
}
