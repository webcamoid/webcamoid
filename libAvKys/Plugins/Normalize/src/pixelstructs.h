/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#ifndef PIXELSTRUCTS_H
#define PIXELSTRUCTS_H

#include <qglobal.h>

template<typename T>
struct Pixel
{
    T red;
    T green;
    T blue;
    T alpha;

    inline Pixel(T red=0, T green=0, T blue=0, T alpha=0):
        red(red), green(green), blue(blue), alpha(alpha)
    {
    }

    inline void clear() {
        this->red = 0;
        this->green = 0;
        this->blue = 0;
        this->alpha = 0;
    }
};

// These are used as accumulators
using IntegerPixel = struct Pixel<qint32>;
using UIntegerPixel = struct Pixel<quint32>;
using ShortPixel = struct Pixel<quint16>;
using CharPixel = struct Pixel<quint8>;
using HistogramListItem = IntegerPixel;

#endif // PIXELSTRUCTS_H
