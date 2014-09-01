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

#include "colorreplaceelement.h"

ColorReplaceElement::ColorReplaceElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    qRegisterMetaType<QRgb>("QRgb");

    this->resetFrom();
    this->resetTo();
    this->resetRadius();
}

QRgb ColorReplaceElement::from() const
{
    return this->m_from;
}

QRgb ColorReplaceElement::to() const
{
    return this->m_to;
}

float ColorReplaceElement::radius() const
{
    return this->m_radius;
}

void ColorReplaceElement::setFrom(QRgb from)
{
    this->m_from = from;
}

void ColorReplaceElement::setTo(QRgb to)
{
    this->m_to = to;
}

void ColorReplaceElement::setRadius(float radius)
{
    this->m_radius = radius;
}

void ColorReplaceElement::resetFrom()
{
    this->m_from = qRgb(0, 0, 0);
}

void ColorReplaceElement::resetTo()
{
    this->m_to = qRgb(0, 0, 0);
}

void ColorReplaceElement::resetRadius()
{
    this->setRadius(1.0);
}

void ColorReplaceElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void ColorReplaceElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void ColorReplaceElement::processFrame(const QbPacket &packet)
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

    for (int i = 0; i < videoArea; i++) {
        int r = qRed(srcBits[i]);
        int g = qGreen(srcBits[i]);
        int b = qBlue(srcBits[i]);

        int rf = qRed(this->m_from);
        int gf = qGreen(this->m_from);
        int bf = qBlue(this->m_from);

        int rd = r - rf;
        int gd = g - gf;
        int bd = b - bf;

        float k = sqrt(rd * rd + gd * gd + bd * bd);

        if (k <= this->m_radius) {
            float p = k / this->m_radius;

            int rt = qRed(this->m_to);
            int gt = qGreen(this->m_to);
            int bt = qBlue(this->m_to);

            r = p * (r - rt) + rt;
            g = p * (g - gt) + gt;
            b = p * (b - bt) + bt;

            destBits[i] = qRgba(r, g, b, qAlpha(srcBits[i]));
        }
        else
            destBits[i] = srcBits[i];
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
