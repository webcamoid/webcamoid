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

#include "edgeelement.h"

EdgeElement::EdgeElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));
}

QImage EdgeElement::convolve(const QImage &src) const
{
    QImage dst(src.size(), src.format());
    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) dst.bits();

    int srcWidth = src.width();
    int srcHeight = src.height();

    QVector<int> kernelX;

    kernelX << -1 << 0 << 1
            << -2 << 0 << 2
            << -1 << 0 << 1;

    QVector<int> kernelY;

    kernelY <<  1 <<  2 <<  1
            <<  0 <<  0 <<  0
            << -1 << -2 << -1;

    for (int y = 0; y < srcHeight; y++)
        for (int x = 0; x < srcWidth; x++) {
            int rx;
            int gx;
            int bx;

            this->sobel(srcBits, kernelX.constData(),
                        x, y, srcWidth, srcHeight,
                        &rx, &gx, &bx);

            int ry;
            int gy;
            int by;

            this->sobel(srcBits, kernelY.constData(),
                        x, y, srcWidth, srcHeight,
                        &ry, &gy, &by);

            int r = sqrt(rx * rx + ry * ry);
            int g = sqrt(gx * gx + gy * gy);
            int b = sqrt(bx * bx + by * by);

            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);

            *destBits++ = qRgb(r, g, b);
        }

    return dst;
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
                        QImage::Format_ARGB32);

    QImage oFrame = this->convolve(src);

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
