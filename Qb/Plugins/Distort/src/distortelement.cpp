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

#include "distortelement.h"

DistortElement::DistortElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetAmplitude();
    this->resetFrequency();
    this->resetGridSizeLog();
}

float DistortElement::amplitude() const
{
    return this->m_amplitude;
}

float DistortElement::frequency() const
{
    return this->m_frequency;
}

int DistortElement::gridSizeLog() const
{
    return this->m_gridSizeLog;
}

QVector<QPoint> DistortElement::createGrid(int width, int height, int gridSize, float time)
{
    QVector<QPoint> grid;

    for (int y = 0; y <= height; y += gridSize)
        for (int x = 0; x <= width; x += gridSize)
            grid << this->plasmaFunction(QPoint(x, y), QSize(width, height),
                                         this->m_amplitude, this->m_frequency,
                                         time);

    return grid;
}

void DistortElement::setAmplitude(float amplitude)
{
    this->m_amplitude = amplitude;
}

void DistortElement::setFrequency(float frequency)
{
    this->m_frequency = frequency;
}

void DistortElement::setGridSizeLog(int gridSizeLog)
{
    this->m_gridSizeLog = gridSizeLog;
}

void DistortElement::resetAmplitude()
{
    this->setAmplitude(1.0);
}

void DistortElement::resetFrequency()
{
    this->setFrequency(1.0);
}

void DistortElement::resetGridSizeLog()
{
    this->setGridSizeLog(1);
}

void DistortElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void DistortElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void DistortElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_ARGB32);

    QImage oFrame = QImage(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    int gridSize = 1 << this->m_gridSizeLog;
    float time = packet.pts() * packet.timeBase().value();
    QVector<QPoint> grid = this->createGrid(width, height, gridSize, time);

    int gridX = width / gridSize;
    int gridY = height / gridSize;

    for (int y = 0; y < gridY; y++)
        for (int x = 0; x < gridX; x++) {
            int offset = x + y * (gridX + 1);

            QPoint upperLeft  = grid[offset];
            QPoint lowerLeft  = grid[offset + gridX + 1];
            QPoint upperRight = grid[offset + 1];
            QPoint lowerRight = grid[offset + gridX + 2];

            int startColXX = upperLeft.x();
            int startColYY = upperLeft.y();
            int endColXX = upperRight.x();
            int endColYY = upperRight.y();

            int stepStartColX = (lowerLeft.x() - upperLeft.x())
                                >> this->m_gridSizeLog;

            int stepStartColY = (lowerLeft.y() - upperLeft.y())
                                >> this->m_gridSizeLog;

            int stepEndColX = (lowerRight.x() - upperRight.x())
                              >> this->m_gridSizeLog;

            int stepEndColY = (lowerRight.y() - upperRight.y())
                              >> this->m_gridSizeLog;

            int pos = (y << this->m_gridSizeLog) * width + (x << this->m_gridSizeLog);

            for (int blockY = 0; blockY < gridSize; blockY++) {
                int xLineIndex = startColXX;
                int yLineIndex = startColYY;

                int stepLineX = (endColXX - startColXX) >> this->m_gridSizeLog;
                int stepLineY = (endColYY - startColYY) >> this->m_gridSizeLog;

                for (int i = 0, blockX = 0; blockX < gridSize; i++, blockX++) {
                    int xx = qBound(0, xLineIndex, width - 1);
                    int yy = qBound(0, yLineIndex, height - 1);

                    xLineIndex += stepLineX;
                    yLineIndex += stepLineY;

                    destBits[pos + i] = srcBits[xx + yy * width];
                }

                startColXX += stepStartColX;
                endColXX   += stepEndColX;
                startColYY += stepStartColY;
                endColYY   += stepEndColY;

                pos += width - gridSize + gridSize;
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
