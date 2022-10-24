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

#ifndef INTEGRAL_H
#define INTEGRAL_H

#include <qrgb.h>

template<typename T> class Pixel
{
    public:
        Pixel():
            r(0), g(0), b(0), a(0)
        {
        }

        Pixel(T r, T g, T b, T a):
            r(r), g(g), b(b), a(a)
        {
        }

        Pixel operator +(const Pixel &other) const
        {
            return Pixel(this->r + other.r,
                         this->g + other.g,
                         this->b + other.b,
                         this->a + other.a);
        }

        Pixel operator -(const Pixel &other) const
        {
            return Pixel(this->r - other.r,
                         this->g - other.g,
                         this->b - other.b,
                         this->a - other.a);
        }

        template <typename R> Pixel<R> operator /(R c) const
        {
            return Pixel<R>(this->r / c,
                            this->g / c,
                            this->b / c,
                            this->a / c);
        }

        Pixel &operator +=(const Pixel &other)
        {
            this->r += other.r;
            this->g += other.g;
            this->b += other.b;
            this->a += other.a;

            return *this;
        }

        Pixel &operator +=(QRgb pixel)
        {
            this->r += qRed(pixel);
            this->g += qGreen(pixel);
            this->b += qBlue(pixel);
            this->a += qAlpha(pixel);

            return *this;
        }

        T r;
        T g;
        T b;
        T a;
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

using PixelU32 = Pixel<quint32>;
using PixelReal = Pixel<qreal>;

#endif // INTEGRAL_H
