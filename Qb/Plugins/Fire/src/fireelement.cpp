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

#include "fireelement.h"

FireElement::FireElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->m_palette.resize(256);

    this->resetMode();
    this->resetDecay();
    this->resetThreshold();
    this->resetMaxColor();
    this->makePalette();
}

int FireElement::mode()
{
    return this->m_mode;
}

int FireElement::decay()
{
    return this->m_decay;
}

int FireElement::threshold()
{
    return this->m_threshold / 7;
}

int FireElement::maxColor()
{
    return this->m_maxColor;
}

bool FireElement::event(QEvent *event)
{
    bool r;

    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);

        if (ke->key() == Qt::Key_Space)
        {
            if (this->mode() == 0)
                this->m_bgIsSet = false;

            return true;
        }
        else if (ke->key() == Qt::Key_1)
        {
            this->setMode(0);

            return true;
        }
        else if (ke->key() == Qt::Key_2)
        {
            this->setMode(1);

            return true;
        }
        else if (ke->key() == Qt::Key_3)
        {
            this->setMode(2);

            return true;
        }
    }
    else if (event->type() == QEvent::ThreadChange)
    {
        QObject::disconnect(this->m_convert.data(),
                            SIGNAL(oStream(const QbPacket &)),
                            this,
                            SIGNAL(processFrame(const QbPacket &)));

        r = QObject::event(event);
        this->m_convert->moveToThread(this->thread());

        QObject::connect(this->m_convert.data(),
                         SIGNAL(oStream(const QbPacket &)),
                         this,
                         SIGNAL(processFrame(const QbPacket &)));
    }
    else
        r = QObject::event(event);

    return r;
}

int FireElement::trunc(double f)
{
    int i = (int) f;

    if (i < 0)
        i = 0;

    if (i > 255)
        i = 255;

    return i;
}

void FireElement::hsiToRgb(double h, double s, double i, int *r, int *g, int *b)
{
    double rv, gv, bv;

    rv = 1 + s * sin(h - 2 * M_PI / 3);
    gv = 1 + s * sin(h);
    bv = 1 + s * sin(h + 2 * M_PI / 3);

    double t = 255.999 * i / 2;

    *r = this->trunc(rv * t);
    *g = this->trunc(gv * t);
    *b = this->trunc(bv * t);
}

void FireElement::imageBgSetY(QImage &src)
{
    quint32 *p = (quint32 *) src.bits();
    short *q = (short *) this->m_background.bits();
    int videoArea = src.width() * src.height();

    for (int  i = 0; i < videoArea; i++)
    {
        int r = ((*p) & 0xff0000) >> (16 - 1);
        int g = ((*p) & 0xff00) >> (8 - 2);
        int b = (*p) & 0xff;

        *q = (short) (r + g + b);

        p++;
        q++;
    }
}

QImage FireElement::imageBgSubtractY(QImage &src)
{
    quint32 *p = (quint32 *) src.bits();
    short *q = (short *) this->m_background.bits();
    uchar *r = this->m_diff.bits();
    int videoArea = src.width() * src.height();

    for (int i = 0; i < videoArea; i++)
    {
        int R = ((*p) & 0xff0000) >> (16 - 1);
        int G = ((*p) & 0xff00) >> (8 - 2);
        int B = (*p) & 0xff;
        int v = (R + G + B) - (int) (*q);
        *r = ((v + this->m_threshold) >> 24) | ((this->m_threshold - v) >> 24);

        p++;
        q++;
        r++;
    }

    return this->m_diff;
}

void FireElement::makePalette()
{
    int r, g, b;

    for (int i = 0; i < this->maxColor(); i++)
    {
            this->hsiToRgb(4.6 - 1.5 * i / this->maxColor(),
                     (double) i / this->maxColor(),
                     (double) i / this->maxColor(),
                     &r, &g, &b);

            this->m_palette[i] = (r << 16) | (g << 8) | b;
    }

    for (int i = this->maxColor(); i < 256; i++)
    {
            if (r < 255)
                r++;

            if (r < 255)
                r++;

            if (r < 255)
                r++;

            if (g < 255)
                g++;

            if (g < 255)
                g++;

            if (b < 255)
                b++;

            if (b < 255)
                b++;

            this->m_palette[i] = (r << 16) | (g << 8) | b;
    }
}

void FireElement::setBackground(QImage &src)
{
    this->imageBgSetY(src);
    this->m_bgIsSet = true;
}

void FireElement::stateChange(QbElement::ElementState from, QbElement::ElementState to)
{
    if (from == QbElement::ElementStateNull
        && to == QbElement::ElementStatePaused)
        this->m_bgIsSet = false;
}

void FireElement::setMode(int mode)
{
    this->m_mode = mode;
}

void FireElement::setDecay(int decay)
{
    this->m_decay = decay;
}

void FireElement::setThreshold(int threshold)
{
    this->m_threshold = 7 * threshold;
}

void FireElement::setMaxColor(int maxColor)
{
    this->m_maxColor = maxColor;
}

void FireElement::resetMode()
{
    this->setMode(0);
}

void FireElement::resetDecay()
{
    this->setDecay(15);
}

void FireElement::resetThreshold()
{
    this->setThreshold(50);
}

void FireElement::resetMaxColor()
{
    this->setMaxColor(120);
}

void FireElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void FireElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void FireElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage(packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_RGB32);

    int videoArea = src.width() * src.height();

    if (packet.caps() != this->m_caps)
    {
        this->m_background = QImage(src.width(), src.height(), QImage::Format_RGB32);
        this->m_diff = QImage(src.width(), src.height(), QImage::Format_Indexed8);
        this->m_buffer.resize(videoArea);

        this->m_caps = packet.caps();
    }

    if (!this->m_bgIsSet)
        this->setBackground(src);

    quint32 *srcBits = (quint32 *) src.constBits();

    if (this->mode() == 1)
        for (int i = 0; i < videoArea - src.width(); i++)
        {
            uchar v = (srcBits[i] >> 16) & 0xff;

            if (v > 150)
                this->m_buffer[i] = this->m_buffer[i] | v;
        }
    else if (this->mode() == 2)
        for (int i = 0; i < videoArea - src.width(); i++)
        {
            uchar v = srcBits[i] & 0xff;

            if (v < 60)
                this->m_buffer[i] = this->m_buffer[i] | (0xff - v);
        }
    else
    {
        QImage diff = this->imageBgSubtractY(src);

        for (int i = 0; i < videoArea - src.width(); i++)
            this->m_buffer[i] = this->m_buffer[i] | diff.constBits()[i];
    }

    for (int x = 1; x < src.width() - 1; x++)
    {
        int i = src.width() + x;

        for (int y = 1; y < src.height(); y++)
        {
            uchar v = this->m_buffer[i];

            if (v < this->decay())
                this->m_buffer[i - src.width()] = 0;
            else
                this->m_buffer[i - src.width() + qrand() % 3 - 1] = v - (qrand() & this->decay());

            i += src.width();
        }
    }

    QImage oFrame(src.size(), src.format());
    quint32 *destBits = (quint32 *) oFrame.bits();

    for (int y = 0; y < src.height(); y++)
        for (int x = 1; x < src.width() - 1; x++)
        {
            uchar v = this->m_buffer[y * src.width() + x];
            destBits[y * src.width() + x] = this->m_palette[v];
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
