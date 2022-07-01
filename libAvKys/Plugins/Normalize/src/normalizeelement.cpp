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
#include <akfrac.h>
#include <akpacket.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "normalizeelement.h"
#include "pixelstructs.h"

class NormalizeElementPrivate
{
    public:
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argb, 0, 0, {}}};
};

NormalizeElement::NormalizeElement(): AkElement()
{
    this->d = new NormalizeElementPrivate;
}

NormalizeElement::~NormalizeElement()
{
    delete this->d;
}

AkPacket NormalizeElement::iVideoStream(const AkVideoPacket &packet)
{
    auto oFrame = this->d->m_videoConverter.convertToImage(packet);

    if (oFrame.isNull())
        return {};

    // form histogram
    QVector<HistogramListItem> histogram(256, HistogramListItem());

    for (int y = 0; y < oFrame.height(); y++) {
        const QRgb *dstLine = reinterpret_cast<const QRgb *>(oFrame.constScanLine(y));

        for (int x = 0; x < oFrame.width(); x++) {
            auto pixel = dstLine[x];
            histogram[qRed(pixel)].r++;
            histogram[qGreen(pixel)].g++;
            histogram[qBlue(pixel)].b++;
            histogram[qAlpha(pixel)].a++;
        }
    }

    // find the histogram boundaries by locating the .01 percent levels.
    ShortPixel high, low;
    auto thresholdIntensity = qint32(oFrame.width() * oFrame.height() / 1e3);
    IntegerPixel intensity;

    for (low.r = 0; low.r < 256; low.r++) {
        intensity.r += histogram[low.r].r;

        if (intensity.r > thresholdIntensity)
            break;
    }

    intensity.clear();

    for (high.r = 255; high.r > 0; high.r--) {
        intensity.r += histogram[high.r].r;

        if (intensity.r > thresholdIntensity)
            break;
    }

    intensity.clear();

    for (low.g = low.r; low.g < high.r; low.g++) {
        intensity.g += histogram[low.g].g;

        if (intensity.g > thresholdIntensity)
            break;
    }

    intensity.clear();

    for (high.g = high.r; high.g != low.r; high.g--) {
        intensity.g += histogram[high.g].g;

        if (intensity.g > thresholdIntensity)
            break;
    }

    intensity.clear();

    for (low.b = low.g; low.b < high.g; low.b++) {
        intensity.b += histogram[low.b].b;

        if (intensity.b > thresholdIntensity)
            break;
    }

    intensity.clear();

    for (high.b = high.g; high.b != low.g; high.b--) {
        intensity.b += histogram[high.b].b;

        if (intensity.b > thresholdIntensity)
            break;
    }

    // stretch the histogram to create the normalized image mapping.
    QVector<IntegerPixel> normalizeMap(256);

    for (int i = 0; i < 256; i++) {
        if(i < low.r)
            normalizeMap[i].r = 0;
        else {
            if (i > high.r)
                normalizeMap[i].r = 255;
            else if (low.r != high.r)
                normalizeMap[i].r = (255 * (i - low.r)) /
                    (high.r - low.r);
        }

        if (i < low.g)
            normalizeMap[i].g = 0;
        else {
            if(i > high.g)
                normalizeMap[i].g = 255;
            else if(low.g != high.g)
                normalizeMap[i].g = (255 * (i - low.g)) /
                    (high.g - low.g);
        }

        if (i < low.b)
            normalizeMap[i].b = 0;
        else {
            if (i > high.b)
                normalizeMap[i].b = 255;
            else if (low.b != high.b)
                normalizeMap[i].b = (255*(i-low.b)) /
                    (high.b - low.b);
        }
    }

    // write
    for (int y = 0; y < oFrame.height(); y++) {
        auto oLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < oFrame.width(); x++) {
            auto pixel = oLine[x];

            int r = (low.r != high.r)? normalizeMap[qRed(pixel)].r:
                    qRed(pixel);

            int g = (low.g != high.g)? normalizeMap[qGreen(pixel)].g:
                        qGreen(pixel);

            int b = (low.b != high.b)?  normalizeMap[qBlue(pixel)].b:
                        qBlue(pixel);

            oLine[x] = qRgba(r, g, b, qAlpha(pixel));
        }
    }

    auto oPacket = this->d->m_videoConverter.convert(oFrame, packet);

    if (oPacket)
        emit this->oStream(oPacket);

    return oPacket;
}

#include "moc_normalizeelement.cpp"
