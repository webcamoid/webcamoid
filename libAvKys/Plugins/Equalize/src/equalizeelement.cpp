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

#include "equalizeelement.h"

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
using HistogramSumType = qreal;

class EqualizeElementPrivate
{
    public:
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_ayuvpack, 0, 0, {}}};

        static void histogram(const AkVideoPacket &src, HistogramType *table);
        static void cumulativeHistogram(const HistogramType *histogram,
                                        HistogramSumType *cumHistogram);
        static void equalizationTable(const AkVideoPacket &src, quint8 *table);
};

EqualizeElement::EqualizeElement():
    AkElement()
{
    this->d = new EqualizeElementPrivate;

#ifdef USE_FULLSWING
    this->d->m_videoConverter.setYuvColorSpaceType(AkVideoConverter::YuvColorSpaceType_FullSwing);
#endif
}

EqualizeElement::~EqualizeElement()
{
    delete this->d;
}

AkPacket EqualizeElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    quint8 equTable[256];
    EqualizeElementPrivate::equalizationTable(src, equTable);

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const AkYuv *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<AkYuv *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto &pixel = srcLine[x];
            auto y = qBound<int>(MIN_Y, akCompY(pixel), MAX_Y);
            dstLine[x] = akYuv(equTable[y],
                               akCompU(pixel),
                               akCompV(pixel),
                               akCompA(pixel));
        }
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void EqualizeElementPrivate::histogram(const AkVideoPacket &src,
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

void EqualizeElementPrivate::cumulativeHistogram(const HistogramType *histogram,
                                                 HistogramSumType *cumHistogram)
{
    HistogramSumType sum = 0;

    for (int i = 0; i < 256; i++) {
        sum += histogram[i];
        cumHistogram[i] = sum;
    }
}

void EqualizeElementPrivate::equalizationTable(const AkVideoPacket &src,
                                               quint8 *table)
{
    HistogramType histogram[256];
    EqualizeElementPrivate::histogram(src, histogram);
    HistogramSumType cumHistogram[256];
    EqualizeElementPrivate::cumulativeHistogram(histogram, cumHistogram);

    auto &hMinY = cumHistogram[MIN_Y];
    auto &hMaxY = cumHistogram[MAX_Y];

    if (qFuzzyCompare(hMinY, hMaxY)) {
        for (int i = 0; i < 256; i++)
            table[i] = i;
    } else {
        auto yDiff = MAX_Y - MIN_Y;
        auto q = hMaxY - hMinY;

        for (int i = 0; i < 256; i++) {
            auto value = qRound((yDiff * (cumHistogram[i] - hMinY) + q * MIN_Y) / q);
            table[i] = quint8(qBound<int>(MIN_Y, value, MAX_Y));
        }
    }
}

#include "moc_equalizeelement.cpp"
