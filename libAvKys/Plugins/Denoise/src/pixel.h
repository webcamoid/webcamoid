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

#ifndef PIXEL_H
#define PIXEL_H

#include <cmath>
#include <qrgb.h>

template<typename T> class Pixel
{
    public:
        Pixel():
            r(0), g(0), b(0)
        {
        }
        Pixel(T r, T g, T b):
            r(r), g(g), b(b)
        {
        }

        template<typename R> Pixel &operator =(const Pixel<R> &other)
        {
            if (reinterpret_cast<void *>(this)
                != reinterpret_cast<const void *>(&other)) {
                this->r = other.r;
                this->g = other.g;
                this->b = other.b;
            }

            return *this;
        }

        Pixel &operator =(QRgb pixel)
        {
            this->r = qRed(pixel);
            this->g = qGreen(pixel);
            this->b = qBlue(pixel);

            return *this;
        }

        Pixel operator +(const Pixel &other) const
        {
            return Pixel(this->r + other.r,
                         this->g + other.g,
                         this->b + other.b);
        }

        Pixel operator +(int c) const
        {
            return Pixel(this->r + c,
                         this->g + c,
                         this->b + c);
        }

        template <typename R> Pixel operator -(const Pixel<R> &other) const
        {
            return Pixel(this->r - other.r,
                         this->g - other.g,
                         this->b - other.b);
        }

        template <typename R> Pixel operator *(const Pixel<R> &other) const
        {
            return Pixel(this->r * other.r,
                         this->g * other.g,
                         this->b * other.b);
        }

        template <typename R> Pixel<R> operator /(R c) const
        {
            return Pixel<R>(this->r / c,
                            this->g / c,
                            this->b / c);
        }

        Pixel operator <<(int bits) const
        {
            return Pixel(this->r << bits,
                         this->g << bits,
                         this->b << bits);
        }

        template <typename R> Pixel operator |(const Pixel<R> &other) const
        {
            return Pixel(this->r | other.r,
                         this->g | other.g,
                         this->b | other.b);
        }

        template <typename R> Pixel &operator +=(const Pixel<R> &other)
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

using PixelI8 = Pixel<qint8>;
using PixelU8 = Pixel<quint8>;
using PixelI32 = Pixel<qint32>;
using PixelU32 = Pixel<quint32>;
using PixelI64 = Pixel<qint64>;
using PixelU64 = Pixel<quint64>;
using PixelReal = Pixel<qreal>;

template <typename R, typename S> inline Pixel<R> mult(R c, const Pixel<S> &pixel)
{
    return Pixel<R>(c * pixel.r, c * pixel.g, c * pixel.b);
}

inline PixelU64 pow2(QRgb pixel)
{
    auto r = quint8(qRed(pixel));
    auto g = quint8(qGreen(pixel));
    auto b = quint8(qBlue(pixel));

    return PixelU64(r * r, g * g, b * b);
}

template<typename T> inline Pixel<T> pow2(const Pixel<T> &pixel)
{
    return Pixel<T>(pixel.r * pixel.r,
                    pixel.g * pixel.g,
                    pixel.b * pixel.b);
}

template<typename T> inline Pixel<T> sqrt(const Pixel<T> &pixel)
{
    return Pixel<T>(std::sqrt(pixel.r),
                    std::sqrt(pixel.g),
                    std::sqrt(pixel.b));
}

template<typename T> Pixel<T> bound(T min, const Pixel<T> &pixel, T max)
{
    return Pixel<T>(qBound(min, pixel.r, max),
                    qBound(min, pixel.g, max),
                    qBound(min, pixel.b, max));
}

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

template <typename T> inline Pixel<quint32> operator *(quint32 c, const Pixel<T> &pixel)
{
    return Pixel<quint32>(c * pixel.r,
                          c * pixel.g,
                          c * pixel.b);
}

#endif // PIXEL_H

