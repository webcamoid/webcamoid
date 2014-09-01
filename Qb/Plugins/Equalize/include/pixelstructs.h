/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#ifndef PIXELSTRUCTS_H
#define PIXELSTRUCTS_H

#include <qglobal.h>

template<typename T>
struct Pixel
{
    Pixel():
        red(0), green(0), blue(0), alpha(0)
    {
    }

    Pixel(T red, T green, T blue, T alpha):
        red(red), green(green), blue(blue), alpha(alpha)
    {
    }

    void clear() {
        this->red = 0;
        this->green = 0;
        this->blue = 0;
        this->alpha = 0;
    }

    T red;
    T green;
    T blue;
    T alpha;
};

// These are used as accumulators
typedef struct Pixel<quint32> IntegerPixel;
typedef struct Pixel<quint16> ShortPixel;
typedef struct Pixel<quint8>  CharPixel;
typedef IntegerPixel HistogramListItem;

#endif // PIXELSTRUCTS_H
