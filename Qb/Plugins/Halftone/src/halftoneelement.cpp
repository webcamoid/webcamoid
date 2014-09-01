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

#include "halftoneelement.h"

HalftoneElement::HalftoneElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetPattern();
    this->resetPatternSize();
    this->resetLightness();
    this->resetSlope();
    this->resetIntercept();
}

QString HalftoneElement::pattern() const
{
    return this->m_pattern;
}

QSize HalftoneElement::patternSize() const
{
    return this->m_patternSize;
}

float HalftoneElement::lightness() const
{
    return this->m_lightness;
}

float HalftoneElement::slope() const
{
    return this->m_slope;
}

float HalftoneElement::intercept() const
{
    return this->m_intercept;
}

void HalftoneElement::setPattern(const QString &pattern)
{
    this->m_pattern = pattern;
}

void HalftoneElement::setPatternSize(const QSize &patternSize)
{
    this->m_patternSize = patternSize;
}

void HalftoneElement::setLightness(float lightness)
{
    this->m_lightness = lightness;
}

void HalftoneElement::setSlope(float slope)
{
    this->m_slope = slope;
}

void HalftoneElement::setIntercept(float intercept)
{
    this->m_intercept = intercept;
}

void HalftoneElement::resetPattern()
{
    this->setPattern(":/Halftone/share/ditherCluster8Matrix.bmp");
}

void HalftoneElement::resetPatternSize()
{
    this->setPatternSize(QSize());
}

void HalftoneElement::resetLightness()
{
    this->setLightness(0.5);
}

void HalftoneElement::resetSlope()
{
    this->setSlope(1.0);
}

void HalftoneElement::resetIntercept()
{
    this->setIntercept(0.0);
}

void HalftoneElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void HalftoneElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void HalftoneElement::processFrame(const QbPacket &packet)
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
        this->m_patternImage = QImage(this->m_pattern);

        if (!this->m_patternSize.isEmpty())
            this->m_patternImage = this->m_patternImage.scaled(this->m_patternSize);

        this->m_patternBits = (quint8 *) this->m_patternImage.bits();
        this->m_caps = packet.caps();
    }

    // filter image
    for (int i = 0, y = 0; y < height; y++)
        for (int x = 0; x < width; i++, x++) {
            int col = x % this->m_patternImage.width();
            int row = y % this->m_patternImage.height();

            int gray = qGray(srcBits[i]);
            int threshold = this->m_patternBits[row * this->m_patternImage.width() + col];
            threshold = this->m_slope * threshold + this->m_intercept;
            threshold = qBound(0, threshold, 255);

            if (gray > threshold)
                destBits[i] = srcBits[i];
            else {
                QColor color(srcBits[i]);

                color.setHsl(color.hue(),
                             color.saturation(),
                             this->m_lightness * color.lightness(),
                             color.alpha());

                destBits[i] = color.rgba();
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
