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
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "radioactiveelement.h"

RadioactiveElement::RadioactiveElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    qRegisterMetaType<QRgb>("QRgb");

    this->m_radiationModeToStr[RadiationModeSoftNormal] = "softNormal";
    this->m_radiationModeToStr[RadiationModeHardNormal] = "hardNormal";
    this->m_radiationModeToStr[RadiationModeSoftColor] = "softColor";
    this->m_radiationModeToStr[RadiationModeHardColor] = "hardColor";

    this->resetMode();
    this->resetBlur();
    this->resetZoom();
    this->resetThreshold();
    this->resetLumaThreshold();
    this->resetAlphaDiff();
    this->resetRadColor();
}

QString RadioactiveElement::mode() const
{
    return this->m_radiationModeToStr[this->m_mode];
}

float RadioactiveElement::blur() const
{
    return this->m_blur;
}

float RadioactiveElement::zoom() const
{
    return this->m_zoom;
}

int RadioactiveElement::threshold() const
{
    return this->m_threshold;
}

int RadioactiveElement::lumaThreshold() const
{
    return this->m_lumaThreshold;
}

int RadioactiveElement::alphaDiff() const
{
    return this->m_alphaDiff;
}

QRgb RadioactiveElement::radColor() const
{
    return this->m_radColor;
}

QImage RadioactiveElement::imageDiff(const QImage &img1,
                                     const QImage &img2,
                                     int threshold,
                                     int lumaThreshold,
                                     QRgb radColor,
                                     RadiationMode mode)
{
    int width = qMin(img1.width(), img2.width());
    int height = qMin(img1.height(), img2.height());
    QImage diff(width, height, img1.format());
    QRgb *img1Bits = (QRgb *) img1.bits();
    QRgb *img2Bits = (QRgb *) img2.bits();
    QRgb *diffBits = (QRgb *) diff.bits();

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

            int alpha = dr * dr + dg * dg + db * db;
            alpha = sqrt(alpha / 3);

            if (mode == RadiationModeSoftNormal
                || mode == RadiationModeSoftColor)
                alpha = alpha < threshold? 0: alpha;
            else
                alpha = alpha < threshold? 0: 255;

            int gray = qGray(img2Bits[i]);

            alpha = gray < lumaThreshold? 0: alpha;

            int r;
            int g;
            int b;

            if (mode == RadiationModeHardNormal
                || mode == RadiationModeSoftNormal) {
                r = r2;
                g = g2;
                b = b2;
            }
            else {
                r = qRed(radColor);
                g = qGreen(radColor);
                b = qBlue(radColor);
            }

            diffBits[i] = qRgba(r, g, b, alpha);
        }
    }

    return diff;
}

QImage RadioactiveElement::imageAlphaDiff(const QImage &src, int alphaDiff)
{
    int videoArea = src.width() * src.height();
    QImage dest(src.size(), src.format());
    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) dest.bits();

    for (int i = 0; i < videoArea; i++) {
        int r = qRed(srcBits[i]);
        int g = qGreen(srcBits[i]);
        int b = qBlue(srcBits[i]);
        int a = qBound(0, qAlpha(srcBits[i]) + alphaDiff, 255);
        destBits[i] = qRgba(r, g, b, a);
    }

    return dest;
}

void RadioactiveElement::setMode(const QString &mode)
{
    if (this->m_radiationModeToStr.values().contains(mode))
        this->m_mode = this->m_radiationModeToStr.key(mode);
    else
        this->m_mode = RadiationModeSoftNormal;
}

void RadioactiveElement::setBlur(float blur)
{
    this->m_blur = blur;
}

void RadioactiveElement::setZoom(float snapTime)
{
    this->m_zoom = snapTime;
}

void RadioactiveElement::setThreshold(int threshold)
{
    this->m_threshold = threshold;
}

void RadioactiveElement::setLumaThreshold(int lumaThreshold)
{
    this->m_lumaThreshold = lumaThreshold;
}

void RadioactiveElement::setAlphaDiff(int alphaDiff)
{
    this->m_alphaDiff = alphaDiff;
}

void RadioactiveElement::setRadColor(QRgb radColor)
{
    this->m_radColor = radColor;
}

void RadioactiveElement::resetMode()
{
    this->setMode("softNormal");
}

void RadioactiveElement::resetBlur()
{
    this->setBlur(1.5);
}

void RadioactiveElement::resetZoom()
{
    this->setZoom(1.1);
}

void RadioactiveElement::resetThreshold()
{
    this->setThreshold(31);
}

void RadioactiveElement::resetLumaThreshold()
{
    this->setLumaThreshold(95);
}

void RadioactiveElement::resetAlphaDiff()
{
    this->setAlphaDiff(-8);
}

void RadioactiveElement::resetRadColor()
{
    this->setRadColor(qRgb(0, 255, 0));
}

QbPacket RadioactiveElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame(src.size(), src.format());

    if (packet.caps() != this->m_caps) {
        this->m_blurZoomBuffer = QImage();
        this->m_prevFrame = QImage();

        this->m_caps = packet.caps();
    }

    if (this->m_prevFrame.isNull()) {
        oFrame = src;
        this->m_blurZoomBuffer = QImage(src.size(), src.format());
        this->m_blurZoomBuffer.fill(qRgba(0, 0, 0, 0));
    }
    else {
        // Compute the difference between previous and current frame,
        // and save it to the buffer.
        QImage diff = this->imageDiff(this->m_prevFrame,
                                      src,
                                      this->m_threshold,
                                      this->m_lumaThreshold,
                                      this->m_radColor,
                                      this->m_mode);

        QPainter painter;
        painter.begin(&this->m_blurZoomBuffer);
        painter.drawImage(0, 0, diff);
        painter.end();

        // Blur buffer.
        QGraphicsScene scene;
        QGraphicsPixmapItem *pixmapItem = scene.addPixmap(QPixmap::fromImage(this->m_blurZoomBuffer));
        QGraphicsBlurEffect *effect = new QGraphicsBlurEffect();
        pixmapItem->setGraphicsEffect(effect);
        effect->setBlurRadius(this->m_blur);

        QImage blur(src.size(), src.format());
        blur.fill(qRgba(0, 0, 0, 0));

        painter.begin(&blur);
        scene.render(&painter);
        painter.end();

        // Zoom buffer.
        QImage blurScaled = blur.scaled(this->m_zoom * blur.size());
        QSize diffSize = blur.size() - blurScaled.size();
        QPoint p(diffSize.width() >> 1,
                 diffSize.height() >> 1);

        QImage zoom(blur.size(), blur.format());
        zoom.fill(qRgba(0, 0, 0, 0));

        painter.begin(&zoom);
        painter.drawImage(p, blurScaled);
        painter.end();

        // Reduce alpha.
        QImage alphaDiff = this->imageAlphaDiff(zoom, this->m_alphaDiff);
        this->m_blurZoomBuffer = alphaDiff;

        // Apply buffer.
        painter.begin(&oFrame);
        painter.drawImage(0, 0, src);
        painter.drawImage(0, 0, this->m_blurZoomBuffer);
        painter.end();
    }

    this->m_prevFrame = src.copy();

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
