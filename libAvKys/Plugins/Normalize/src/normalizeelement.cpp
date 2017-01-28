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

#include "normalizeelement.h"
#include "pixelstructs.h"

NormalizeElement::NormalizeElement(): AkElement()
{
}

AkPacket NormalizeElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    QImage oFrame = src.convertToFormat(QImage::Format_ARGB32);

    // form histogram
    QVector<HistogramListItem> histogram(256, HistogramListItem());

    for (int y = 0; y < oFrame.height(); y++) {
        const QRgb *dstLine = reinterpret_cast<const QRgb *>(oFrame.constScanLine(y));

        for (int x = 0; x < oFrame.width(); x++) {
            QRgb pixel = dstLine[x];
            histogram[qRed(pixel)].red++;
            histogram[qGreen(pixel)].green++;
            histogram[qBlue(pixel)].blue++;
            histogram[qAlpha(pixel)].alpha++;
        }
    }

    // find the histogram boundaries by locating the .01 percent levels.
    ShortPixel high, low;
    qint32 thresholdIntensity = qint32(oFrame.width() * oFrame.height() / 1e3);
    IntegerPixel intensity;

    for (low.red = 0; low.red < 256; low.red++) {
        intensity.red += histogram[low.red].red;

        if (intensity.red > thresholdIntensity)
            break;
    }

    intensity.clear();

    for (high.red = 255; high.red > 0; high.red--) {
        intensity.red += histogram[high.red].red;

        if (intensity.red > thresholdIntensity)
            break;
    }

    intensity.clear();

    for (low.green = low.red; low.green < high.red; low.green++) {
        intensity.green += histogram[low.green].green;

        if (intensity.green > thresholdIntensity)
            break;
    }

    intensity.clear();

    for (high.green = high.red; high.green != low.red; high.green--) {
        intensity.green += histogram[high.green].green;

        if (intensity.green > thresholdIntensity)
            break;
    }

    intensity.clear();

    for (low.blue = low.green; low.blue < high.green; low.blue++) {
        intensity.blue += histogram[low.blue].blue;

        if (intensity.blue > thresholdIntensity)
            break;
    }

    intensity.clear();

    for (high.blue = high.green; high.blue != low.green; high.blue--) {
        intensity.blue += histogram[high.blue].blue;

        if (intensity.blue > thresholdIntensity)
            break;
    }

    // stretch the histogram to create the normalized image mapping.
    QVector<IntegerPixel> normalizeMap(256);

    for (int i = 0; i < 256; i++) {
        if(i < low.red)
            normalizeMap[i].red = 0;
        else {
            if (i > high.red)
                normalizeMap[i].red = 255;
            else if (low.red != high.red)
                normalizeMap[i].red = (255 * (i - low.red)) /
                    (high.red - low.red);
        }

        if (i < low.green)
            normalizeMap[i].green = 0;
        else {
            if(i > high.green)
                normalizeMap[i].green = 255;
            else if(low.green != high.green)
                normalizeMap[i].green = (255 * (i - low.green)) /
                    (high.green - low.green);
        }

        if (i < low.blue)
            normalizeMap[i].blue = 0;
        else {
            if (i > high.blue)
                normalizeMap[i].blue = 255;
            else if (low.blue != high.blue)
                normalizeMap[i].blue = (255*(i-low.blue)) /
                    (high.blue - low.blue);
        }
    }

    // write
    for (int y = 0; y < oFrame.height(); y++) {
        QRgb *oLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < oFrame.width(); x++) {
            QRgb pixel = oLine[x];

            int r = (low.red != high.red)? normalizeMap[qRed(pixel)].red:
                    qRed(pixel);

            int g = (low.green != high.green)? normalizeMap[qGreen(pixel)].green:
                        qGreen(pixel);

            int b = (low.blue != high.blue)?  normalizeMap[qBlue(pixel)].blue:
                        qBlue(pixel);

            oLine[x] = qRgba(r, g, b, qAlpha(pixel));
        }
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
