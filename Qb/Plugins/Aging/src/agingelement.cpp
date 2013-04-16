/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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

#include "agingelement.h"

AgingElement::AgingElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->m_dx << 1 << 1 << 0 << -1 << -1 << -1 << 0 << 1;
    this->m_dy << 0 << -1 << -1 << -1 << 0 << 1 << 1 << 1;

    this->resetNScratches();
    this->resetScratchLines();
    this->resetAgingMode();
}

AgingElement::~AgingElement()
{
}

int AgingElement::nScratches() const
{
    return this->m_scratches.size();
}

int AgingElement::scratchLines() const
{
    return this->m_scratchLines;
}

int AgingElement::agingMode() const
{
    return this->m_agingMode;
}

QImage AgingElement::colorAging(const QImage &src)
{
    QImage dest(src.size(), src.format());
    static qint32 c = 0x18;

    c -= qrand() >> 28;

    if (c < 0)
        c = 0;

    if (c > 0x18)
        c = 0x18;

    quint32 *srcBits = (quint32 *) src.constBits();
    quint32 *destBits = (quint32 *) dest.bits();

    int dstArea = dest.byteCount() >> 2;

    for (int i = 0; i < dstArea; i++)
    {
        quint32 a = *srcBits++;
        quint32 b = (a & 0xfcfcfc) >> 2;

        *destBits++ = a - b + (c | (c << 8) | (c << 16)) + ((qrand() >> 8) & 0x101010);
    }

    return dest;
}

void AgingElement::scratching(QImage &dest)
{
    for (int i = 0; i < this->scratchLines(); i++)
        if (this->m_scratches[i].life())
        {
            this->m_scratches[i].setX(this->m_scratches[i].x() +
                                      this->m_scratches[i].dx());

            if (this->m_scratches[i].x() < 0 ||
                this->m_scratches[i].x() > dest.width() * 256)
            {
                this->m_scratches[i].setLife(0);

                break;
            }

            quint32 *p = (quint32 *) dest.bits() + (this->m_scratches[i].x() >> 8);
            int y1;

            if (this->m_scratches[i].init())
            {
                y1 = this->m_scratches[i].init();
                this->m_scratches[i].setInit(0);
            }
            else
                y1 = 0;

            this->m_scratches[i].setLife(this->m_scratches[i].life() - 1);
            int y2;

            if (this->m_scratches[i].life())
                y2 = dest.height();
            else
                y2 = qrand() % dest.height();

            for (int y = y1; y < y2; y++)
            {
                quint32 a = *p & 0xfefeff;
                a += 0x202020;
                quint32 b = a & 0x1010100;
                *p = a | (b - (b >> 8));
                p += dest.width();
            }
        }
        else if ((qrand() & 0xf0000000) == 0)
        {
            this->m_scratches[i].setLife(2 + (qrand() >> 27));
            this->m_scratches[i].setX(qrand() % (dest.width() * 256));
            this->m_scratches[i].setDx(qrand() >> 23);
            this->m_scratches[i].setInit((qrand() % (dest.height() - 1)) + 1);
        }
}

void AgingElement::pits(QImage &dest)
{
    int pnum;
    int areaScale;

    if (this->agingMode() == 0)
    {
        areaScale = dest.width() * dest.height() / 64 / 480;

        if (areaScale < 1)
            areaScale = 1;
    }
    else
        areaScale = 1;

    int pnumscale = areaScale * 2;
    static int pitsInterval = 0;

    if (pitsInterval)
    {
        pnum = pnumscale + (qrand() % pnumscale);
        pitsInterval--;
    }
    else
    {
        pnum = qrand() % pnumscale;

        if ((qrand() & 0xf8000000) == 0)
            pitsInterval = (qrand() >> 28) + 20;
    }

    quint32 *destBits = (quint32 *) dest.bits();

    for (int i = 0; i < pnum; i++)
    {
        int x = qrand() % (dest.width() - 1);
        int y = qrand() % (dest.height() - 1);
        int size = qrand() >> 28;

        for (int j = 0; j < size; j++)
        {
            x = x + qrand() % 3 - 1;
            y = y + qrand() % 3 - 1;

            if(x < 0 || x >= dest.width())
                break;

            if(y < 0 || y >= dest.height())
                break;

            destBits[y * dest.width() + x] = 0xc0c0c0;
        }
    }
}

void AgingElement::dusts(QImage &dest)
{
    static int dustInterval = 0;

    if (dustInterval == 0)
    {
        if ((qrand() & 0xf0000000) == 0)
            dustInterval = qrand() >> 29;

        return;
    }

    int areaScale;

    if (this->agingMode() == 0)
    {
        areaScale = dest.width() * dest.height() / 64 / 480;

        if (areaScale < 1)
            areaScale = 1;
    }
    else
        areaScale = 1;

    int dnum = areaScale * 4 + (qrand() >> 27);
    quint32 *destBits = (quint32 *) dest.bits();

    for (int i = 0; i < dnum; i++)
    {
        int x = qrand() % dest.width();
        int y = qrand() % dest.height();
        int d = qrand() >> 29;
        int len = qrand() % areaScale + 5;

        for (int j = 0; j < len; j++)
        {
            destBits[y * dest.width() + x] = 0x101010;
            y += this->m_dy[d];
            x += this->m_dx[d];

            if(x < 0 || x >= dest.width())
                break;

            if(y < 0 || y >= dest.height())
                break;

            d = (d + qrand() % 3 - 1) & 7;
        }
    }

    dustInterval--;
}

void AgingElement::setNScratches(int nScratches)
{
    this->m_scratches.resize(nScratches);
}

void AgingElement::setScratchLines(int scratchLines)
{
    this->m_scratchLines = scratchLines;
}

void AgingElement::setAgingMode(int agingMode)
{
    this->m_agingMode = agingMode;
}

void AgingElement::resetNScratches()
{
    this->setNScratches(20);
}

void AgingElement::resetScratchLines()
{
    this->setScratchLines(7);
}

void AgingElement::resetAgingMode()
{
    this->setAgingMode(0);
}

void AgingElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        packet.caps().mimeType() != "video/x-raw" ||
        this->state() != ElementStatePlaying)
        return;

    this->m_convert->iStream(packet);
}

void AgingElement::setState(ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void AgingElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage oFrame = QImage(packet.buffer().data(),
                           width,
                           height,
                           QImage::Format_RGB32);

    oFrame = this->colorAging(oFrame);
    this->scratching(oFrame);
    this->pits(oFrame);

    if (this->agingMode() == 0)
        this->dusts(oFrame);

    QSharedPointer<uchar> oBuffer(new uchar[oFrame.byteCount()]);
    memcpy(oBuffer.data(), oFrame.constBits(), oFrame.byteCount());

    QbCaps caps(QString("video/x-raw,"
                        "format=%1,"
                        "width=%2,"
                        "height=%3,"
                        "fps=%4").arg("bgr0")
                                 .arg(width)
                                 .arg(height)
                                 .arg(packet.caps().property("fps").toString()));

    QbPacket oPacket(caps,
                     oBuffer,
                     oFrame.byteCount());

    oPacket.setDts(packet.dts());
    oPacket.setPts(packet.pts());
    oPacket.setDuration(packet.duration());
    oPacket.setTimeBase(packet.timeBase());
    oPacket.setIndex(packet.index());

    emit this->oStream(oPacket);
}
