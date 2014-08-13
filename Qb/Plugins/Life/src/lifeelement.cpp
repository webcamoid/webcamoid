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

#include "lifeelement.h"

LifeElement::LifeElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetYThreshold();
}

int LifeElement::yThreshold() const
{
    return this->m_yThreshold / 7;
}

QImage LifeElement::imageBgSubtractUpdateY(const QImage &src)
{
    quint32 *p = (quint32 *) src.bits();
    short *q = (short *) this->m_background.bits();
    qint8 *d = (qint8 *) this->m_diff.bits();
    int videoArea = src.width() * src.height();

    for (int i = 0; i < videoArea; i++) {
        int r = ((*p) & 0xff0000) >> (16 - 1);
        int g = ((*p) & 0xff00) >> (8 - 2);
        int b = (*p) & 0xff;
        int v = (r + g + b) - (int) (*q);

        *q = (short) (r + g + b);
        *d = ((this->m_yThreshold + v) >> 24)
             | ((this->m_yThreshold - v) >> 24);

        p++;
        q++;
        d++;
    }

    return this->m_diff;
}

QImage LifeElement::imageDiffFilter(const QImage &diff)
{
    quint8 *src = (quint8 *) diff.bits();
    quint8 *dest = (quint8 *) this->m_diff2.bits() + diff.width() + 1;

    for (int y = 1; y < diff.height() - 1; y++) {
        uint sum1 = src[0] + src[diff.width()] + src[diff.width() * 2];
        uint sum2 = src[1] + src[diff.width() + 1] + src[diff.width() * 2 + 1];
        src += 2;

        for (int x = 1; x < diff.width() - 1; x++) {
            uint sum3 = src[0] + src[diff.width()] + src[diff.width() * 2];
            uint count = sum1 + sum2 + sum3;

            sum1 = sum2;
            sum2 = sum3;

            *dest++ = (0xff * 3 - count) >> 24;
            src++;
        }

        dest += 2;
    }

    return this->m_diff2;
}

void LifeElement::setYThreshold(int threshold)
{
    this->m_yThreshold = 7 * threshold;
}

void LifeElement::resetYThreshold()
{
    this->setYThreshold(40);
}

void LifeElement::clearField()
{
    int videoArea = this->m_background.width()
                    * this->m_background.height();

    memset(this->m_field1, 0, videoArea);
}

void LifeElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void LifeElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void LifeElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_RGB32);

    int videoArea = src.width() * src.height();

    if (packet.caps() != this->m_caps) {
        this->m_background = QImage(src.width(), src.height(), QImage::Format_RGB32);
        this->m_diff = QImage(src.width(), src.height(), QImage::Format_Indexed8);
        this->m_diff2 = QImage(src.width(), src.height(), QImage::Format_Indexed8);

        this->m_field = QImage(src.width(), 2 * src.height(), QImage::Format_Indexed8);
        this->m_field1 = (quint8 *) this->m_field.bits();
        this->m_field2 = (quint8 *) this->m_field.bits() + videoArea;
        this->clearField();

        this->m_caps = packet.caps();
    }

    QImage oFrame = QImage(src.size(), src.format());

    quint32 *srcBits = (quint32 *) src.bits();
    quint32 *destBits = (quint32 *) oFrame.bits();

    // {

    this->imageDiffFilter(this->imageBgSubtractUpdateY(src));
    quint8 *p = this->m_diff2.bits();

    for (int x = 0; x < videoArea; x++)
        this->m_field1[x] |= p[x];

    p = this->m_field1 + 1;
    quint8 *q = this->m_field2 + width + 1;
    destBits += width + 1;
    srcBits += width + 1;

    // each value of cell is 0 or 0xff. 0xff can be treated as -1, so
    // following equations treat each value as negative number.
    for (int y = 1; y < height - 1; y++) {
        quint8 sum1 = 0;
        quint8 sum2 = p[0] + p[width] + p[width * 2];

        for (int x = 1; x < width - 1; x++) {
            quint8 sum3 = p[1] + p[width + 1] + p[width * 2 + 1];
            quint8 sum = sum1 + sum2 + sum3;
            quint8 v = 0 - ((sum == 0xfd) | ((p[width] != 0) & (sum == 0xfc)));
            *q++ = v;
            quint32 pix = (qint8) v;
            // pix = pix >> 8;
            *destBits++ = pix | *srcBits++;
            sum1 = sum2;
            sum2 = sum3;
            p++;
        }

        p += 2;
        q += 2;
        srcBits += 2;
        destBits += 2;
    }

    p = this->m_field1;
    this->m_field1 = this->m_field2;
    this->m_field2 = p;

    // }

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
