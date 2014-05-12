/* Webcamod, webcam capture plasmoid.
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

#include "waveelement.h"

WaveElement::WaveElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetAmplitude();
    this->resetPhases();
    this->resetBackground();
}

float WaveElement::amplitude() const
{
    return this->m_amplitude;
}

float WaveElement::phases() const
{
    return this->m_phases;
}

QRgb WaveElement::background() const
{
    return this->m_background;
}

QImage WaveElement::wave(const QImage &img, float amplitude, float phases, QRgb background) const
{
    QImage buffer(img.width(),
                  img.height() + 2 * abs(amplitude),
                  img.format());

    int width = buffer.width();
    int height = buffer.height();

    float *sine_map = new float[width];

    for(int x = 0; x < width; x++)
        sine_map[x] = abs(amplitude) + amplitude * sin((phases * 2.0 * M_PI * x) / width);

    for (int y = 0; y < height; y++) {
        QRgb *dest = (QRgb *) buffer.scanLine(y);

        for (int x = 0; x < width; x++)
            *dest++ = this->interpolateBackground(img, x, y - sine_map[x], background);
    }

    delete sine_map;

    return buffer;
}

void WaveElement::setAmplitude(float amplitude)
{
    this->m_amplitude = amplitude;
}

void WaveElement::setPhases(float phases)
{
    this->m_phases = phases;
}

void WaveElement::setBackground(QRgb background)
{
    this->m_background = background;
}

void WaveElement::resetAmplitude()
{
    this->setAmplitude(1);
}

void WaveElement::resetPhases()
{
    this->setPhases(0);
}

void WaveElement::resetBackground()
{
    this->setBackground(0);
}

void WaveElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void WaveElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void WaveElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_ARGB32);

    QImage oFrame = this->wave(src,
                               this->m_amplitude,
                               this->m_phases,
                               this->m_background);

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
    oPacket.setDuration(packet.duration());
    oPacket.setTimeBase(packet.timeBase());
    oPacket.setIndex(packet.index());

    emit this->oStream(oPacket);
}
