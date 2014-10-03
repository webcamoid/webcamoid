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

#include "blurelement.h"

BlurElement::BlurElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetRadius();
}

int BlurElement::radius() const
{
    return this->m_radius;
}

void BlurElement::setRadius(int radius)
{
    this->m_radius = radius;
}

void BlurElement::resetRadius()
{
    this->setRadius(1);
}

QbPacket BlurElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame(src.size(), src.format());
    QGraphicsScene scene;
    QGraphicsPixmapItem *pixmapItem = scene.addPixmap(QPixmap::fromImage(src));
    QGraphicsBlurEffect *effect = new QGraphicsBlurEffect();
    pixmapItem->setGraphicsEffect(effect);
    effect->setBlurRadius(this->m_radius);

    QPainter painter;
    painter.begin(&oFrame);
    scene.render(&painter);
    painter.end();

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
