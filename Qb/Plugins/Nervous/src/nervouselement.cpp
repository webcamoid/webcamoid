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

#include "nervouselement.h"

NervousElement::NervousElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    qRegisterMetaType<QRgb>("QRgb");

    this->resetNFrames();
    this->resetSimple();
    this->m_stride = 0;
}

int NervousElement::nFrames() const
{
    return this->m_nFrames;
}

bool NervousElement::simple() const
{
    return this->m_simple;
}

void NervousElement::setNFrames(int nFrames)
{
    this->m_nFrames = nFrames;
}

void NervousElement::setSimple(bool simple)
{
    this->m_simple = simple;
}

void NervousElement::resetNFrames()
{
    this->setNFrames(32);
}

void NervousElement::resetSimple()
{
    this->setSimple(false);
}

void NervousElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void NervousElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void NervousElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_ARGB32);

    if (packet.caps() != this->m_caps) {
        this->m_frames.clear();
        this->resetNFrames();
        this->m_stride = 0;

        this->m_caps = packet.caps();
    }

    this->m_frames << src.copy();
    int diff = this->m_frames.size() - this->m_nFrames;

    for (int i = 0; i < diff; i++)
        this->m_frames.takeFirst();

    int timer = 0;
    int nFrame = 0;

    if (!this->m_simple) {
        if (timer) {
            nFrame += this->m_stride;
            nFrame = qBound(0, nFrame, this->m_frames.size() - 1);
            timer--;
        }
        else {
            nFrame = qrand() % this->m_frames.size();
            this->m_stride = qrand() % 5 - 2;

            if(this->m_stride >= 0)
                this->m_stride++;

            timer = qrand() % 6 + 2;
        }
    }
    else if(this->m_frames.size() > 0)
        nFrame = qrand() % this->m_frames.size();

    QImage oFrame = this->m_frames[nFrame];

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
