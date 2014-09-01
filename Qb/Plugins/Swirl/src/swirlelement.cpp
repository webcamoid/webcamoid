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

#include "swirlelement.h"

SwirlElement::SwirlElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetDegrees();
}

float SwirlElement::degrees() const
{
    return this->m_degrees;
}

QImage SwirlElement::swirl(const QImage &img, float degrees) const
{
    int width = img.width();
    int height = img.height();

    QImage buffer(img.size(), img.format());

    float xScale = 1.0;
    float yScale = 1.0;
    float xCenter = 0.5 * width;
    float yCenter = 0.5 * height;
    float radius = qMax(xCenter, yCenter);

    if (width > height)
        yScale = (float) width / height;
    else if (width < height)
        xScale = (float) height / width;

    degrees = M_PI * degrees / 180.0;

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
                float factor = 1.0 - sqrt(distance) / radius;
                float sine = sin(degrees * factor * factor);
                float cosine = cos(degrees * factor * factor);

                *dest = this->interpolate(img,
                                          (cosine * xDistance - sine * yDistance) / xScale + xCenter,
                                          (sine * xDistance + cosine * yDistance) / yScale + yCenter);
            }

            dest++;
        }
    }

    return buffer;
}

void SwirlElement::setDegrees(float degrees)
{
    this->m_degrees = degrees;
}

void SwirlElement::resetDegrees()
{
    this->setDegrees(0);
}

void SwirlElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void SwirlElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void SwirlElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_ARGB32);

    QImage oFrame = this->swirl(src, this->m_degrees);

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
