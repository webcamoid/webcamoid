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

#ifndef COLORCONV_H
#define COLORCONV_H

#include <cstdint>

// RGB3 -> RGB
size_t rgb3_to_bgr3(void *dst, const void *src, int width, int height);
size_t rgb3_to_rgb4(void *dst, const void *src, int width, int height);
size_t rgb3_to_bgr4(void *dst, const void *src, int width, int height);
size_t rgb3_to_rgbp(void *dst, const void *src, int width, int height);
size_t rgb3_to_rgbo(void *dst, const void *src, int width, int height);

// RGB3 -> YUV
size_t rgb3_to_uyvy(void *dst, const void *src, int width, int height);
size_t rgb3_to_yuy2(void *dst, const void *src, int width, int height);
size_t rgb3_to_yv12(void *dst, const void *src, int width, int height);
size_t rgb3_to_y41p(void *dst, const void *src, int width, int height);
size_t rgb3_to_nv12(void *dst, const void *src, int width, int height);
size_t rgb3_to_nv21(void *dst, const void *src, int width, int height);

// BGR3 -> RGB
size_t bgr3_to_bgr4(void *dst, const void *src, int width, int height);
size_t bgr3_to_rgbp(void *dst, const void *src, int width, int height);
size_t bgr3_to_rgbo(void *dst, const void *src, int width, int height);

// BGR3 -> YUV
size_t bgr3_to_uyvy(void *dst, const void *src, int width, int height);
size_t bgr3_to_yuy2(void *dst, const void *src, int width, int height);
size_t bgr3_to_yv12(void *dst, const void *src, int width, int height);

// RGB4 -> RGB
size_t rgb4_to_rgb3(void *dst, const void *src, int width, int height);
size_t rgb4_to_bgr3(void *dst, const void *src, int width, int height);

// BGR4 -> RGB
size_t bgr4_to_rgb3(void *dst, const void *src, int width, int height);
size_t bgr4_to_bgr3(void *dst, const void *src, int width, int height);

// YUV -> RGB
size_t yuy2_to_rgb3(void *dst, const void *src, int width, int height);
size_t i420_to_rgb3(void *dst, const void *src, int width, int height);

// YUV -> RGB
size_t yuy2_to_bgr3(void *dst, const void *src, int width, int height);
size_t i420_to_bgr3(void *dst, const void *src, int width, int height);

#endif // COLORCONV_H
