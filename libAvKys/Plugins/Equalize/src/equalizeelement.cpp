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

#include <QImage>
#include <QVector>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "equalizeelement.h"
#include "pixelstructs.h"

class EqualizeElementPrivate
{
    public:
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argb, 0, 0, {}}};

        static QVector<quint64> histogram(const QImage &img);
        static QVector<quint64> cumulativeHistogram(const QVector<quint64> &histogram);
        static QVector<quint8> equalizationTable(const QImage &img);
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
    auto src = this->d->m_videoConverter.convertToImage(packet);

    if (src.isNull())
        return AkPacket();

    QImage oFrame(src.size(), src.format());
    auto equTable = EqualizeElementPrivate::equalizationTable(src);

    for (int y = 0; y < src.height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        auto dstLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < src.width(); x++){
            int r = equTable[qRed(srcLine[x])];
            int g = equTable[qGreen(srcLine[x])];
            int b = equTable[qBlue(srcLine[x])];
            int a = equTable[qAlpha(srcLine[x])];

            dstLine[x] = qRgba(r, g, b, a);
        }
    }

    auto oPacket = this->d->m_videoConverter.convert(oFrame, packet);

    if (oPacket)
        emit this->oStream(oPacket);

    return oPacket;
}

QVector<quint64> EqualizeElementPrivate::histogram(const QImage &img)
{
    QVector<quint64> histogram(256, 0);

    for (int y = 0; y < img.height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(img.constScanLine(y));

        for (int x = 0; x < img.width(); x++)
            histogram[qGray(srcLine[x])]++;
    }

    return histogram;
}

QVector<quint64> EqualizeElementPrivate::cumulativeHistogram(const QVector<quint64> &histogram)
{
    QVector<quint64> cumulativeHistogram(histogram.size());
    quint64 sum = 0;

    for (int i = 0; i < histogram.size(); i++) {
        sum += histogram[i];
        cumulativeHistogram[i] = sum;
    }

    return cumulativeHistogram;
}

QVector<quint8> EqualizeElementPrivate::equalizationTable(const QImage &img)
{
    auto histogram = EqualizeElementPrivate::histogram(img);
    auto cumHist = EqualizeElementPrivate::cumulativeHistogram(histogram);
    QVector<quint8> equalizationTable(cumHist.size());
    int maxLevel = cumHist.size() - 1;
    quint64 q = cumHist[maxLevel] - cumHist[0];

    for (int i = 0; i < cumHist.size(); i++)
        if (cumHist[i] > cumHist[0])
            equalizationTable[i] = quint8(qRound(qreal(maxLevel)
                                                 * (cumHist[i] - cumHist[0])
                                                 / q));
        else
            equalizationTable[i] = 0;

    return equalizationTable;
}

#include "moc_equalizeelement.cpp"
