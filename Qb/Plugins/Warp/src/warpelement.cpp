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

#include "warpelement.h"

WarpElement::WarpElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetRipples();
}

float WarpElement::ripples() const
{
    return this->m_ripples;
}

void WarpElement::setRipples(float ripples)
{
    this->m_ripples = ripples;
}

void WarpElement::resetRipples()
{
    this->setRipples(4);
}

void WarpElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void WarpElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void WarpElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_ARGB32);

    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    if (packet.caps() != this->m_caps) {
        int cx = width >> 1;
        int cy = height >> 1;

        float k = 2.0 * M_PI / sqrt(cx * cx + cy * cy);

        this->m_phiTable.clear();

        for (int y = -cy; y < cy; y++)
            for (int x = -cx; x < cx; x++)
                this->m_phiTable << k * sqrt(x * x + y * y);

        this->m_caps = packet.caps();
    }

    static int tval = 0;

    float dx = 30 * sin((tval + 100) * M_PI / 128)
               + 40 * sin((tval - 10) * M_PI / 512);

    float dy = -35 * sin(tval * M_PI / 256)
               + 40 * sin((tval + 30) * M_PI / 512);

    float ripples = this->m_ripples * sin((tval - 70) * M_PI / 64);

    tval = (tval + 1) & 511;
    float *phiTable = this->m_phiTable.data();

    for (int i = 0, y = 0; y < height; y++)
        for (int x = 0; x < width; i++, x++) {
            float phi = ripples * phiTable[i];

            int xOrig = dx * cos(phi) + x;
            int yOrig = dy * sin(phi) + y;

            xOrig = qBound(0, xOrig, width);
            yOrig = qBound(0, yOrig, height);

            destBits[i] = srcBits[xOrig + width * yOrig];
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
