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

#include "rippleelement.h"

RippleElement::RippleElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->m_rippleModeToStr[RippleModeMotionDetect] = "motionDetect";
    this->m_rippleModeToStr[RippleModeRain] = "rain";

    this->resetMode();
    this->resetAmplitude();
    this->resetDecay();
    this->resetThreshold();
    this->resetLumaThreshold();
}

QString RippleElement::mode() const
{
    return this->m_rippleModeToStr[this->m_mode];
}

int RippleElement::amplitude() const
{
    return this->m_amplitude;
}

int RippleElement::decay() const
{
    return this->m_decay;
}

int RippleElement::threshold() const
{
    return this->m_threshold;
}

int RippleElement::lumaThreshold() const
{
    return this->m_lumaThreshold;
}

QImage RippleElement::imageDiff(const QImage &img1,
                                const QImage &img2,
                                int threshold,
                                int lumaThreshold,
                                int strength)
{
    int width = qMin(img1.width(), img2.width());
    int height = qMin(img1.height(), img2.height());
    QImage diff(width, height, QImage::Format_ARGB32);
    QRgb *img1Bits = (QRgb *) img1.bits();
    QRgb *img2Bits = (QRgb *) img2.bits();
    int *diffBits = (int *) diff.bits();

    for (int y = 0; y < height; y++) {
        int i = y * width;

        for (int x = 0; x < width; x++, i++) {
            int r1 = qRed(img1Bits[i]);
            int g1 = qGreen(img1Bits[i]);
            int b1 = qBlue(img1Bits[i]);

            int r2 = qRed(img2Bits[i]);
            int g2 = qGreen(img2Bits[i]);
            int b2 = qBlue(img2Bits[i]);

            int dr = r1 - r2;
            int dg = g1 - g2;
            int db = b1 - b2;

            int s = dr * dr + dg * dg + db * db;
            s = sqrt(s / 3);
            s = s < threshold? 0: s;

            int gray = qGray(img2Bits[i]);
            s = gray < lumaThreshold? 0: s;

            diffBits[i] = (strength * s) >> 8;
        }
    }

    return diff;
}

void RippleElement::addDrops(const QImage &buffer, const QImage &drops)
{
    int *bufferBits = (int *) buffer.bits();
    int *dropsBits = (int *) drops.bits();

    for (int y = 0; y < buffer.height(); y++) {
        int xOffset = y * buffer.width();

        for (int x = 0; x < buffer.width(); x++)
            bufferBits[x + xOffset] += dropsBits[x + xOffset];
    }
}

void RippleElement::ripple(const QImage &buffer1, const QImage &buffer2, int decay)
{
    QImage buffer3(buffer1.size(), buffer1.format());
    int *buffer1Bits = (int *) buffer1.bits();
    int *buffer2Bits = (int *) buffer2.bits();
    int *buffer3Bits = (int *) buffer3.bits();
    int widthM1 = buffer1.width() - 1;
    int widthP1 = buffer1.width() + 1;
    int height = buffer1.height() - 1;

    memset(buffer2Bits, 0, buffer1.bytesPerLine());
    memset(buffer2Bits + height * buffer1.width(), 0, buffer1.bytesPerLine());
    memset(buffer3Bits, 0, buffer1.bytesPerLine());
    memset(buffer3Bits + height * buffer1.width(), 0, buffer1.bytesPerLine());

    for (int y = 1; y < height; y++) {
        buffer2Bits[y * buffer1.width()] = 0;
        buffer2Bits[widthM1 + y * buffer1.width()] = 0;
        buffer3Bits[y * buffer1.width()] = 0;
        buffer3Bits[widthM1 + y * buffer1.width()] = 0;
    }

    // Wave simulation.
    for (int y = 1; y < height; y++) {
        int xOfftset = y * buffer1.width();

        for (int x = 1; x < widthM1; x++) {
            int xp = x + xOfftset;
            int h = 0;

            h += buffer1Bits[xp - widthP1];
            h += buffer1Bits[xp - buffer1.width()];
            h += buffer1Bits[xp - widthM1];
            h += buffer1Bits[xp - 1];
            h += buffer1Bits[xp + 1];
            h += buffer1Bits[xp + widthM1];
            h += buffer1Bits[xp + buffer1.width()];
            h += buffer1Bits[xp + widthP1];
            h -= 9 * buffer1Bits[xp];
            h >>= 3;

            int v = buffer1Bits[xp] - buffer2Bits[xp];
            v += h - (v >> decay);
            buffer3Bits[xp] = v + buffer1Bits[xp];
        }
    }

    // Low pass filter.
    for (int y = 1; y < height; y++) {
        int xOfftset = y * buffer1.width();

        for (int x = 1; x < widthM1; x++) {
            int xp = x + xOfftset;

            int h = 0;

            h += buffer3Bits[xp - 1];
            h += buffer3Bits[xp + 1];
            h += buffer3Bits[xp - buffer3.width()];
            h += buffer3Bits[xp + buffer3.width()];
            h += 60 * buffer3Bits[xp];

            buffer2Bits[xp] = h >> 6;
        }
    }
}

QImage RippleElement::applyWater(const QImage &src, const QImage &buffer)
{
    QImage dest(src.size(), src.format());
    QRgb *srcBits = (QRgb *) src.bits();
    int *bufferBits = (int *) buffer.bits();
    QRgb *destBits = (QRgb *) dest.bits();

    for (int y = 0; y < src.height(); y++) {
        int xOfftset = y * src.width();

        for (int x = 0; x < src.width(); x++) {
            int xp = x + xOfftset;

            int xOff = 0;

            if (x > 1
                && x < src.width() - 1) {
                xOff += bufferBits[xp - 1];
                xOff -= bufferBits[xp + 1];
            }

            int yOff = 0;

            if (y > 1
                && y < src.height() - 1) {
                yOff += bufferBits[xp - buffer.width()];
                yOff -= bufferBits[xp + buffer.width()];
            }

            QColor color;
            int xq = qBound(0, x + xOff, src.width() - 1);
            int yq = qBound(0, y + yOff, src.height() - 1);

            color.setRgba(srcBits[xq + yq * src.width()]);

            // Shading
            int lightness = color.lightness() + xOff;
            lightness = qBound(0, lightness, 255);
            color.setHsl(color.hue(), color.saturation(), lightness);

            destBits[xp] = color.rgb();
        }
    }

    return dest;
}

QImage RippleElement::rainDrop(int width, int height, int strength)
{
    if (this->m_period == 0) {
        if (this->m_rainStat == 0) {
            this->m_period = (qrand() >> 23) + 100;
            this->m_dropProb = 0;
            this->m_dropProbIncrement = 0x00ffffff / this->m_period;
            this->m_dropPower = qrand() % (strength << 1) - strength;
            this->m_dropsPerFrameMax = 2 << (qrand() >> 30); // 2,4,8 or 16
            this->m_rainStat = 1;
        }
        else if (this->m_rainStat == 1) {
            this->m_dropProb = 0x00ffffff;
            this->m_dropsPerFrame = 1;
            this->m_dropProbIncrement = 1;
            this->m_period = 16 * (this->m_dropsPerFrameMax - 1);
            this->m_rainStat = 2;
        }
        else if (this->m_rainStat == 2) {
            m_period = (qrand() >> 22) + 1000;
            m_dropProbIncrement = 0;
            m_rainStat = 3;
        }
        else if (this->m_rainStat == 3) {
            this->m_period = 16 * (this->m_dropsPerFrameMax - 1);
            this->m_dropProbIncrement = -1;
            this->m_rainStat = 4;
        }
        else if (this->m_rainStat == 4) {
            this->m_period = (qrand() >> 24) + 60;
            this->m_dropProbIncrement = -this->m_dropProb / this->m_period;
            this->m_rainStat = 5;
        }
        else {
            this->m_period = (qrand() >> 23) + 500;
            this->m_dropProb = 0;
            this->m_rainStat = 0;
        }
    }

    QImage rain;

    if (this->m_rainStat == 1
        || this->m_rainStat == 5) {

        if ((qrand() >> 8) < (int) this->m_dropProb)
            rain = this->drop(width, height, this->m_dropPower);

        this->m_dropProb += this->m_dropProbIncrement;
    }
    else if (this->m_rainStat == 2
             || this->m_rainStat == 3
             || this->m_rainStat == 4) {
        for  (int i = this->m_dropsPerFrame / 16; i > 0; i--)
            rain = this->drop(width, height, this->m_dropPower);

        this->m_dropsPerFrame += this->m_dropProbIncrement;
    }

    this->m_period--;

    if (rain.isNull()) {
        rain = QImage(width, height, QImage::Format_ARGB32);
        rain.fill(qRgba(0, 0, 0, 0));
    }

    return rain;
}

void RippleElement::setMode(const QString &mode)
{
    if (this->m_rippleModeToStr.values().contains(mode))
        this->m_mode = this->m_rippleModeToStr.key(mode);
    else
        this->m_mode = RippleModeMotionDetect;
}

void RippleElement::setAmplitude(int amplitude)
{
    this->m_amplitude = amplitude;
}

void RippleElement::setDecay(int decay)
{
    this->m_decay = decay;
}

void RippleElement::setThreshold(int threshold)
{
    this->m_threshold = threshold;
}

void RippleElement::setLumaThreshold(int lumaThreshold)
{
    this->m_lumaThreshold = lumaThreshold;
}

void RippleElement::resetMode()
{
    this->setMode("motionDetect");
}

void RippleElement::resetAmplitude()
{
    this->setAmplitude(256);
}

void RippleElement::resetDecay()
{
    this->setDecay(8);
}

void RippleElement::resetThreshold()
{
    this->setThreshold(15);
}

void RippleElement::resetLumaThreshold()
{
    this->setLumaThreshold(15);
}

void RippleElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void RippleElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void RippleElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_ARGB32);

    QImage oFrame(src.size(), src.format());

    if (packet.caps() != this->m_caps) {
        this->m_prevFrame = QImage();

        this->m_period = 0;
        this->m_rainStat = 0;
        this->m_dropProb = 0;
        this->m_dropProbIncrement = 0;
        this->m_dropsPerFrameMax = 0;
        this->m_dropsPerFrame = 0;
        this->m_dropPower = 0;

        this->m_caps = packet.caps();
    }

    if (this->m_prevFrame.isNull()) {
        oFrame = src;
        this->m_rippleBuffer.clear();
        this->m_rippleBuffer << QImage(src.size(), src.format());
        this->m_rippleBuffer[0].fill(qRgba(0, 0, 0, 0));
        this->m_rippleBuffer << QImage(src.size(), src.format());
        this->m_rippleBuffer[1].fill(qRgba(0, 0, 0, 0));
        this->m_curRippleBuffer = 0;
    }
    else {
        QImage drops;

        if (this->m_mode == RippleModeMotionDetect)
            // Compute the difference between previous and current frame,
            // and save it to the buffer.
            drops = this->imageDiff(this->m_prevFrame,
                                    src,
                                    this->m_threshold,
                                    this->m_lumaThreshold,
                                    this->m_amplitude);
        else
            drops = this->rainDrop(width, height, this->m_amplitude);

        this->addDrops(this->m_rippleBuffer[this->m_curRippleBuffer], drops);
        this->addDrops(this->m_rippleBuffer[1 - this->m_curRippleBuffer], drops);

        this->ripple(this->m_rippleBuffer[this->m_curRippleBuffer],
                     this->m_rippleBuffer[1 - this->m_curRippleBuffer],
                     this->m_decay);

        // Apply buffer.
        oFrame = this->applyWater(src, this->m_rippleBuffer[this->m_curRippleBuffer]);
        this->m_curRippleBuffer = 1 - this->m_curRippleBuffer;
    }

    this->m_prevFrame = src.copy();

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
