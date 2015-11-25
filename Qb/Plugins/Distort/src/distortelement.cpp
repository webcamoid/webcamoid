/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "distortelement.h"

DistortElement::DistortElement(): QbElement()
{
    this->m_amplitude = 1.0;
    this->m_frequency = 1.0;
    this->m_gridSizeLog = 1;
}

QObject *DistortElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Distort/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Distort", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);

    if (!item) {
        delete context;

        return NULL;
    }

    context->setParent(item);

    return item;
}

qreal DistortElement::amplitude() const
{
    return this->m_amplitude;
}

qreal DistortElement::frequency() const
{
    return this->m_frequency;
}

int DistortElement::gridSizeLog() const
{
    return this->m_gridSizeLog;
}

QVector<QPoint> DistortElement::createGrid(int width, int height,
                                           int gridSize, qreal time)
{
    QVector<QPoint> grid;

    for (int y = 0; y <= height; y += gridSize)
        for (int x = 0; x <= width; x += gridSize)
            grid << this->plasmaFunction(QPoint(x, y), QSize(width, height),
                                         this->m_amplitude, this->m_frequency,
                                         time);

    return grid;
}

void DistortElement::setAmplitude(qreal amplitude)
{
    if (this->m_amplitude == amplitude)
        return;

    this->m_amplitude = amplitude;
    emit this->amplitudeChanged(amplitude);
}

void DistortElement::setFrequency(qreal frequency)
{
    if (this->m_frequency == frequency)
        return;

    this->m_frequency = frequency;
    emit this->frequencyChanged(frequency);
}

void DistortElement::setGridSizeLog(int gridSizeLog)
{
    if (this->m_gridSizeLog == gridSizeLog)
        return;

    this->m_gridSizeLog = gridSizeLog;
    emit this->gridSizeLogChanged(gridSizeLog);
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

QbPacket DistortElement::iStream(const QbPacket &packet)
{
    QImage src = QbUtils::packetToImage(packet);

    if (src.isNull())
        return QbPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame = QImage(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    int gridSizeLog = this->m_gridSizeLog > 0? this->m_gridSizeLog: 1;
    int gridSize = 1 << gridSizeLog;
    qreal time = packet.pts() * packet.timeBase().value();
    QVector<QPoint> grid = this->createGrid(src.width(), src.height(), gridSize, time);

    int gridX = src.width() / gridSize;
    int gridY = src.height() / gridSize;

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
                                >> gridSizeLog;

            int stepStartColY = (lowerLeft.y() - upperLeft.y())
                                >> gridSizeLog;

            int stepEndColX = (lowerRight.x() - upperRight.x())
                              >> gridSizeLog;

            int stepEndColY = (lowerRight.y() - upperRight.y())
                              >> gridSizeLog;

            int pos = (y << gridSizeLog) * src.width() + (x << gridSizeLog);

            for (int blockY = 0; blockY < gridSize; blockY++) {
                int xLineIndex = startColXX;
                int yLineIndex = startColYY;

                int stepLineX = (endColXX - startColXX) >> gridSizeLog;
                int stepLineY = (endColYY - startColYY) >> gridSizeLog;

                for (int i = 0, blockX = 0; blockX < gridSize; i++, blockX++) {
                    int xx = qBound(0, xLineIndex, src.width() - 1);
                    int yy = qBound(0, yLineIndex, src.height() - 1);

                    xLineIndex += stepLineX;
                    yLineIndex += stepLineY;

                    destBits[pos + i] = srcBits[xx + yy * src.width()];
                }

                startColXX += stepStartColX;
                endColXX   += stepEndColX;
                startColYY += stepStartColY;
                endColYY   += stepEndColY;

                pos += src.width() - gridSize + gridSize;
            }
        }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, packet);
    qbSend(oPacket)
}
