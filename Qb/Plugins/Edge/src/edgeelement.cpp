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

#include "edgeelement.h"

EdgeElement::EdgeElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=gray");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetEqualize();
    this->resetInvert();
}

bool EdgeElement::equalize() const
{
    return this->m_equalize;
}

bool EdgeElement::invert() const
{
    return this->m_invert;
}

void EdgeElement::setEqualize(bool equalize)
{
    this->m_equalize = equalize;
}

void EdgeElement::setInvert(bool invert)
{
    this->m_invert = invert;
}

void EdgeElement::resetEqualize()
{
    this->setEqualize(false);
}

void EdgeElement::resetInvert()
{
    this->setInvert(false);
}

void EdgeElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void EdgeElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void EdgeElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_Indexed8);

    QImage oFrame(src.size(), src.format());

    quint8 *srcBits = (quint8 *) src.bits();
    quint8 *destBits = (quint8 *) oFrame.bits();

    int widthMin = width - 1;
    int widthMax = width + 1;
    int heightMin = height - 1;

    if (this->m_equalize) {
        int videoArea = width * height;
        int minGray = 255;
        int maxGray = 0;

        for (int i = 0; i < videoArea; i++) {
            if (srcBits[i] < minGray)
                minGray = srcBits[i];

            if (srcBits[i] > maxGray)
                maxGray = srcBits[i];
        }

        int diffGray = maxGray - minGray;

        for (int i = 0; i < videoArea; i++)
            srcBits[i] = 255 * (srcBits[i] - minGray) / diffGray;
    }

    memset((quint8 *) oFrame.constScanLine(0), 0, width);
    memset((quint8 *) oFrame.constScanLine(heightMin), 0, width);

    for (int y = 0; y < height; y++) {
        int xOffset = y * width;

        destBits[xOffset] = 0;
        destBits[xOffset + widthMin] = 0;
    }

    for (int y = 1; y < heightMin; y++) {
        int xOffset = y * width;

        for (int x = 1; x < widthMin; x++) {
            int pixel = x + xOffset;

            int grayX =   srcBits[pixel - widthMax]
                        + srcBits[pixel - 1]
                        + srcBits[pixel + widthMin]
                        - srcBits[pixel - widthMin]
                        - srcBits[pixel + 1]
                        - srcBits[pixel + widthMax];

            int grayY =   srcBits[pixel - widthMax]
                        + srcBits[pixel - width]
                        + srcBits[pixel - widthMin]
                        - srcBits[pixel + widthMin]
                        - srcBits[pixel + width]
                        - srcBits[pixel + widthMax];

            int gray = sqrt(grayX * grayX + grayY * grayY);

            if (this->m_invert)
                destBits[pixel] = 255 - qBound(0, gray, 255);
            else
                destBits[pixel] = qBound(0, gray, 255);
        }
    }

    QbBufferPtr oBuffer(new char[oFrame.byteCount()]);
    memcpy(oBuffer.data(), oFrame.constBits(), oFrame.byteCount());

    QbCaps caps(packet.caps());
    caps.setProperty("format", "gray");
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
