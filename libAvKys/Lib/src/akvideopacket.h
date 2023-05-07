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

#ifndef AKVIDEOPACKET_H
#define AKVIDEOPACKET_H

#include <qrgb.h>

#include "akpacketbase.h"
#include "akvideocaps.h"

using AkYuv = quint32;

inline int akCompY(AkYuv yuv)
{
    return (yuv >> 16) & 0xff;
}

inline int akCompU(AkYuv yuv)
{
    return (yuv >> 8) & 0xff;
}

inline int akCompV(AkYuv yuv)
{
    return yuv & 0xff;
}

inline int akCompA(AkYuv yuv)
{
    return yuv >> 24;
}

inline AkYuv akYuv(int y, int u, int v, int a)
{
    return ((a & 0xff) << 24) | ((y & 0xff) << 16) | ((u & 0xff) << 8) | (v & 0xff);
}

inline AkYuv akYuv(int y, int u, int v)
{
    return akYuv(y, v, u, 255);
}

class AkVideoPacketPrivate;
class AkPacket;

class AKCOMMONS_EXPORT AkVideoPacket: public AkPacketBase
{
    Q_OBJECT
    Q_PROPERTY(AkVideoCaps caps
               READ caps
               CONSTANT)
    Q_PROPERTY(size_t size
               READ size
               CONSTANT)
    Q_PROPERTY(size_t planes
               READ planes
               CONSTANT)

    public:
        AkVideoPacket(QObject *parent=nullptr);
        AkVideoPacket(const AkVideoCaps &caps,
                      bool initialized=false,
                      size_t align=32);
        AkVideoPacket(const AkPacket &other);
        AkVideoPacket(const AkVideoPacket &other);
        ~AkVideoPacket();
        AkVideoPacket &operator =(const AkPacket &other);
        AkVideoPacket &operator =(const AkVideoPacket &other);
        operator bool() const;
        operator AkPacket() const;

        Q_INVOKABLE const AkVideoCaps &caps() const;
        Q_INVOKABLE size_t size() const;
        Q_INVOKABLE size_t planes() const;
        Q_INVOKABLE size_t planeSize(int plane) const;
        Q_INVOKABLE size_t pixelSize(int plane) const;
        Q_INVOKABLE size_t lineSize(int plane) const;
        Q_INVOKABLE size_t bytesUsed(int plane) const;
        Q_INVOKABLE size_t widthDiv(int plane) const;
        Q_INVOKABLE size_t heightDiv(int plane) const;
        Q_INVOKABLE const char *constData() const;
        Q_INVOKABLE char *data();
        Q_INVOKABLE const quint8 *constPlane(int plane) const;
        Q_INVOKABLE quint8 *plane(int plane);
        Q_INVOKABLE const quint8 *constLine(int plane, int y) const;
        Q_INVOKABLE quint8 *line(int plane, int y);
        Q_INVOKABLE AkVideoPacket copy(int x,
                                       int y,
                                       int width,
                                       int height) const;

        template <typename T>
        inline T pixel(int plane, int x, int y) const
        {
            auto line = reinterpret_cast<const T *>(this->constLine(plane, y));

            return line[x >> this->widthDiv(plane)];
        }

        template <typename T>
        inline void setPixel(int plane, int x, int y, T value)
        {
            auto line = reinterpret_cast<T *>(this->line(plane, y));
            line[x >> this->widthDiv(plane)] = value;
        }

        template <typename T>
        inline void fill(int plane, T value)
        {
            int width = this->caps().width() >> this->widthDiv(plane);
            auto line = new T [width];

            for (int x = 0; x < width; x++)
                line[x] = value;

            size_t lineSize = width * sizeof(T);

            for (int y = 0; y < this->caps().height(); y++)
                memcpy(this->line(plane, y), line, lineSize);

            delete [] line;
        }

        template <typename T>
        inline void fill(T value)
        {
            for (size_t plane = 0; plane < this->planes(); plane++)
                this->fill(plane, value);
        }

        Q_INVOKABLE void fillRgb(QRgb color);

    private:
        AkVideoPacketPrivate *d;

    public Q_SLOTS:
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkVideoPacket &packet);

Q_DECLARE_METATYPE(AkVideoPacket)

#endif // AKVIDEOPACKET_H
