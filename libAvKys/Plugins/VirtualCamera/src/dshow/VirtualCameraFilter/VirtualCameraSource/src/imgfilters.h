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

#ifndef IMGFILTERS_H
#define IMGFILTERS_H

#include <cstdint>

size_t adjust_hsl(void *dst, const void *src,
                  int width, int height,
                  int hue, int saturation, int luminance);
size_t adjust_gamma(void *dst, const void *src,
                    int width, int height,
                    int gamma);
size_t adjust_contrast(void *dst, const void *src,
                       int width, int height,
                       int contrast);
size_t to_gray_scale(void *dst, const void *src,
                     int width, int height);
size_t adjust_image(void *dst, const void *src,
                    int width, int height,
                    int hue,
                    int saturation,
                    int luminance,
                    int gamma,
                    int contrast,
                    bool gray);

#endif // IMGFILTERS_H
