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

#include "equalizeelement.h"
#include "pixelstructs.h"

EqualizeElement::EqualizeElement(): AkElement()
{
}

AkPacket EqualizeElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    QImage oFrame = src.convertToFormat(QImage::Format_ARGB32);

    // form histogram
    QVector<HistogramListItem> histogram(256, HistogramListItem());

    QRgb *dest = (QRgb *) oFrame.bits();
    int count = oFrame.width() * oFrame.height();

    for (int i = 0; i < count; i++) {
        QRgb pixel = *dest++;
        histogram[qRed(pixel)].red++;
        histogram[qGreen(pixel)].green++;
        histogram[qBlue(pixel)].blue++;
        histogram[qAlpha(pixel)].alpha++;
    }

    // integrate the histogram to get the equalization map
    IntegerPixel intensity;
    QVector<IntegerPixel> map(256);

    for (int i = 0; i < 256; i++) {
        intensity.red += histogram[i].red;
        intensity.green += histogram[i].green;
        intensity.blue += histogram[i].blue;
        map[i] = intensity;
    }

    // stretch the histogram to create the equalized image mapping.
    IntegerPixel low = map[0];
    IntegerPixel high = map[255];
    QVector<CharPixel> equalizeMap(256, CharPixel());

    for (int i = 0; i < 256; i++) {
        if (high.red != low.red)
            equalizeMap[i].red = (uchar)
                ((255 * (map[i].red - low.red)) / (high.red - low.red));

        if (high.green != low.green)
            equalizeMap[i].green = (uchar)
                ((255 * (map[i].green - low.green)) / (high.green - low.green));

        if (high.blue != low.blue)
            equalizeMap[i].blue = (uchar)
                ((255 * (map[i].blue - low.blue)) / (high.blue - low.blue));

    }

    // write
    dest = (QRgb *) oFrame.bits();

    for (int i = 0; i < count; i++){
        QRgb pixel = *dest;

        uchar r = (low.red != high.red)?
                      equalizeMap[qRed(pixel)].red:
                      qRed(pixel);

        uchar g = (low.green != high.green)?
                      equalizeMap[qGreen(pixel)].green:
                      qGreen(pixel);

        uchar b = (low.blue != high.blue)?
                      equalizeMap[qBlue(pixel)].blue:
                      qBlue(pixel);

        *dest++ = qRgba(r, g, b, qAlpha(pixel));
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
