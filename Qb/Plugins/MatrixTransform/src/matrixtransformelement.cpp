/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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

#include "matrixtransformelement.h"

MatrixTransformElement::MatrixTransformElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetKernel();
}

QVariantList MatrixTransformElement::kernel() const
{
    QVariantList kernel;

    foreach (float e, this->m_kernel)
        kernel << e;

    return kernel;
}

void MatrixTransformElement::setKernel(const QVariantList &kernel)
{
    this->m_kernel.clear();

    foreach (QVariant e, kernel)
        this->m_kernel << e.toFloat();
}

void MatrixTransformElement::resetKernel()
{
    this->m_kernel.clear();

    this->m_kernel << 1 << 0 << 0
                   << 0 << 1 << 0;
}

void MatrixTransformElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void MatrixTransformElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void MatrixTransformElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_RGB32);

    QImage oFrame = QImage(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    float det = this->m_kernel[0] * this->m_kernel[4]
                - this->m_kernel[1] * this->m_kernel[3];

    QRect rect(0, 0, width, height);
    int cx = width >> 1;
    int cy = height >> 1;

    for (int i = 0, y = 0; y < height; y++)
        for (int x = 0; x < width; i++, x++) {
            int dx = x - cx - this->m_kernel[2];
            int dy = y - cy - this->m_kernel[5];

            int xp = cx + (dx * this->m_kernel[4] - dy * this->m_kernel[3]) / det;
            int yp = cy + (dy * this->m_kernel[0] - dx * this->m_kernel[1]) / det;

            if (rect.contains(xp, yp))
                destBits[i] = srcBits[xp + yp * width];
            else
                destBits[i] = qRgba(0, 0, 0, 0);
        }

    QbBufferPtr oBuffer(new char[oFrame.byteCount()]);
    memcpy(oBuffer.data(), oFrame.constBits(), oFrame.byteCount());

    QbCaps caps(packet.caps());
    caps.setProperty("format", "bgr0");
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
