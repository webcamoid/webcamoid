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

#include "delaygrabelement.h"

DelayGrabElement::DelayGrabElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->m_modeToStr[DelayGrabModeRandomSquare] = "RandomSquare";
    this->m_modeToStr[DelayGrabModeVerticalIncrease] = "VerticalIncrease";
    this->m_modeToStr[DelayGrabModeHorizontalIncrease] = "HorizontalIncrease";
    this->m_modeToStr[DelayGrabModeRingsIncrease] = "RingsIncrease";

    this->resetMode();
    this->resetBlockSize();
    this->resetNFrames();
}

QString DelayGrabElement::mode() const
{
    return this->m_modeToStr[this->m_mode];
}

int DelayGrabElement::blockSize() const
{
    return this->m_blockSize;
}

int DelayGrabElement::nFrames() const
{
    return this->m_nFrames;
}

void DelayGrabElement::setBlockSize(int blockSize)
{
    this->m_blockSize = blockSize;
}

void DelayGrabElement::setNFrames(int nFrames)
{
    this->m_nFrames = nFrames;
}

void DelayGrabElement::resetMode()
{
    this->setMode("RingsIncrease");
}

void DelayGrabElement::resetBlockSize()
{
    this->setBlockSize(2);
}

void DelayGrabElement::resetNFrames()
{
    this->setNFrames(71);
}

QVector<int> DelayGrabElement::createDelaymap(DelayGrabMode mode)
{
    QVector<int> delayMap(this->m_delayMapHeight * this->m_delayMapWidth);

    for (int i = 0, y = this->m_delayMapHeight; y > 0; y--) {
        for (int x = this->m_delayMapWidth; x > 0; i++, x--) {
            // Random delay with square distribution
            if (mode == DelayGrabModeRandomSquare) {
                float d = (float) qrand() / RAND_MAX;
                delayMap[i] = 16.0 * d * d;
            }
            // Vertical stripes of increasing delay outward from center
            else if (mode == DelayGrabModeVerticalIncrease) {
                int k = this->m_delayMapWidth >> 1;
                int v;

                if (x < k)
                    v = k - x;
                else if (x > k)
                    v = x - k;
                else
                    v = 0;

                delayMap[i] = v >> 1;
            }
            // Horizontal stripes of increasing delay outward from center
            else if (mode == DelayGrabModeHorizontalIncrease) {
                int k = this->m_delayMapHeight >> 1;
                int v;

                if (y < k)
                    v = k;
                else if (y > k)
                    v = y - k;
                else
                    v = 0;

                delayMap[i] = v >> 1;
            }
            // Rings of increasing delay outward from center
            else {
                int dx = x - (this->m_delayMapWidth >> 1);
                int dy = y - (this->m_delayMapHeight >> 1);
                int v = sqrt(dx * dx + dy * dy);

                delayMap[i] = v >> 1;
            }

            // Clip values
            delayMap[i] = qBound(0, delayMap[i], this->m_nFrames - 1);
        }
    }

    return delayMap;
}

void DelayGrabElement::setMode(const QString &mode)
{
    if (this->m_modeToStr.values().contains(mode))
        this->m_mode = this->m_modeToStr.key(mode);
    else
        this->m_mode = DelayGrabModeRingsIncrease;
}

void DelayGrabElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void DelayGrabElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void DelayGrabElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_RGB32);

    QImage oFrame = QImage(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    if (packet.caps() != this->m_caps) {
        int bpp = src.bytesPerLine() / width;
        this->m_blockPerPitch = this->m_blockSize * src.bytesPerLine();
        this->m_blockPerBytes = this->m_blockSize * (bpp >> 3);
        this->m_blockPerRes = this->m_blockSize << (bpp >> 4);

        this->m_delayMapWidth = width / this->m_blockSize;
        this->m_delayMapHeight = height / this->m_blockSize;

        this->m_delayMap = this->createDelaymap(this->m_mode);

        this->m_caps = packet.caps();
    }

    this->m_frames << src.copy();
    int diff = this->m_frames.size() - this->m_nFrames;

    for (int i = 0; i < diff; i++)
        this->m_frames.takeFirst();

    // Copy image blockwise to screenbuffer
    for (int i = 0, y = 0; y < this->m_delayMapHeight; y++) {
        for (int x = 0; x < this->m_delayMapWidth ; i++, x++) {
            int curFrame = (this->m_frames.size() - 1 - this->m_delayMap[i]) % this->m_frames.size();
            int xyoff = x * this->m_blockPerBytes + y * this->m_blockPerPitch;

            // source
            curpos = imagequeue;
            curpos += geo.size * curFrame;
            curpos += xyoff;

            // targe
            curimage = (uint8_t *) out;
            curimage += xyoff;

            // copy block
            for (int j = 0; j < this->m_blockSize; j++) {
                memcpy(curimage, curpos, this->m_blockPerRes);
                curpos += geo.pitch;
                curimage += geo.pitch;
            }
        }
    }

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
