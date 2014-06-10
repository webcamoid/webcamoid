/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "equalizeelement.h"
#include "pixelstructs.h"

EqualizeElement::EqualizeElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));
}

void EqualizeElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void EqualizeElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void EqualizeElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage oFrame = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_ARGB32);

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

        uchar r = (low.red != high.red) ? equalizeMap[qRed(pixel)].red :
                qRed(pixel);

        uchar g = (low.green != high.green) ? equalizeMap[qGreen(pixel)].green :
                    qGreen(pixel);

        uchar b = (low.blue != high.blue) ?  equalizeMap[qBlue(pixel)].blue :
                    qBlue(pixel);

        *dest++ = qRgba(r, g, b, qAlpha(pixel));
    }

    QbBufferPtr oBuffer(new char[oFrame.byteCount()]);
    memcpy(oBuffer.data(), oFrame.constBits(), oFrame.byteCount());

    QbCaps caps(packet.caps());
    caps.setProperty("format", "bgra");
    caps.setProperty("width", oFrame.width());
    caps.setProperty("height", oFrame.height());

    QbPacket oPacket(caps,
                     oBuffer,
                     oFrame.byteCount());

    oPacket.setPts(packet.pts());
    oPacket.setTimeBase(packet.timeBase());
    oPacket.setIndex(packet.index());

    emit this->oStream(oPacket);
}
