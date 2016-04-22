/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
        explicit Pixel():
            r(0), g(0), b(0), a(0), n(1)
        {
        }

        Pixel(T r, T g=0, T b=0, T a=0, T n=1):
            r(r), g(g), b(b), a(a), n(n)
        {
        }

        Pixel(const Pixel &other):
            r(other.r), g(other.g), b(other.b), a(other.a), n(other.n)
        {
        }

        Pixel(QRgb pixel):
            r(qRed(pixel)), g(qGreen(pixel)), b(qBlue(pixel)), a(qAlpha(pixel)), n(1)
        {
        }

        Pixel &operator =(const Pixel &other)
        {
            if (this != &other) {
                this->r = other.r;
                this->g = other.g;
                this->b = other.b;
                this->a = other.a;
                this->n = other.n;
            }

            return *this;
        }

        Pixel &operator =(QRgb pixel)
        {
            this->r = qRed(pixel);
            this->g = qGreen(pixel);
            this->b = qBlue(pixel);
            this->a = qAlpha(pixel);
            this->n = 1;

            return *this;
        }

        Pixel &operator +=(const Pixel &other)
        {
            this->r += other.r;
            this->g += other.g;
            this->b += other.b;
            this->a += other.a;
            this->n += other.n;

            return *this;
        }

        Pixel &operator +=(QRgb pixel)
        {
            this->r += qRed(pixel);
            this->g += qGreen(pixel);
            this->b += qBlue(pixel);
            this->a += qAlpha(pixel);
            this->n++;

            return *this;
        }

        operator QRgb() const
        {
            return qRgba(this->r / this->n,
                         this->g / this->n,
                         this->b / this->n,
                         this->a / this->n);
        }

        T r;
        T g;
        T b;
        T a;
        T n;
};

typedef Pixel<int> PixelInt;

#endif // INTEGRAL_H
