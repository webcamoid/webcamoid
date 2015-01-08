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

#include "waveelement.h"

WaveElement::WaveElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetAmplitude();
    this->resetPhases();
    this->resetBackground();
}

qreal WaveElement::amplitude() const
{
    return this->m_amplitude;
}

qreal WaveElement::phases() const
{
    return this->m_phases;
}

QRgb WaveElement::background() const
{
    return this->m_background;
}

void WaveElement::setAmplitude(qreal amplitude)
{
    this->m_amplitude = amplitude;
}

void WaveElement::setPhases(qreal phases)
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

QbPacket WaveElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame(src.width(),
                  src.height() + 2 * fabs(this->m_amplitude),
                  src.format());

    qreal sineMap[oFrame.width()];

    for (int x = 0; x < oFrame.width(); x++)
        sineMap[x] = fabs(this->m_amplitude)
                      + this->m_amplitude
                      * sin((this->m_phases * 2.0 * M_PI * x) / oFrame.width());

    for (int y = 0; y < oFrame.height(); y++) {
        QRgb *dest = (QRgb *) oFrame.scanLine(y);

        for (int x = 0; x < oFrame.width(); x++)
            *dest++ = this->interpolateBackground(src,
                                                  x,
                                                  y - sineMap[x],
                                                  this->m_background);
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
