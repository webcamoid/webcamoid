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

#include "quarkelement.h"

QuarkElement::QuarkElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetPlanes();
    this->m_plane = 0;
}

int QuarkElement::planes() const
{
    return this->m_planes;
}

void QuarkElement::setPlanes(int planes)
{
    this->m_planes = planes;
}

void QuarkElement::resetPlanes()
{
    this->setPlanes(16);
}

void QuarkElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void QuarkElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void QuarkElement::processFrame(const QbPacket &packet)
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


    if (packet.caps() != this->m_caps) {
        this->m_buffer = QImage(width, src.height() * this->m_planes, QImage::Format_RGB32);
        this->m_planeTable.resize(this->m_planes);

        for(int i = 0; i < this->m_planes; i++)
            this->m_planeTable[i] = (QRgb *) this->m_buffer.bits() + videoArea * i;

        this->m_plane = this->m_planes - 1;

        this->m_caps = packet.caps();
    }

    memcpy(this->m_planeTable[this->m_plane], srcBits, src.bytesPerLine() * height);

    for (int i = 0; i < videoArea; i++) {
        int cf = (rand() >> 24) & (this->m_planes - 1);
        destBits[i] = this->m_planeTable[cf][i];
    }

    this->m_plane--;

    if (this->m_plane < 0)
        this->m_plane = this->m_planes - 1;

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
