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

#include "colorconv.h"

#define UNUSED(x) (void)(x);

struct RGB3
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct RGB4
{
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct BGR3
{
    uint8_t b;
    uint8_t g;
    uint8_t r;
};

struct BGR4
{
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
};

struct RGBP
{
    uint16_t r: 5;
    uint16_t g: 6;
    uint16_t b: 5;
};

struct RGBO
{
    uint16_t r: 5;
    uint16_t g: 5;
    uint16_t b: 5;
    uint16_t a: 1;
};

struct UYVY
{
    uint8_t u0;
    uint8_t y0;
    uint8_t v0;
    uint8_t y1;
};

struct YUY2
{
    uint8_t y0;
    uint8_t u0;
    uint8_t y1;
    uint8_t v0;
};

struct Y41P
{
    uint8_t u0;
    uint8_t y0;
    uint8_t v0;
    uint8_t y1;

    uint8_t u4;
    uint8_t y2;
    uint8_t v4;
    uint8_t y3;

    uint8_t y4;
    uint8_t y5;
    uint8_t y6;
    uint8_t y7;
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

template<typename T>
T bound(T min, T value, T max)
{
    return value < min? min: value > max? max: value;
}

inline uint8_t rgb_y(int r, int g, int b)
{
    return uint8_t(((66 * r + 129 * g + 25 * b + 128) >> 8) + 16);
}

inline uint8_t rgb_u(int r, int g, int b)
{
    return uint8_t(((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128);
}

inline uint8_t rgb_v(int r, int g, int b)
{
    return uint8_t(((112 * r - 94 * g - 18 * b + 128) >> 8) + 128);
}

inline uint8_t yuv_r(int y, int u, int v)
{
    UNUSED(u)
    int r = (298 * (y - 16) + 409 * (v - 128) + 128) >> 8;

    return uint8_t(bound(0, r, 255));
}

inline uint8_t yuv_g(int y, int u, int v)
{
    int g = (298 * (y - 16) - 100 * (u - 128) - 208 * (v - 128) + 128) >> 8;

    return uint8_t(bound(0, g, 255));
}

inline uint8_t yuv_b(int y, int u, int v)
{
    UNUSED(v)
    int b = (298 * (y - 16) + 516 * (u - 128) + 128) >> 8;

    return uint8_t(bound(0, b, 255));
}

size_t rgb3_to_bgr3(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 24 * len / 8;

    if (!dst || !src)
        return osize;

    const RGB3 *_src = reinterpret_cast<const RGB3 *>(src);
    BGR3 *_dst = reinterpret_cast<BGR3 *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].r = _src[i].r;
        _dst[i].g = _src[i].g;
        _dst[i].b = _src[i].b;
    }

    return osize;
}

size_t rgb3_to_rgb4(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 32 * len / 8;

    if (!dst || !src)
        return osize;

    const RGB3 *_src = reinterpret_cast<const RGB3 *>(src);
    RGB4 *_dst = reinterpret_cast<RGB4 *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].a = 255;
        _dst[i].r = _src[i].r;
        _dst[i].g = _src[i].g;
        _dst[i].b = _src[i].b;
    }

    return osize;
}

size_t rgb3_to_bgr4(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 32 * len / 8;

    if (!dst || !src)
        return osize;

    const RGB3 *_src = reinterpret_cast<const RGB3 *>(src);
    BGR4 *_dst = reinterpret_cast<BGR4 *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].a = 255;
        _dst[i].r = _src[i].r;
        _dst[i].g = _src[i].g;
        _dst[i].b = _src[i].b;
    }

    return osize;
}

size_t rgb3_to_rgbp(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 16 * len / 8;

    if (!dst || !src)
        return osize;

    const RGB3 *_src = reinterpret_cast<const RGB3 *>(src);
    RGBP *_dst = reinterpret_cast<RGBP *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].r = _src[i].r >> 3;
        _dst[i].g = _src[i].g >> 2;
        _dst[i].b = _src[i].b >> 3;
    }

    return osize;
}

size_t rgb3_to_rgbo(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 16 * len / 8;

    if (!dst || !src)
        return osize;

    const RGB3 *_src = reinterpret_cast<const RGB3 *>(src);
    RGBO *_dst = reinterpret_cast<RGBO *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].r = _src[i].r >> 3;
        _dst[i].g = _src[i].g >> 3;
        _dst[i].b = _src[i].b >> 3;
        _dst[i].a = 1;
    }

    return osize;
}

size_t rgb3_to_uyvy(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 16 * len / 8;

    if (!dst || !src)
        return osize;

    const RGB3 *_src = reinterpret_cast<const RGB3 *>(src);
    UYVY *_dst = reinterpret_cast<UYVY *>(dst);

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

size_t rgb3_to_yuy2(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 16 * len / 8;

    if (!dst || !src)
        return osize;

    const RGB3 *_src = reinterpret_cast<const RGB3 *>(src);
    YUY2 *_dst = reinterpret_cast<YUY2 *>(dst);

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

size_t rgb3_to_yv12(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 12 * len / 8;

    if (!dst || !src)
        return osize;

    const RGB3 *_src = reinterpret_cast<const RGB3 *>(src);
    uint8_t *dst_y = reinterpret_cast<uint8_t *>(dst);
    uint8_t *dst_v = dst_y + len;
    uint8_t *dst_u = dst_v + (len >> 2);

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
        const RGB3 *line = _src + 2 * y * width;
        uint8_t *line_v = dst_v + y * _width;
        uint8_t *line_u = dst_u + y * _width;

        for (int x = 0; x < _width; x++) {
            int j = 2 * x;

            uint8_t r = line[j].r;
            uint8_t g = line[j].g;
            uint8_t b = line[j].b;

            line_v[x] = rgb_v(r, g, b);
            line_u[x] = rgb_u(r, g, b);
        }
    }

    return osize;
}

size_t rgb3_to_y41p(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 12 * len / 8;

    if (!dst || !src)
        return osize;

    const RGB3 *_src = reinterpret_cast<const RGB3 *>(src);
    Y41P *_dst = reinterpret_cast<Y41P *>(dst);

    size_t stride = 8;
    size_t olen = len / stride;

    for (size_t i = 0; i < olen; i++) {
        size_t j = stride * i;

        uint8_t r0 = _src[j].r;
        uint8_t g0 = _src[j].g;
        uint8_t b0 = _src[j].b;

        j++;

        uint8_t r1 = _src[j].r;
        uint8_t g1 = _src[j].g;
        uint8_t b1 = _src[j].b;

        j++;

        uint8_t r2 = _src[j].r;
        uint8_t g2 = _src[j].g;
        uint8_t b2 = _src[j].b;

        j++;

        uint8_t r3 = _src[j].r;
        uint8_t g3 = _src[j].g;
        uint8_t b3 = _src[j].b;

        j++;

        uint8_t r4 = _src[j].r;
        uint8_t g4 = _src[j].g;
        uint8_t b4 = _src[j].b;

        j++;

        uint8_t r5 = _src[j].r;
        uint8_t g5 = _src[j].g;
        uint8_t b5 = _src[j].b;

        j++;

        uint8_t r6 = _src[j].r;
        uint8_t g6 = _src[j].g;
        uint8_t b6 = _src[j].b;

        j++;

        uint8_t r7 = _src[j].r;
        uint8_t g7 = _src[j].g;
        uint8_t b7 = _src[j].b;

        _dst[i].u0 = rgb_u(r0, g0, b0);
        _dst[i].y0 = rgb_y(r0, g0, b0);
        _dst[i].v0 = rgb_v(r0, g0, b0);
        _dst[i].y1 = rgb_y(r1, g1, b1);

        _dst[i].u4 = rgb_u(r4, g4, b4);
        _dst[i].y2 = rgb_y(r2, g2, b2);
        _dst[i].v4 = rgb_v(r4, g4, b4);
        _dst[i].y3 = rgb_y(r3, g3, b3);

        _dst[i].y4 = rgb_y(r4, g4, b4);
        _dst[i].y5 = rgb_y(r5, g5, b5);
        _dst[i].y6 = rgb_y(r6, g6, b6);
        _dst[i].y7 = rgb_y(r7, g7, b7);
    }

    return osize;
}

size_t rgb3_to_nv12(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 12 * len / 8;

    if (!dst || !src)
        return osize;

    const RGB3 *_src = reinterpret_cast<const RGB3 *>(src);
    uint8_t *dst_y = reinterpret_cast<uint8_t *>(dst);
    VU *dst_vu = reinterpret_cast<VU *>(dst_y + len);

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
        const RGB3 *line = _src + 2 * y * width;
        VU *line_vu = dst_vu + y * _width;

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

size_t rgb3_to_nv21(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 12 * len / 8;

    if (!dst || !src)
        return osize;

    const RGB3 *_src = reinterpret_cast<const RGB3 *>(src);
    uint8_t *dst_y = reinterpret_cast<uint8_t *>(dst);
    UV *dst_uv = reinterpret_cast<UV *>(dst_y + len);

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
        const RGB3 *line = _src + 2 * y * width;
        UV *line_uv = dst_uv + y * _width;

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

size_t bgr3_to_bgr4(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 32 * len / 8;

    if (!dst || !src)
        return osize;

    const BGR3 *_src = reinterpret_cast<const BGR3 *>(src);
    BGR4 *_dst = reinterpret_cast<BGR4 *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].a = 255;
        _dst[i].r = _src[i].r;
        _dst[i].g = _src[i].g;
        _dst[i].b = _src[i].b;
    }

    return osize;
}

size_t bgr3_to_rgbp(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 16 * len / 8;

    if (!dst || !src)
        return osize;

    const BGR3 *_src = reinterpret_cast<const BGR3 *>(src);
    RGBP *_dst = reinterpret_cast<RGBP *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].r = _src[i].r >> 3;
        _dst[i].g = _src[i].g >> 2;
        _dst[i].b = _src[i].b >> 3;
    }

    return osize;
}

size_t bgr3_to_rgbo(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 16 * len / 8;

    if (!dst || !src)
        return osize;

    const BGR3 *_src = reinterpret_cast<const BGR3 *>(src);
    RGBO *_dst = reinterpret_cast<RGBO *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].r = _src[i].r >> 3;
        _dst[i].g = _src[i].g >> 3;
        _dst[i].b = _src[i].b >> 3;
        _dst[i].a = 1;
    }

    return osize;
}

size_t bgr3_to_uyvy(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 16 * len / 8;

    if (!dst || !src)
        return osize;

    const BGR3 *_src = reinterpret_cast<const BGR3 *>(src);
    UYVY *_dst = reinterpret_cast<UYVY *>(dst);

    size_t olen = len >> 1;

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

size_t bgr3_to_yuy2(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 16 * len / 8;

    if (!dst || !src)
        return osize;

    const BGR3 *_src = reinterpret_cast<const BGR3 *>(src);
    YUY2 *_dst = reinterpret_cast<YUY2 *>(dst);

    size_t olen = len >> 1;

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

size_t bgr3_to_yv12(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 12 * len / 8;

    if (!dst || !src)
        return osize;

    const BGR3 *_src = reinterpret_cast<const BGR3 *>(src);
    uint8_t *dst_y = reinterpret_cast<uint8_t *>(dst);
    uint8_t *dst_v = dst_y + len;
    uint8_t *dst_u = dst_v + (len >> 2);

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
        const BGR3 *line = _src + 2 * y * width;
        uint8_t *line_v = dst_v + y * _width;
        uint8_t *line_u = dst_u + y * _width;

        for (int x = 0; x < _width; x++) {
            int j = 2 * x;

            uint8_t r = line[j].r;
            uint8_t g = line[j].g;
            uint8_t b = line[j].b;

            line_v[x] = rgb_v(r, g, b);
            line_u[x] = rgb_u(r, g, b);
        }
    }

    return osize;
}

size_t rgb4_to_rgb3(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 24 * len / 8;

    if (!dst || !src)
        return osize;

    const RGB4 *_src = reinterpret_cast<const RGB4 *>(src);
    RGB3 *_dst = reinterpret_cast<RGB3 *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].r = _src[i].a * _src[i].r / 255;
        _dst[i].g = _src[i].a * _src[i].g / 255;
        _dst[i].b = _src[i].a * _src[i].b / 255;
    }

    return osize;
}

size_t rgb4_to_bgr3(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 24 * len / 8;

    if (!dst || !src)
        return osize;

    const RGB4 *_src = reinterpret_cast<const RGB4 *>(src);
    BGR3 *_dst = reinterpret_cast<BGR3 *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].b = _src[i].a * _src[i].b / 255;
        _dst[i].g = _src[i].a * _src[i].g / 255;
        _dst[i].r = _src[i].a * _src[i].r / 255;
    }

    return osize;
}

size_t bgr4_to_rgb3(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 24 * len / 8;

    if (!dst || !src)
        return osize;

    const BGR4 *_src = reinterpret_cast<const BGR4 *>(src);
    RGB3 *_dst = reinterpret_cast<RGB3 *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].r = _src[i].a * _src[i].r / 255;
        _dst[i].g = _src[i].a * _src[i].g / 255;
        _dst[i].b = _src[i].a * _src[i].b / 255;
    }

    return osize;
}

size_t bgr4_to_bgr3(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 24 * len / 8;

    if (!dst || !src)
        return osize;

    const BGR4 *_src = reinterpret_cast<const BGR4 *>(src);
    BGR3 *_dst = reinterpret_cast<BGR3 *>(dst);

    for (size_t i = 0; i < len; i++) {
        _dst[i].b = _src[i].a * _src[i].b / 255;
        _dst[i].g = _src[i].a * _src[i].g / 255;
        _dst[i].r = _src[i].a * _src[i].r / 255;
    }

    return osize;
}

size_t yuy2_to_rgb3(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 24 * len / 8;

    if (!dst || !src)
        return osize;

    const YUY2 *_src = reinterpret_cast<const YUY2 *>(src);
    RGB3 *_dst = reinterpret_cast<RGB3 *>(dst);

    size_t olen = len >> 1;

    for (size_t i = 0; i < olen; i++) {
        size_t j = 2 * i;

        uint8_t y0 = _src[i].y0;
        uint8_t u0 = _src[i].u0;
        uint8_t y1 = _src[i].y1;
        uint8_t v0 = _src[i].v0;

        _dst[j].r = yuv_r(y0, u0, v0);
        _dst[j].g = yuv_g(y0, u0, v0);
        _dst[j].b = yuv_b(y0, u0, v0);

        j++;

        _dst[j].r = yuv_r(y1, u0, v0);
        _dst[j].g = yuv_g(y1, u0, v0);
        _dst[j].b = yuv_b(y1, u0, v0);
    }

    return osize;
}

size_t i420_to_rgb3(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 24 * len / 8;

    if (!dst || !src)
        return osize;

    const uint8_t *src_y = reinterpret_cast<const uint8_t *>(src);
    const uint8_t *src_u = src_y + len;
    const uint8_t *src_v = src_u + (len >> 2);
    RGB3 *_dst = reinterpret_cast<RGB3 *>(dst);

    for (int y = 0; y < height; y++) {
        const uint8_t *line_y = src_y + y * width;
        const uint8_t *line_u = src_u + y * width / 2;
        const uint8_t *line_v = src_v + y * width / 2;
        RGB3 *line = _dst + y * width;

        for (int x = 0; x < width; x++) {
            int j = x / 2;

            uint8_t y = line_y[x];
            uint8_t u = line_u[j];
            uint8_t v = line_v[j];

            line[x].r = yuv_r(y, u, v);
            line[x].g = yuv_g(y, u, v);
            line[x].b = yuv_b(y, u, v);
        }
    }

    return osize;
}

size_t yuy2_to_bgr3(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 24 * len / 8;

    if (!dst || !src)
        return osize;

    const YUY2 *_src = reinterpret_cast<const YUY2 *>(src);
    BGR3 *_dst = reinterpret_cast<BGR3 *>(dst);

    size_t olen = len >> 1;

    for (size_t i = 0; i < olen; i++) {
        size_t j = 2 * i;

        uint8_t y0 = _src[i].y0;
        uint8_t u0 = _src[i].u0;
        uint8_t y1 = _src[i].y1;
        uint8_t v0 = _src[i].v0;

        _dst[j].b = yuv_b(y0, u0, v0);
        _dst[j].g = yuv_g(y0, u0, v0);
        _dst[j].r = yuv_r(y0, u0, v0);

        j++;

        _dst[j].b = yuv_b(y1, u0, v0);
        _dst[j].g = yuv_g(y1, u0, v0);
        _dst[j].r = yuv_r(y1, u0, v0);
    }

    return osize;
}

size_t i420_to_bgr3(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 24 * len / 8;

    if (!dst || !src)
        return osize;

    const uint8_t *src_y = reinterpret_cast<const uint8_t *>(src);
    const uint8_t *src_u = src_y + len;
    const uint8_t *src_v = src_u + (len >> 2);
    BGR3 *_dst = reinterpret_cast<BGR3 *>(dst);

    for (int y = 0; y < height; y++) {
        const uint8_t *line_y = src_y + y * width;
        const uint8_t *line_u = src_u + y * width / 2;
        const uint8_t *line_v = src_v + y * width / 2;
        BGR3 *line = _dst + y * width;

        for (int x = 0; x < width; x++) {
            int j = x / 2;

            uint8_t y = line_y[x];
            uint8_t u = line_u[j];
            uint8_t v = line_v[j];

            line[x].b = yuv_b(y, u, v);
            line[x].g = yuv_g(y, u, v);
            line[x].r = yuv_r(y, u, v);
        }
    }

    return osize;
}
