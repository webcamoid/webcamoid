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

#include "colorfilterelement.h"

ColorFilterElement::ColorFilterElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    qRegisterMetaType<QRgb>("QRgb");

    this->resetColor();
    this->resetRadius();
    this->resetGradient();
}

QRgb ColorFilterElement::color() const
{
    return this->m_color;
}

float ColorFilterElement::radius() const
{
    return this->m_radius;
}

bool ColorFilterElement::gradient() const
{
    return this->m_gradient;
}

void ColorFilterElement::setColor(QRgb color)
{
    this->m_color = color;
}

void ColorFilterElement::setRadius(float radius)
{
    this->m_radius = radius;
}

void ColorFilterElement::setGradient(bool gradient)
{
    this->m_gradient = gradient;
}

void ColorFilterElement::resetColor()
{
    this->m_color = qRgb(0, 0, 0);
}

void ColorFilterElement::resetRadius()
{
    this->setRadius(1.0);
}

void ColorFilterElement::resetGradient()
{
    this->setGradient(false);
}

void ColorFilterElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void ColorFilterElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void ColorFilterElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_ARGB32);

    int videoArea = width * height;

    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    for (int i = 0; i < videoArea; i++) {
        int r = qRed(srcBits[i]);
        int g = qGreen(srcBits[i]);
        int b = qBlue(srcBits[i]);

        int rf = qRed(this->m_color);
        int gf = qGreen(this->m_color);
        int bf = qBlue(this->m_color);

        int rd = r - rf;
        int gd = g - gf;
        int bd = b - bf;

        float k = sqrt(rd * rd + gd * gd + bd * bd);

        if (k <= this->m_radius) {
            if (this->m_gradient) {
                float p = k / this->m_radius;

                int gray = qGray(srcBits[i]);

                r = p * (gray - r) + r;
                g = p * (gray - g) + g;
                b = p * (gray - b) + b;

                destBits[i] = qRgba(r, g, b, qAlpha(srcBits[i]));
            }
            else
                destBits[i] = srcBits[i];
        }
        else {
            int gray = qGray(srcBits[i]);
            destBits[i] = qRgba(gray, gray, gray, qAlpha(srcBits[i]));
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
