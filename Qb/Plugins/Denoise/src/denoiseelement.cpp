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

#include "denoiseelement.h"

DenoiseElement::DenoiseElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr24");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetStrength();
    this->resetTemplateWindowSize();
    this->resetSearchWindowSize();
}

float DenoiseElement::strength() const
{
    return this->m_strength;
}

int DenoiseElement::templateWindowSize() const
{
    return this->m_templateWindowSize;
}

int DenoiseElement::searchWindowSize() const
{
    return this->m_searchWindowSize;
}

void DenoiseElement::setStrength(float strength)
{
    this->m_strength = strength;
}

void DenoiseElement::setTemplateWindowSize(int templateWindowSize)
{
    this->m_templateWindowSize = templateWindowSize;
}

void DenoiseElement::setSearchWindowSize(int searchWindowSize)
{
    this->m_searchWindowSize = searchWindowSize;
}

void DenoiseElement::resetStrength()
{
    this->setStrength(3);
}

void DenoiseElement::resetTemplateWindowSize()
{
    this->setTemplateWindowSize(7);
}

void DenoiseElement::resetSearchWindowSize()
{
    this->setSearchWindowSize(21);
}

void DenoiseElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void DenoiseElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void DenoiseElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_RGB888);

    QImage oFrame(src.size(), src.format());

    cv::Mat srcCV(src.height(),
                  src.width(),
                  CV_8UC3,
                  (uchar *) src.bits(),
                  src.bytesPerLine());

    cv::Mat dstCV(oFrame.height(),
                  oFrame.width(),
                  CV_8UC3,
                  (uchar *) oFrame.bits(),
                  oFrame.bytesPerLine());

    cv::fastNlMeansDenoising(srcCV,
                             dstCV,
                             this->m_strength,
                             this->m_templateWindowSize,
                             this->m_searchWindowSize);

    QbBufferPtr oBuffer(new char[oFrame.byteCount()]);
    memcpy(oBuffer.data(), oFrame.constBits(), oFrame.byteCount());

    QbCaps caps(packet.caps());
    caps.setProperty("format", "bgr24");
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
