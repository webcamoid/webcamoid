/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include "equalizeelement.h"

EqualizeElement::EqualizeElement(): AkElement()
{
}

QVector<quint64> EqualizeElement::histogram(const QImage &img) const
{
    QVector<quint64> histogram(256, 0);

    for (int y = 0; y < img.height(); y++) {
        const QRgb *srcLine = reinterpret_cast<const QRgb *>(img.constScanLine(y));

        for (int x = 0; x < img.width(); x++)
            histogram[qGray(srcLine[x])]++;
    }

    return histogram;
}

QVector<quint64> EqualizeElement::cumulativeHistogram(const QVector<quint64> &histogram) const
{
    QVector<quint64> cumulativeHistogram(histogram.size());
    quint64 sum = 0;

    for (int i = 0; i < histogram.size(); i++) {
        sum += histogram[i];
        cumulativeHistogram[i] = sum;
    }

    return cumulativeHistogram;
}

QVector<quint8> EqualizeElement::equalizationTable(const QImage &img) const
{
    QVector<quint64> cumHist = this->cumulativeHistogram(this->histogram(img));
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

AkPacket EqualizeElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());
    QVector<quint8> equTable = this->equalizationTable(src);

    for (int y = 0; y < src.height(); y++) {
        const QRgb *srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        QRgb *dstLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < src.width(); x++){
            int r = equTable[qRed(srcLine[x])];
            int g = equTable[qGreen(srcLine[x])];
            int b = equTable[qBlue(srcLine[x])];
            int a = equTable[qAlpha(srcLine[x])];

            dstLine[x] = qRgba(r, g, b, a);
        }
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
