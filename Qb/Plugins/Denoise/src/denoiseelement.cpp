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

#include "denoiseelement.h"

DenoiseElement::DenoiseElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->m_denoiseModeToStr[DenoiseModeGauss] = "gauss";
    this->m_denoiseModeToStr[DenoiseModeSelect] = "select";

    this->resetMode();
    this->resetScanSize();
    this->resetMu();
    this->resetSigma();
}

QString DenoiseElement::mode() const
{
    return this->m_denoiseModeToStr[this->m_mode];
}

QSize DenoiseElement::scanSize() const
{
    return this->m_scanSize;
}

float DenoiseElement::mu() const
{
    return this->m_mu;
}

float DenoiseElement::sigma() const
{
    return this->m_sigma;
}

void DenoiseElement::setMode(const QString &mode)
{
    if (this->m_denoiseModeToStr.values().contains(mode))
        this->m_mode = this->m_denoiseModeToStr.key(mode);
    else
        this->m_mode = DenoiseModeGauss;
}

void DenoiseElement::setScanSize(const QSize &scanSize)
{
    this->m_scanSize = scanSize;
}

void DenoiseElement::setMu(float mu)
{
    this->m_mu = mu;
}

void DenoiseElement::setSigma(float sigma)
{
    this->m_sigma = sigma;
}

void DenoiseElement::resetMode()
{
    this->setMode("gauss");
}

void DenoiseElement::resetScanSize()
{
    this->setScanSize(QSize(1, 1));
}

void DenoiseElement::resetMu()
{
    this->setMu(1.0);
}

void DenoiseElement::resetSigma()
{
    this->setSigma(1.0);
}

void DenoiseElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void DenoiseElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void DenoiseElement::processFrame(const QbPacket &packet)
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

    int scanWidth = 2 * this->m_scanSize.width() + 1;
    int scanHeight = 2 * this->m_scanSize.height() + 1;

    for (int y = 0; y < height; y++) {
        int xOffset = y * width;
        int yMin = y - this->m_scanSize.height();
        int yMax = y + this->m_scanSize.height();
        int kernelHeight = scanHeight;

        if (yMin < 0) {
            kernelHeight -= abs(yMin);
            yMin = 0;
        }

        if (yMax >= height)
            kernelHeight -= abs(yMax - height + 1);

        for (int x = 0; x < width; x++) {
            int xMin = x - this->m_scanSize.width();
            int xMax = x + this->m_scanSize.width();
            int kernelWidth = scanWidth;

            if (xMin < 0) {
                kernelWidth -= abs(xMin);
                xMin = 0;
            }

            if (xMax >= width)
                kernelWidth -= abs(xMax - width + 1);

            QRect rect(xMin, yMin, kernelWidth, kernelHeight);

            QRgb pixel;

            if (this->m_mode == DenoiseModeGauss)
                pixel = this->averageColor(srcBits,
                                           width,
                                           rect,
                                           this->m_mu,
                                           this->m_sigma);
            else
                pixel = this->selectAverageColor(srcBits,
                                                 width,
                                                 rect);

            destBits[x + xOffset] = pixel;
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
