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
#include <algorithm>
#include <list>
#include <cmath>

#include "imgfilters.h"

struct BGR3
{
    uint8_t b;
    uint8_t g;
    uint8_t r;
};

template<typename T>
inline T bound(T min, T value, T max)
{
    return value < min? min: value > max? max: value;
}

inline int grayval(int r, int g, int b)
{
    return (11 * r + 16 * g + 5 * b) >> 5;
}

template<typename T>
inline T mod(T value, T mod)
{
    return ((value < 0? mod: 0) + value) % mod;
}

// https://en.wikipedia.org/wiki/HSL_and_HSV
inline void rgb_to_hsl(int r, int g, int b, int *h, int *s, int *l)
{
    int max = std::max(r, std::max(g, b));
    int min = std::min(r, std::min(g, b));
    int c = max - min;

    *l = (max + min) / 2;

    if (!c) {
        *h = 0;
        *s = 0;
    } else {
        if (max == r)
            *h = mod(g - b, 6 * c);
        else if (max == g)
            *h = b - r + 2 * c;
        else
            *h = r - g + 4 * c;

        *h = 60 * (*h) / c;
        *s = 255 * c / (255 - abs(max + min - 255));
    }
}

inline void hsl_to_rgb(int h, int s, int l, int *r, int *g, int *b)
{
    int c = s * (255 - abs(2 * l - 255)) / 255;
    int x = c * (60 - abs((h % 120) - 60)) / 60;

    if (h >= 0 && h < 60) {
        *r = c;
        *g = x;
        *b = 0;
    } else if (h >= 60 && h < 120) {
        *r = x;
        *g = c;
        *b = 0;
    } else if (h >= 120 && h < 180) {
        *r = 0;
        *g = c;
        *b = x;
    } else if (h >= 180 && h < 240) {
        *r = 0;
        *g = x;
        *b = c;
    } else if (h >= 240 && h < 300) {
        *r = x;
        *g = 0;
        *b = c;
    } else if (h >= 300 && h < 360) {
        *r = c;
        *g = 0;
        *b = x;
    } else {
        *r = 0;
        *g = 0;
        *b = 0;
    }

    int m = 2 * l - c;

    *r = (2 * (*r) + m) / 2;
    *g = (2 * (*g) + m) / 2;
    *b = (2 * (*b) + m) / 2;
}

inline std::vector<uint8_t> init_gamma_table()
{
    std::list<uint8_t> gamma_table;

    for (int i = 0; i < 256; i++) {
        uint8_t ig = uint8_t(255. * pow(i / 255., 255));
        gamma_table.push_back(ig);
    }

    for (int gamma = -254; gamma < 256; gamma++) {
        double k = 255. / (gamma + 255);

        for (int i = 0; i < 256; i++) {
            uint8_t ig = uint8_t(255. * pow(i / 255., k));
            gamma_table.push_back(ig);
        }
    }

    return std::vector<uint8_t>(gamma_table.begin(), gamma_table.end());
}

static std::vector<uint8_t> gamma_table = init_gamma_table();

inline std::vector<uint8_t> init_contrast_table()
{
    std::list<uint8_t> contrast_table;

    for (int contrast = -255; contrast < 256; contrast++) {
        double f = 259. * (255 + contrast) / (255. * (259 - contrast));

        for (int i = 0; i < 256; i++) {
            int ic = int(f * (i - 128) + 128.);
            contrast_table.push_back(uint8_t(bound(0, ic, 255)));
        }
    }

    return std::vector<uint8_t>(contrast_table.begin(), contrast_table.end());
}

static std::vector<uint8_t> contrast_table = init_contrast_table();

size_t adjust_hsl(void *dst, const void *src,
                  int width, int height,
                  int hue, int saturation, int luminance)
{
    size_t len = size_t(width * height);
    size_t osize = 24 * len / 8;

    if (!dst || !src)
        return osize;

    const BGR3 *_src = reinterpret_cast<const BGR3 *>(src);
    BGR3 *_dst = reinterpret_cast<BGR3 *>(dst);

    for (size_t i = 0; i < len; i++) {
        int h;
        int s;
        int l;
        rgb_to_hsl(_src[i].r, _src[i].g, _src[i].b, &h, &s, &l);

        h = bound(0, h + hue, 359);
        s = bound(0, s + saturation, 255);
        l = bound(0, l + luminance, 255);

        int r;
        int g;
        int b;
        hsl_to_rgb(h, s, l, &r, &g, &b);

        _dst[i].r = uint8_t(r);
        _dst[i].g = uint8_t(g);
        _dst[i].b = uint8_t(b);
    }

    return osize;
}

size_t adjust_gamma(void *dst, const void *src,
                    int width, int height,
                    int gamma)
{
    size_t len = size_t(width * height);
    size_t osize = 24 * len / 8;

    if (!dst || !src)
        return osize;

    const BGR3 *_src = reinterpret_cast<const BGR3 *>(src);
    BGR3 *_dst = reinterpret_cast<BGR3 *>(dst);

    gamma = bound(-255, gamma, 255);
    size_t gamma_offset = size_t(gamma + 255) << 8;

    for (size_t i = 0; i < len; i++) {
        _dst[i].r = gamma_table[gamma_offset | _src[i].r];
        _dst[i].g = gamma_table[gamma_offset | _src[i].g];
        _dst[i].b = gamma_table[gamma_offset | _src[i].b];
    }

    return osize;
}

size_t adjust_contrast(void *dst, const void *src, int width, int height, int contrast)
{
    size_t len = size_t(width * height);
    size_t osize = 24 * len / 8;

    if (!dst || !src)
        return osize;

    const BGR3 *_src = reinterpret_cast<const BGR3 *>(src);
    BGR3 *_dst = reinterpret_cast<BGR3 *>(dst);

    contrast = bound(-255, contrast, 255);
    size_t contrast_offset = size_t(contrast + 255) << 8;

    for (size_t i = 0; i < len; i++) {
        _dst[i].r = contrast_table[contrast_offset | _src[i].r];
        _dst[i].g = contrast_table[contrast_offset | _src[i].g];
        _dst[i].b = contrast_table[contrast_offset | _src[i].b];
    }

    return osize;
}

size_t to_gray_scale(void *dst, const void *src, int width, int height)
{
    size_t len = size_t(width * height);
    size_t osize = 24 * len / 8;

    if (!dst || !src)
        return osize;

    const BGR3 *_src = reinterpret_cast<const BGR3 *>(src);
    BGR3 *_dst = reinterpret_cast<BGR3 *>(dst);

    for (size_t i = 0; i < len; i++) {
        int luma = grayval(_src[i].r, _src[i].g, _src[i].b);

        _dst[i].r = uint8_t(luma);
        _dst[i].g = uint8_t(luma);
        _dst[i].b = uint8_t(luma);
    }

    return osize;
}

size_t adjust_image(void *dst, const void *src,
                    int width, int height,
                    int hue,
                    int saturation,
                    int luminance,
                    int gamma,
                    int contrast,
                    bool gray)
{
    size_t len = size_t(width * height);
    size_t osize = 24 * len / 8;

    if (!dst || !src)
        return osize;

    const BGR3 *_src = reinterpret_cast<const BGR3 *>(src);
    BGR3 *_dst = reinterpret_cast<BGR3 *>(dst);

    gamma = bound(-255, gamma, 255);
    size_t gamma_offset = size_t(gamma + 255) << 8;

    contrast = bound(-255, contrast, 255);
    size_t contrast_offset = size_t(contrast + 255) << 8;

    for (size_t i = 0; i < len; i++) {
        int h;
        int s;
        int l;
        rgb_to_hsl(_src[i].r, _src[i].g, _src[i].b, &h, &s, &l);

        h = bound(0, h + hue, 359);
        s = bound(0, s + saturation, 255);
        l = bound(0, l + luminance, 255);

        int r;
        int g;
        int b;
        hsl_to_rgb(h, s, l, &r, &g, &b);

        r = gamma_table[gamma_offset | size_t(r)];
        g = gamma_table[gamma_offset | size_t(g)];
        b = gamma_table[gamma_offset | size_t(b)];

        r = contrast_table[contrast_offset | size_t(r)];
        g = contrast_table[contrast_offset | size_t(g)];
        b = contrast_table[contrast_offset | size_t(b)];

        if (gray) {
            int luma = grayval(r, g, b);

            r = luma;
            g = luma;
            b = luma;
        }

        _dst[i].r = uint8_t(r);
        _dst[i].g = uint8_t(g);
        _dst[i].b = uint8_t(b);
    }

    return osize;
}
