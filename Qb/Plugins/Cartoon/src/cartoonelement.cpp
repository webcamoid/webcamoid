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

#include "cartoonelement.h"

CartoonElement::CartoonElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetTriplevel();
    this->resetDiffspace();
}

float CartoonElement::triplevel() const
{
    return this->m_triplevel;
}

float CartoonElement::diffspace() const
{
    return this->m_diffspace / 256.0;
}

long CartoonElement::getMaxContrast(const QRgb *src, int x, int y)
{
    long max = 0;

    // Assumes PrePixelModify has been run.
    QRgb c1 = this->pixelate(src, x - this->m_diffspace, y);
    QRgb c2 = this->pixelate(src, x + this->m_diffspace, y);
    long error = this->gmError(c1, c2);

    if (error > max)
        max = error;

    c1 = this->pixelate(src, x, y - this->m_diffspace);
    c2 = this->pixelate(src, x, y + this->m_diffspace);
    error = this->gmError(c1, c2);

    if (error > max)
        max = error;

    c1 = this->pixelate(src, x - this->m_diffspace, y - this->m_diffspace);
    c2 = this->pixelate(src, x + this->m_diffspace, y + this->m_diffspace);
    error = this->gmError(c1,  c2);

    if (error > max)
        max = error;

    c1 = this->pixelate(src, x + this->m_diffspace, y - this->m_diffspace);
    c2 = this->pixelate(src, x - this->m_diffspace, y + this->m_diffspace);
    error = this->gmError(c1, c2);

    if (error > max)
        max = error;

    return max;
}

void CartoonElement::setTriplevel(float triplevel)
{
    this->m_triplevel = qBound(0.0, (double) triplevel, 1.0);
}

void CartoonElement::setDiffspace(float diffspace)
{
    this->m_diffspace = qBound(0.0, 256.0 * diffspace, 256.0);
}

void CartoonElement::resetTriplevel()
{
    this->setTriplevel(0.25);
}

void CartoonElement::resetDiffspace()
{
    this->m_diffspace = 0.99;
}

void CartoonElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void CartoonElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void CartoonElement::processFrame(const QbPacket &packet)
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
        this->m_yprecal.clear();

        for (int x = 0; x < 2 * height; x++)
            this->m_yprecal << width * x;

        this->m_caps = packet.caps();
    }

    // Cartoonify picture, do a form of edge detect
    for (int x = this->m_diffspace; x < width - (1 + this->m_diffspace); x++) {
        for (int y = this->m_diffspace; y < height - (1 + this->m_diffspace); y++) {
            int t = this->getMaxContrast(srcBits, x, y);
            int offset = x + this->m_yprecal[y];

            if (t > 1 / (1 - this->m_triplevel) - 1)
                // Make a border pixel
                destBits[offset] = qRgb(0, 0, 0);
            else
                // Copy original color
                destBits[offset] = this->flattenColor(srcBits[offset]);
        }
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
