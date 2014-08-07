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

#include "implodeelement.h"

ImplodeElement::ImplodeElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetAmount();
}

float ImplodeElement::amount() const
{
    return this->m_amount;
}

QImage ImplodeElement::implode(const QImage &img, float amount) const
{
    int width = img.width();
    int height = img.height();

    QImage buffer(img.size(), img.format());

    float xScale = 1.0;
    float yScale = 1.0;
    float xCenter = 0.5 * width;
    float yCenter = 0.5 * height;
    float radius = xCenter;

    if (width > height)
        yScale = (float) width / height;
    else if (width < height) {
        xScale = (float) height / width;
        radius = yCenter;
    }

    for (int y = 0; y < height; y++) {
        QRgb *src = (QRgb *) img.scanLine(y);
        QRgb *dest = (QRgb *) buffer.scanLine(y);
        float yDistance = yScale * (y - yCenter);

        for (int x = 0; x < width; x++) {
            float xDistance = xScale * (x - xCenter);
            float distance = xDistance * xDistance + yDistance * yDistance;

            if (distance >= (radius * radius))
                *dest = src[x];
            else {
                float factor = 1.0;

                if(distance > 0.0)
                    factor = pow(sin((M_PI) * sqrt(distance) / radius / 2), -amount);

                *dest = this->interpolate(img, factor * xDistance / xScale + xCenter,
                                               factor * yDistance / yScale + yCenter);
            }

            dest++;
        }
    }

    return buffer;
}

void ImplodeElement::setAmount(float amount)
{
    this->m_amount = amount;
}

void ImplodeElement::resetAmount()
{
    this->setAmount(1);
}

void ImplodeElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void ImplodeElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void ImplodeElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_ARGB32);

    QImage oFrame = this->implode(src, this->m_amount);

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
