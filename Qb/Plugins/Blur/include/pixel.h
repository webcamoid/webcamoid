/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef INTEGRAL_H
#define INTEGRAL_H

#include <qrgb.h>

template<typename T> class Pixel
{
    public:
        explicit Pixel():
            r(0), g(0), b(0)
        {
        }
        Pixel(T r, T g, T b):
            r(r), g(g), b(b)
        {
        }

        Pixel operator +(const Pixel &other) const
        {
            return Pixel(this->r + other.r,
                         this->g + other.g,
                         this->b + other.b);
        }

        Pixel operator -(const Pixel &other) const
        {
            return Pixel(this->r - other.r,
                         this->g - other.g,
                         this->b - other.b);
        }

        template <typename R> Pixel<R> operator /(R c) const
        {
            return Pixel<R>(this->r / c,
                            this->g / c,
                            this->b / c);
        }

        Pixel &operator +=(const Pixel &other)
        {
            this->r += other.r;
            this->g += other.g;
            this->b += other.b;

            return *this;
        }

        Pixel &operator +=(QRgb pixel)
        {
            this->r += qRed(pixel);
            this->g += qGreen(pixel);
            this->b += qBlue(pixel);

            return *this;
        }

        T r;
        T g;
        T b;
};

template<typename T> inline Pixel<T> integralSum(const Pixel<T> *integral,
                                          int lineWidth,
                                          int x, int y, int kw, int kh)
{

    const Pixel<T> *p0 = integral + x + y * lineWidth;
    const Pixel<T> *p1 = p0 + kw;
    const Pixel<T> *p2 = p0 + kh * lineWidth;
    const Pixel<T> *p3 = p2 + kw;

    return *p0 + *p3 - *p1 - *p2;
}

typedef Pixel<quint32> PixelU32;
typedef Pixel<qreal> PixelReal;

#endif // INTEGRAL_H
