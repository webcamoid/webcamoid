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
    Pixel():
        r(0), g(0), b(0), a(0)
    {
    }

    Pixel(T red, T green, T blue, T alpha):
        r(red), g(green), b(blue), a(alpha)
    {
    }

    Pixel operator +(const Pixel &other)
    {
        return Pixel(this->r + other.r,
                     this->g + other.g,
                     this->b + other.b,
                     this->a + other.a);
    }

    void clear()
    {
        this->r = 0;
        this->g = 0;
        this->b = 0;
        this->a = 0;
    }

    T r;
    T g;
    T b;
    T a;
};

// These are used as accumulators
using Integer64Pixel = struct Pixel<quint64>;
using IntegerPixel = struct Pixel<quint32>;
using ShortPixel = struct Pixel<quint16>;
using CharPixel = struct Pixel<quint8>;
using HistogramListItem = IntegerPixel;

#endif // PIXELSTRUCTS_H
