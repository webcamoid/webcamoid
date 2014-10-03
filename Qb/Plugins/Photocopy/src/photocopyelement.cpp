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

#include "photocopyelement.h"

PhotocopyElement::PhotocopyElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetBrightness();
    this->resetSharpness();
}

float PhotocopyElement::brightness() const
{
    return this->m_brightness;
}

float PhotocopyElement::sharpness() const
{
    return this->m_sharpness;
}

int PhotocopyElement::sigmoidalRange() const
{
    return this->m_sigmoidalRange;
}

int PhotocopyElement::sigmoidalBase() const
{
    return this->m_sigmoidalBase;
}

void PhotocopyElement::setBrightness(float brightness)
{
    this->m_brightness = brightness;
}

void PhotocopyElement::setSharpness(float sharpness)
{
    this->m_sharpness = sharpness;
}

void PhotocopyElement::setSigmoidalBase(int sigmoidalBase)
{
    this->m_sigmoidalBase = sigmoidalBase;
}

void PhotocopyElement::setSigmoidalRange(int sigmoidalRange)
{
    this->m_sigmoidalRange = sigmoidalRange;
}

void PhotocopyElement::resetBrightness()
{
    this->setBrightness(0.75);
}

void PhotocopyElement::resetSharpness()
{
    this->setSharpness(0.85);
}

void PhotocopyElement::resetSigmoidalBase()
{
    this->setSigmoidalBase(2);
}

void PhotocopyElement::resetSigmoidalRange()
{
    this->setSigmoidalRange(20);
}

QbPacket PhotocopyElement::iStream(const QbPacket &packet)
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

        //desaturate
        int luma = this->rgbToLuma(r, g, b);

        //compute sigmoidal transfer
        float val = luma / 255.0;
        val = 255.0 / (1 + exp((this->m_sigmoidalBase + this->m_sharpness * this->m_sigmoidalRange) * (0.5 - val)));
        val = val * this->m_brightness;
        luma = qBound(0, (int) val, 255);

        destBits[i] = qRgba(luma, luma, luma, qAlpha(srcBits[i]));
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
