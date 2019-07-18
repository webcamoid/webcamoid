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
    T r;
    T g;
    T b;
    T a;

    inline Pixel(T r=0, T g=0, T b=0, T a=0):
        r(r), g(g), b(b), a(a)
    {
    }

    inline void clear() {
        this->r = 0;
        this->g = 0;
        this->b = 0;
        this->a = 0;
    }
};

// These are used as accumulators
typedef Pixel<qint32> IntegerPixel;
typedef Pixel<quint32> UIntegerPixel;
typedef Pixel<quint16> ShortPixel;
typedef Pixel<quint8> CharPixel;
typedef IntegerPixel HistogramListItem;

#endif // PIXELSTRUCTS_H
