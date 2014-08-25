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

#include "frameoverlapelement.h"

FrameOverlapElement::FrameOverlapElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    qRegisterMetaType<QRgb>("QRgb");

    this->resetNFrames();
    this->resetStride();
}

int FrameOverlapElement::nFrames() const
{
    return this->m_nFrames;
}

int FrameOverlapElement::stride() const
{
    return this->m_stride;
}

void FrameOverlapElement::setNFrames(int nFrames)
{
    this->m_nFrames = nFrames;
}

void FrameOverlapElement::setStride(int stride)
{
    this->m_stride = stride;
}

void FrameOverlapElement::resetNFrames()
{
    this->setNFrames(16);
}

void FrameOverlapElement::resetStride()
{
    this->setStride(4);
}

void FrameOverlapElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void FrameOverlapElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void FrameOverlapElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_ARGB32);

    int videoArea = width * height;

    QImage oFrame(src.size(), src.format());

    QRgb *destBits = (QRgb *) oFrame.bits();

    if (packet.caps() != this->m_caps) {
        this->m_frames.clear();
        this->resetNFrames();
        this->resetStride();

        this->m_caps = packet.caps();
    }

    this->m_frames << src;
    int diff = this->m_frames.size() - this->m_nFrames;

    for (int i = 0; i < diff; i++)
        this->m_frames.takeFirst();

    QVector<QRgb *> framesBits;

    for (int frame = 0; frame < this->m_frames.size(); frame++)
        framesBits << (QRgb *) this->m_frames[frame].bits();

    for (int i = 0; i < videoArea; i++) {
        int r = 0;
        int g = 0;
        int b = 0;
        int a = 0;
        int n = 0;

        for (int frame = this->m_frames.size() - 1;
             frame >= 0;
             frame -= this->m_stride) {
            QRgb pixel = framesBits[frame][i];

            r += qRed(pixel);
            g += qGreen(pixel);
            b += qBlue(pixel);
            a += qAlpha(pixel);
            n++;
        }

        r /= n;
        g /= n;
        b /= n;
        a /= n;

        destBits[i] = qRgba(r, g, b, a);
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
