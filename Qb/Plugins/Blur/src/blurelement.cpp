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

#include "blurelement.h"

BlurElement::BlurElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

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

void BlurElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void BlurElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void BlurElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage image = QImage((const uchar *) packet.buffer().data(),
                           width,
                           height,
                           QImage::Format_ARGB32);

    QImage oFrame(image.size(), image.format());
    QGraphicsScene scene;
    QGraphicsPixmapItem *pixmapItem = scene.addPixmap(QPixmap::fromImage(image));
    QGraphicsBlurEffect *effect = new QGraphicsBlurEffect();
    pixmapItem->setGraphicsEffect(effect);
    effect->setBlurRadius(this->m_radius);

    QPainter painter;
    painter.begin(&oFrame);
    scene.render(&painter);
    painter.end();

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
