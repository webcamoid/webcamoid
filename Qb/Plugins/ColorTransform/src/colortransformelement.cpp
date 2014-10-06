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

#include "colortransformelement.h"

ColorTransformElement::ColorTransformElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetKernel();
}

QVariantList ColorTransformElement::kernel() const
{
    QVariantList kernel;

    foreach (float e, this->m_kernel)
        kernel << e;

    return kernel;
}

void ColorTransformElement::setKernel(const QVariantList &kernel)
{
    this->m_kernel.clear();

    foreach (QVariant e, kernel)
        this->m_kernel << e.toFloat();
}

void ColorTransformElement::resetKernel()
{
    this->m_kernel.clear();

    this->m_kernel << 1 << 0 << 0 << 0
                   << 0 << 1 << 0 << 0
                   << 0 << 0 << 1 << 0;
}

QbPacket ColorTransformElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    int videoArea = src.width() * src.height();

    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    for (int i = 0; i < videoArea; i++) {
        int r = qRed(srcBits[i]);
        int g = qGreen(srcBits[i]);
        int b = qBlue(srcBits[i]);

        int rt = r * this->m_kernel[0] + g * this->m_kernel[1] + b * this->m_kernel[2] + this->m_kernel[3];
        int gt = r * this->m_kernel[4] + g * this->m_kernel[5] + b * this->m_kernel[6] + this->m_kernel[7];
        int bt = r * this->m_kernel[8] + g * this->m_kernel[9] + b * this->m_kernel[10] + this->m_kernel[11];

        rt = qBound(0, rt, 255);
        gt = qBound(0, gt, 255);
        bt = qBound(0, bt, 255);

        destBits[i] = qRgba(rt, gt, bt, qAlpha(srcBits[i]));
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
