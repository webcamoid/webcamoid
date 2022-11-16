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
#include "pixelstructs.h"

using HistogramType = quint64;
using HistogramSumType = qreal;

class EqualizeElementPrivate
{
    public:
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        static void histogram(const AkVideoPacket &src, HistogramType *table);
        static void cumulativeHistogram(const HistogramType *histogram,
                                        HistogramSumType *cumHistogram);
        static void equalizationTable(const AkVideoPacket &src, quint8 *table);
};

EqualizeElement::EqualizeElement():
    AkElement()
{
    this->d = new EqualizeElementPrivate;
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
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++){
            auto &pixel = srcLine[x];
            int r = equTable[qRed(pixel)];
            int g = equTable[qGreen(pixel)];
            int b = equTable[qBlue(pixel)];
            dstLine[x] = qRgba(r, g, b, qAlpha(pixel));
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
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));

        for (int x = 0; x < src.caps().width(); x++)
            table[qGray(srcLine[x])]++;
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
    auto q = cumHistogram[255] - cumHistogram[0];

    for (int i = 0; i < 256; i++)
        table[i] = quint8(qRound(255.0 * (cumHistogram[i] - cumHistogram[0]) / q));
}

#include "moc_equalizeelement.cpp"
