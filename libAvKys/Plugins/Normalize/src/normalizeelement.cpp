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

#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "normalizeelement.h"

#if 0
#define USE_FULLSWING
#endif

#ifdef USE_FULLSWING
    #define MIN_Y 0
    #define MAX_Y 255
#else
    #define MIN_Y 16
    #define MAX_Y 235
#endif

using HistogramType = quint64;

class NormalizeElementPrivate
{
    public:
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_ayuvpack, 0, 0, {}}};

        static void histogram(const AkVideoPacket &src, HistogramType *table);
        static void limits(const AkVideoPacket &src,
                           const HistogramType *histogram,
                           int &low, int &high);
        static void normalizationTable(const AkVideoPacket &src, quint8 *table);
};

NormalizeElement::NormalizeElement(): AkElement()
{
    this->d = new NormalizeElementPrivate;

#ifdef USE_FULLSWING
    this->d->m_videoConverter.setYuvColorSpaceType(AkVideoConverter::YuvColorSpaceType_FullSwing);
#endif
}

NormalizeElement::~NormalizeElement()
{
    delete this->d;
}

AkPacket NormalizeElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    quint8 normTable[256];
    NormalizeElementPrivate::normalizationTable(src, normTable);

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const AkYuv *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<AkYuv *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto &pixel = srcLine[x];
            auto y = qBound<int>(MIN_Y, akCompY(pixel), MAX_Y);
            dstLine[x] = akYuv(normTable[y],
                               akCompU(pixel),
                               akCompV(pixel),
                               akCompA(pixel));
        }
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void NormalizeElementPrivate::histogram(const AkVideoPacket &src,
                                        HistogramType *table)
{
    memset(table, 0, 256 * sizeof(HistogramType));

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const AkYuv *>(src.constLine(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto &pixel = srcLine[x];
            auto y = qBound<int>(MIN_Y, akCompY(pixel), MAX_Y);
            table[y]++;
        }
    }
}

void NormalizeElementPrivate::limits(const AkVideoPacket &src,
                                     const HistogramType *histogram,
                                     int &low, int &high)
{
    // The lowest and highest levels must occupy at least 0.1 % of the image.
    auto thresholdIntensity =
            size_t(src.caps().width()) * size_t(src.caps().height()) / 1000;
    int intensity = 0;

    for (low = 0; low < 256; low++) {
        intensity += histogram[low];

        if (intensity > thresholdIntensity)
            break;
    }

    intensity = 0;

    for (high = 255; high > 0; high--) {
        intensity += histogram[high];

        if (intensity > thresholdIntensity)
            break;
    }
}

void NormalizeElementPrivate::normalizationTable(const AkVideoPacket &src,
                                                 quint8 *table)
{
    HistogramType histogram[256];
    NormalizeElementPrivate::histogram(src, histogram);
    int low = 0;
    int high = 0;
    NormalizeElementPrivate::limits(src, histogram, low, high);

    if (low == high) {
        for (int i = 0; i < 256; i++)
            table[i] = i;
    } else {
        auto yDiff = MAX_Y - MIN_Y;
        auto q = high - low;

        for (int i = 0; i < 256; i++) {
            auto value = (yDiff * (i - low) + q * MIN_Y) / q;
            table[i] = quint8(qBound<int>(MIN_Y, value, MAX_Y));
        }
    }
}

#include "moc_normalizeelement.cpp"
