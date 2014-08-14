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

#include "streakelement.h"

StreakElement::StreakElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetPlanes();
    this->resetStride();
    this->resetStrideMask();
    this->resetStrideShift();
}

int StreakElement::planes() const
{
    return this->m_planes;
}

int StreakElement::stride() const
{
    return this->m_stride;
}

quint32 StreakElement::strideMask() const
{
    return this->m_strideMask;
}

int StreakElement::strideShift() const
{
    return this->m_strideShift;
}

void StreakElement::setPlanes(int planes)
{
    this->m_planes = planes;
}

void StreakElement::setStride(int stride)
{
    this->m_stride = stride;
}

void StreakElement::setStrideMask(quint32 strideMask)
{
    this->m_strideMask = strideMask;
}

void StreakElement::setStrideShift(int strideShift)
{
    this->m_strideShift = strideShift;
}

void StreakElement::resetPlanes()
{
    this->setPlanes(32);
}

void StreakElement::resetStride()
{
    this->setStride(4);
}

void StreakElement::resetStrideMask()
{
    this->setStrideMask(0xf8f8f8f8);
}

void StreakElement::resetStrideShift()
{
    this->setStrideShift(3);
}

void StreakElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void StreakElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void StreakElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_RGB32);

    int videoArea = width * height;

    QImage oFrame = QImage(src.size(), src.format());

    quint32 *srcBits = (quint32 *) src.bits();
    quint32 *destBits = (quint32 *) oFrame.bits();

    if (packet.caps() != this->m_caps) {
        this->m_buffer = QImage(width, src.height() * this->m_planes, QImage::Format_RGB32);
        this->m_planetable.resize(this->m_planes);

        for (int i = 0; i < this->m_planes; i++)
            this->m_planetable[i] = (quint32 *) this->m_buffer.bits() + videoArea * i;

        this->m_plane = 0;
        this->m_caps = packet.caps();
    }

    for (int i = 0; i < videoArea; i++)
        this->m_planetable[this->m_plane][i] = \
            (srcBits[i] & this->m_strideMask) >> this->m_strideShift;

    int cf = this->m_plane & (this->m_stride - 1);

    for (int i = 0; i < videoArea; i++) {
        destBits[i] = this->m_planetable[cf][i]
                    + this->m_planetable[cf + this->m_stride][i]
                    + this->m_planetable[cf + this->m_stride * 2][i]
                    + this->m_planetable[cf + this->m_stride * 3][i]
                    + this->m_planetable[cf + this->m_stride * 4][i]
                    + this->m_planetable[cf + this->m_stride * 5][i]
                    + this->m_planetable[cf + this->m_stride * 6][i]
                    + this->m_planetable[cf + this->m_stride * 7][i];
    }

    this->m_plane++;
    this->m_plane = this->m_plane & (this->m_planes - 1);

    QbBufferPtr oBuffer(new char[oFrame.byteCount()]);
    memcpy(oBuffer.data(), oFrame.constBits(), oFrame.byteCount());

    QbCaps caps(packet.caps());
    caps.setProperty("format", "bgr0");
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
