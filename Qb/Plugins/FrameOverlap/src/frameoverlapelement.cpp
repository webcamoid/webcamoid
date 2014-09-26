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

#include "frameoverlapelement.h"

FrameOverlapElement::FrameOverlapElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

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

QbPacket FrameOverlapElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    int videoArea = src.width() * src.height();

    QImage oFrame(src.size(), src.format());
    QRgb *destBits = (QRgb *) oFrame.bits();

    if (packet.caps() != this->m_caps) {
        this->m_frames.clear();
        this->resetNFrames();
        this->resetStride();

        this->m_caps = packet.caps();
    }

    this->m_frames << src.copy();
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

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
