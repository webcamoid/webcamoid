/* Webcamoid, webcam capture application.
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

#include "oilpaintelement.h"
#include "defs.h"

OilPaintElement::OilPaintElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetRadius();
}

int OilPaintElement::radius() const
{
    return this->m_radius;
}

QImage OilPaintElement::oilPaint(const QImage &src, int radius) const
{
    QImage dst(src.size(), src.format());
    quint16 histogram[256];
    QRgb *oLine = (QRgb *) dst.bits();
    int scanBlockLen = (radius << 1) + 1;
    QRgb *scanBlock[scanBlockLen];

    for (int y = 0; y < src.height(); y++) {
        int pos = y - radius;

        for (int j = 0; j < scanBlockLen; j++) {
            scanBlock[j] = (QRgb *) src.constScanLine(qBound(0, pos, src.height()));
            pos++;
        }

        for (int x = 0; x < src.width(); x++) {
            int minI = x - radius;
            int maxI = x + radius + 1;

            if (minI < 0)
                minI = 0;

            if (maxI > src.width())
                maxI = src.width();

            memset(histogram, 0, 512);
            quint16 max = 0;

            for (int j = 0; j < scanBlockLen; j++)
                for (int i = minI; i < maxI; i++) {
                    Pixel *p = (Pixel *) &scanBlock[j][i];
                    quint16 value = ++histogram[(p->r + p->g + p->b) / 3];

                    if (value > max) {
                        max = value;
                        *oLine = scanBlock[j][i];
                    }
                }

            oLine++;
        }
    }

    return dst;
}

void OilPaintElement::setRadius(int radius)
{
    this->m_radius = radius;
}

void OilPaintElement::resetRadius()
{
    this->setRadius(1);
}

void OilPaintElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void OilPaintElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void OilPaintElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage img = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_RGB32);

    QImage oFrame = this->oilPaint(img, this->m_radius);

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
