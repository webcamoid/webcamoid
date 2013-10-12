/* Webcamod, webcam capture plasmoid.
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

#include "warholelement.h"

WarholElement::WarholElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->m_colorTable << 0x000080 << 0x008000 << 0x800000
                       << 0x00e000 << 0x808000 << 0x800080
                       << 0x808080 << 0x008080 << 0xe0e000;

    this->resetNFrames();
}

WarholElement::~WarholElement()
{
}

int WarholElement::nFrames() const
{
    return this->m_nFrames;
}

void WarholElement::setNFrames(int nFrames)
{
    this->m_nFrames = nFrames;
}

void WarholElement::resetNFrames()
{
    this->setNFrames(3);
}

void WarholElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void WarholElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void WarholElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_RGB32);

    QImage oFrame = QImage(src.size(), src.format());

    quint32 *srcBits = (quint32 *) src.bits();
    quint32 *destBits = (quint32 *) oFrame.bits();

    for (int y = 0; y < src.height(); y++)
        for (int x = 0; x < src.width(); x++)
        {
            int p = (x * this->nFrames()) % src.width();
            int q = (y * this->nFrames()) % src.height();
            int i = ((y * this->nFrames()) / src.height()) * this->nFrames() +
                    ((x * this->nFrames()) / src.width());

            *destBits++ = srcBits[q * src.width() + p] ^ this->m_colorTable[i];
        }

    QbBufferPtr oBuffer(new uchar[oFrame.byteCount()]);
    memcpy(oBuffer.data(), oFrame.constBits(), oFrame.byteCount());

    QbCaps caps(packet.caps());
    caps.setProperty("format", "bgr0");
    caps.setProperty("width", oFrame.width());
    caps.setProperty("height", oFrame.height());

    QbPacket oPacket(caps,
                     oBuffer,
                     oFrame.byteCount());

    oPacket.setPts(packet.pts());
    oPacket.setDuration(packet.duration());
    oPacket.setTimeBase(packet.timeBase());
    oPacket.setIndex(packet.index());

    emit this->oStream(oPacket);
}
