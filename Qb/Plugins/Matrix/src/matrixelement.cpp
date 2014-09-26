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

#include "matrixelement.h"

MatrixElement::MatrixElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    this->m_matrixFont.load(":/Qb/Plugins/Matrix/share/matrixFont.xpm");

    this->resetNChars();
    this->resetFontWidth();
    this->resetFontHeight();
    this->resetFontDepth();
    this->resetMode();
    this->resetWhite();
    this->resetPause();

    this->m_font.resize(this->nChars() * this->fontWidth() * this->fontHeight());
    this->m_palette.resize(256 * this->fontDepth());
}

int MatrixElement::nChars() const
{
    return this->m_nChars;
}

int MatrixElement::fontWidth() const
{
    return this->m_fontWidth;
}

int MatrixElement::fontHeight() const
{
    return this->m_fontHeight;
}

int MatrixElement::fontDepth() const
{
    return this->m_fontDepth;
}

int MatrixElement::mode() const
{
    return this->m_mode;
}

float MatrixElement::white() const
{
    return this->m_white;
}

bool MatrixElement::pause() const
{
    return this->m_pause;
}

void MatrixElement::setPattern()
{
    // FIXME: This code is highly depends on the structure of bundled
    //        matrixFont.xpm.
    char *matrixFontBits = (char *) this->m_matrixFont.bits();

    for (int l = 0; l < 32; l++) {
        char *p = &matrixFontBits[5 + l];
        int cy = l /4;
        int y = l % 4;

        for (int c = 0; c < 40; c++) {
            int cx = c / 4;
            int x = c % 4;

            uchar v;

            switch (*p) {
                case ' ':
                    v = 0;
                break;

                case '.':
                    v = 1;
                break;

                case 'o':
                    v = 2;
                break;

                case 'O':
                default:
                    v = 3;
                break;
            }

            this->m_font[(cy * 10 + cx) * this->fontWidth() * this->fontHeight() + y * this->fontWidth() + x] = v;
            p++;
        }
    }
}

quint32 MatrixElement::green(uint v)
{
    if (v < 256)
        return ((int)(v * this->white()) << 16) | (v << 8) | (int) (v * this->white());

    uint w = v - (int) (256 * this->white());

    if (w > 255)
        w = 255;

    return (w << 16) + 0xff00 + w;
}

void MatrixElement::setPalette()
{
    for (int i = 0; i < 256; i++) {
        this->m_palette[i * this->fontDepth()] = 0;
        this->m_palette[i * this->fontDepth() + 1] = this->green(0x44 * i / 170);
        this->m_palette[i * this->fontDepth() + 2] = this->green(0x99 * i / 170);
        this->m_palette[i * this->fontDepth() + 3] = this->green(0xff * i / 170);
    }
}

void MatrixElement::darkenColumn(int x)
{
    uchar *p = this->m_vmap.bits() + x;

    for (int y = 0; y < this->m_mapHeight; y++) {
        int v = *p;

        if (v < 255) {
            v *= 0.9;
            *p = v;
        }

        p += this->m_mapWidth;
    }
}

void MatrixElement::blipNone(int x)
{
    // This is a test code to reuse a randome number for multi purpose. :-P
    // Of course it isn't good code because fastrand() doesn't generate ideal
    // randome numbers.
    uint r = qrand();

    if ((r & 0xf0) == 0xf0) {
        this->m_blips[x].setMode(BlipModeFall);
        this->m_blips[x].setY(0);
        this->m_blips[x].setSpeed((r >> 30) + 1);
        this->m_blips[x].setTimer(0);
    }
    else if ((r & 0x0f000) ==  0x0f000) {
        this->m_blips[x].setMode(BlipModeSlid);
        this->m_blips[x].setTimer((r >> 28) + 15);
        this->m_blips[x].setSpeed(((r >> 24) & 3) + 2);
    }
}

void MatrixElement::blipFall(int x)
{
    int y = this->m_blips[x].y();
    uchar *p = this->m_vmap.bits() + x + y * this->m_mapWidth;
    uchar *c = this->m_cmap.bits() + x + y * this->m_mapWidth;

    for (int i = this->m_blips[x].speed(); i > 0; i--) {
        if (this->m_blips[x].timer() > 0)
            *p = 255;
        else
            *p = 254 - i * 10;

        *c = qrand() % this->nChars();
        p += this->m_mapWidth;
        c += this->m_mapWidth;
        y++;

        if (y >= this->m_mapHeight)
            break;
    }

    if (this->m_blips[x].timer() > 0)
        this->m_blips[x].setTimer(this->m_blips[x].timer() - 1);

    if (y >= this->m_mapHeight)
        this->m_blips[x].setMode(BlipModeNone);

    this->m_blips[x].setY(y);

    if (this->m_blips[x].timer() == 0) {
        uint r = qrand();

        if ((r & 0x3f00) == 0x3f00)
            this->m_blips[x].setTimer((r >> 28) + 8);
        else if (this->m_blips[x].speed() > 1 && (r & 0x7f) == 0x7f) {
            this->m_blips[x].setMode(BlipModeStop);
            this->m_blips[x].setTimer((r >> 26) + 30);
        }
    }
}

void MatrixElement::blipStop(int x)
{
    int y = this->m_blips[x].y();
    this->m_vmap.bits()[x + y * this->m_mapWidth] = 254;
    this->m_cmap.bits()[x + y * this->m_mapWidth] = qrand() % this->nChars();

    this->m_blips[x].setTimer(this->m_blips[x].timer() - 1);

    if (this->m_blips[x].timer() < 0)
        this->m_blips[x].setMode(BlipModeFall);
}

void MatrixElement::blipSlide(int x)
{
    this->m_blips[x].setTimer(this->m_blips[x].timer() - 1);

    if (this->m_blips[x].timer() < 0)
        this->m_blips[x].setMode(BlipModeNone);

    uchar *p = this->m_cmap.bits() + x + this->m_mapWidth * (this->m_mapHeight - 1);
    int dy = this->m_mapWidth * this->m_blips[x].speed();

    for (int y = this->m_mapHeight - this->m_blips[x].speed(); y > 0; y--) {
        *p = *(p - dy);
        p -= this->m_mapWidth;
    }

    for (int y = this->m_blips[x].speed(); y > 0; y--) {
        *p = qrand() % this->nChars();
        p -= this->m_mapWidth;
    }
}

void MatrixElement::updateCharMap()
{
    for (int x = 0; x < this->m_mapWidth; x++) {
        this->darkenColumn(x);

        switch(this->m_blips[x].mode()) {
            default:
            case BlipModeNone:
                this->blipNone(x);
            break;

            case BlipModeFall:
                this->blipFall(x);
            break;

            case BlipModeStop:
                this->blipStop(x);
            break;

            case BlipModeSlid:
                this->blipSlide(x);
            break;
        }
    }
}

// Create edge-enhanced image data from the input
void MatrixElement::createImg(QImage &src)
{
    quint32 *srcBits = (quint32 *) src.bits();
    uchar *q = this->m_img.bits();

    for (int y = 0; y < this->m_mapHeight; y++) {
        quint32 *p = srcBits;

        for (int x = 0; x < this->m_mapWidth; x++) {
            // center, right, below
            quint32 pc = *p;
            quint32 pr = *(p + this->fontWidth() - 1);
            quint32 pb = *(p + src.width() * (this->fontHeight() - 1));

            int r = (int) (pc & 0xff0000) >> 15;
            int g = (int) (pc & 0x00ff00) >> 7;
            int b = (int) (pc & 0x0000ff) * 2;

            uint val = (r + 2 * g + b) >> 5; // val < 64

            r -= (int) (pr & 0xff0000) >> 16;
            g -= (int) (pr & 0x00ff00) >> 8;
            b -= (int) (pr & 0x0000ff);
            r -= (int) (pb & 0xff0000) >> 16;
            g -= (int) (pb & 0x00ff00) >> 8;
            b -= (int) (pb & 0x0000ff);

            val += (r * r + g * g + b * b) >> 5;

            // want not to make blip from the edge.
            if (val > 160)
                val = 160;

            *q = (uchar) val;

            p += this->fontWidth();
            q++;
        }

        srcBits += src.width() * this->fontHeight();
    }
}

void MatrixElement::drawChar(quint32 *dest, uchar c, uchar v, QSize size)
{
    // sticky characters
    if (v == 255)
        v = 160;

    int *p = (int *) &this->m_palette[(int) v * this->fontDepth()];
    uchar *f = (uchar *) &this->m_font.data()[(int) c * this->fontWidth() * this->fontHeight()];

    for (int y = 0; y < this->fontHeight(); y++) {
        for (int x = 0; x < this->fontWidth(); x++) {
            *dest++ = p[*f];
            f++;
        }

        dest += size.width() - this->fontWidth();
    }
}

void MatrixElement::setNChars(int nChars)
{
    this->m_nChars = nChars;
}

void MatrixElement::setFontWidth(int fontWidth)
{
    this->m_fontWidth = fontWidth;
}

void MatrixElement::setFontHeight(int fontHeight)
{
    this->m_fontHeight = fontHeight;
}

void MatrixElement::setFontDepth(int fontDepth)
{
    this->m_fontDepth = fontDepth;
}

void MatrixElement::setMode(int mode)
{
    this->m_mode = mode;
}

void MatrixElement::setWhite(float white)
{
    this->m_white = white;
}

void MatrixElement::setPause(bool pause)
{
    this->m_pause = pause;
}

void MatrixElement::resetNChars()
{
    this->setNChars(80);
}

void MatrixElement::resetFontWidth()
{
    this->setFontWidth(4);
}

void MatrixElement::resetFontHeight()
{
    this->setFontHeight(4);
}

void MatrixElement::resetFontDepth()
{
    this->setFontDepth(4);
}

void MatrixElement::resetMode()
{
    this->setMode(0);
}

void MatrixElement::resetWhite()
{
    this->setWhite(0.45);
}

void MatrixElement::resetPause()
{
    this->setPause(false);
}

QbPacket MatrixElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame(src.size(), src.format());

    if (packet.caps() != this->m_caps) {
        this->m_mapWidth = src.width() / this->fontWidth();
        this->m_mapHeight = src.height() / this->fontHeight();

        this->m_cmap = QImage(this->m_mapWidth, this->m_mapHeight, QImage::Format_Indexed8);
        this->m_vmap = QImage(this->m_mapWidth, this->m_mapHeight, QImage::Format_Indexed8);
        this->m_img = QImage(this->m_mapWidth, this->m_mapHeight, QImage::Format_Indexed8);
        this->m_blips.resize(this->m_mapWidth);

        this->m_cmap.fill(this->nChars() - 1);
        this->m_vmap.fill(0);

        this->setPattern();
        this->setPalette();

        this->m_caps = packet.caps();
    }

    if (!this->pause()) {
        this->updateCharMap();
        this->createImg(src);
    }

    uchar *c = this->m_cmap.bits();
    uchar *v = this->m_vmap.bits();
    uchar *i = this->m_img.bits();

    quint32 *srcBits = (quint32 *) src.bits();
    quint32 *destBits = (quint32 *) oFrame.bits();
    quint32 *p = destBits;

    for (int y = 0; y < this->m_mapHeight; y++) {
        quint32 *q = p;

        for (int x = 0; x < this->m_mapWidth; x++) {
            uint val = *i | *v;
//			if(val > 255) val = 255;
            this->drawChar(q, *c, val, src.size());

            i++;
            v++;
            c++;

            q += this->fontWidth();
        }

        p += src.width() * this->fontHeight();
    }

    int videoArea = src.width() * src.height();

    if (this->mode() == 1)
        for (int x = 0; x < videoArea; x++) {
            quint32 a = *destBits;
            quint32 b = *srcBits++;

            b = (b & 0xfefeff) >> 1;

            *destBits++ = a | b;
        }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
