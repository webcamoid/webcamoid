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
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "matrixtransformelement.h"

MatrixTransformElement::MatrixTransformElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    this->resetKernel();
}

QVariantList MatrixTransformElement::kernel() const
{
    QVariantList kernel;

    foreach (qreal e, this->m_kernel)
        kernel << e;

    return kernel;
}

void MatrixTransformElement::setKernel(const QVariantList &kernel)
{
    this->m_kernel.clear();

    foreach (QVariant e, kernel)
        this->m_kernel << e.toReal();
}

void MatrixTransformElement::resetKernel()
{
    this->m_kernel.clear();

    this->m_kernel << 1 << 0 << 0
                   << 0 << 1 << 0;
}

QbPacket MatrixTransformElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame = QImage(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    qreal det = this->m_kernel[0] * this->m_kernel[4]
                - this->m_kernel[1] * this->m_kernel[3];

    QRect rect(0, 0, src.width(), src.height());
    int cx = src.width() >> 1;
    int cy = src.height() >> 1;

    for (int i = 0, y = 0; y < src.height(); y++)
        for (int x = 0; x < src.width(); i++, x++) {
            int dx = x - cx - this->m_kernel[2];
            int dy = y - cy - this->m_kernel[5];

            int xp = cx + (dx * this->m_kernel[4] - dy * this->m_kernel[3]) / det;
            int yp = cy + (dy * this->m_kernel[0] - dx * this->m_kernel[1]) / det;

            if (rect.contains(xp, yp))
                destBits[i] = srcBits[xp + yp * src.width()];
            else
                destBits[i] = qRgba(0, 0, 0, 0);
        }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
