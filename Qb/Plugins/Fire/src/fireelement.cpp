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

#include "fireelement.h"

FireElement::FireElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->m_fireModeToStr[FireModeSoft] = "soft";
    this->m_fireModeToStr[FireModeHard] = "hard";

    this->m_palette = this->createPalette();

    this->resetMode();
    this->resetCool();
    this->resetDisolve();
    this->resetBlur();
    this->resetZoom();
    this->resetThreshold();
    this->resetLumaThreshold();
    this->resetAlphaDiff();
    this->resetAlphaVariation();
    this->resetNColors();
}

QObject *FireElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Fire/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Fire", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

QString FireElement::mode() const
{
    return this->m_fireModeToStr[this->m_mode];
}

int FireElement::cool() const
{
    return this->m_cool;
}

float FireElement::disolve() const
{
    return this->m_disolve;
}

float FireElement::blur() const
{
    return this->m_blur;
}

float FireElement::zoom() const
{
    return this->m_zoom;
}

int FireElement::threshold() const
{
    return this->m_threshold;
}

int FireElement::lumaThreshold() const
{
    return this->m_lumaThreshold;
}

int FireElement::alphaDiff() const
{
    return this->m_alphaDiff;
}

int FireElement::alphaVariation() const
{
    return this->m_alphaVariation;
}

int FireElement::nColors() const
{
    return this->m_nColors;
}

QImage FireElement::imageDiff(const QImage &img1,
                            const QImage &img2,
                            int colors,
                            int threshold,
                            int lumaThreshold,
                            int alphaVariation,
                            FireMode mode)
{
    int width = qMin(img1.width(), img2.width());
    int height = qMin(img1.height(), img2.height());
    QImage diff(width, height, QImage::Format_ARGB32);
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

            if (mode == FireModeSoft)
                alpha = alpha < threshold? 0: alpha;
            else
                alpha = alpha < threshold?
                            0: (256 - alphaVariation)
                            + qrand() % alphaVariation;

            int gray = qGray(img2Bits[i]);

            alpha = gray < lumaThreshold? 0: alpha;
            int b = (256 - colors) + qrand() % colors;

            diffBits[i] = qRgba(0, 0, b, alpha);
        }
    }

    return diff;
}

QImage FireElement::zoomImage(const QImage &src, float factor)
{
    QImage scaled = src.scaled(src.width(),
                               (1 + factor) * src.height());

    QPoint p(0, src.height() - scaled.height());

    QImage zoom(src.size(), src.format());
    zoom.fill(qRgba(0, 0, 0, 0));

    QPainter painter;
    painter.begin(&zoom);
    painter.drawImage(p, src);
    painter.end();

    return zoom;
}

void FireElement::coolImage(const QImage &src, int colorDiff)
{
    int videoArea = src.width() * src.height();
    QRgb *srcBits = (QRgb *) src.bits();

    for (int i = 0; i < videoArea; i++) {
        int b = qBound(0, qBlue(srcBits[i]) + colorDiff, 255);
        srcBits[i] = qRgba(0, 0, b, qAlpha(srcBits[i]));
    }
}

void FireElement::imageAlphaDiff(const QImage &src, int alphaDiff)
{
    int videoArea = src.width() * src.height();
    QRgb *srcBits = (QRgb *) src.bits();

    for (int i = 0; i < videoArea; i++) {
        QRgb pixel = srcBits[i];
        int b = qBlue(pixel);
        int a = qBound(0, qAlpha(pixel) + alphaDiff, 255);
        srcBits[i] = qRgba(0, 0, b, a);
    }
}

void FireElement::disolveImage(const QImage &src, float amount)
{
    int videoArea = src.width() * src.height();
    int n = amount * videoArea;
    QRgb *srcBits = (QRgb *) src.bits();

    for (int i = 0; i < n; i++) {
        int index = qrand() % videoArea;
        QRgb pixel = srcBits[index];
        int b = qBlue(pixel);
        int a = qAlpha(pixel) < 1? 0: qrand() % qAlpha(pixel);

        srcBits[index] = qRgba(0, 0, b, a);
    }
}

QImage FireElement::blurImage(const QImage &src, float factor)
{
    QGraphicsScene scene;
    QGraphicsPixmapItem *pixmapItem = scene.addPixmap(QPixmap::fromImage(src));
    QGraphicsBlurEffect *effect = new QGraphicsBlurEffect();
    pixmapItem->setGraphicsEffect(effect);
    effect->setBlurRadius(factor);

    QImage blur(src.size(), src.format());
    blur.fill(qRgba(0, 0, 0, 0));

    QPainter painter;
    painter.begin(&blur);
    scene.render(&painter);
    painter.end();

    return blur;
}

QImage FireElement::burn(const QImage &src, const QVector<QRgb> &palette)
{
    int videoArea = src.width() * src.height();
    QImage dest(src.size(), src.format());
    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) dest.bits();

    for (int i = 0; i < videoArea; i++) {
        int index = qBlue(srcBits[i]);
        int r = qRed(palette[index]);
        int g = qGreen(palette[index]);
        int b = qBlue(palette[index]);
        int a = qAlpha(srcBits[i]);
        destBits[i] = qRgba(r, g, b, a);
    }

    return dest;
}

QVector<QRgb> FireElement::createPalette()
{
    QVector<QRgb> palette;

    for (int i = 0; i < 128; i++)
        palette << qRgb(255,
                        (3 * i +  128) >> 1,
                        i >> 1);

    for (int i = 0; i < 128; i++)
        palette << qRgb(255,
                        255,
                        (3 * i +  128) >> 1);

    return palette;
}

void FireElement::setMode(const QString &mode)
{
    FireMode modeEnum = this->m_fireModeToStr.values().contains(mode)?
                            this->m_fireModeToStr.key(mode):
                            FireModeHard;

    if (modeEnum != this->m_mode) {
        this->m_mode = modeEnum;
        emit this->modeChanged();
    }
}

void FireElement::setCool(int cool)
{
    if (cool != this->m_cool) {
        this->m_cool = cool;
        emit this->coolChanged();
    }
}

void FireElement::setDisolve(float disolve)
{
    if (disolve != this->m_disolve) {
        this->m_disolve = disolve;
        emit this->disolveChanged();
    }
}

void FireElement::setBlur(float blur)
{
    if (blur != this->m_blur) {
        this->m_blur = blur;
        emit this->blurChanged();
    }
}

void FireElement::setZoom(float zoom)
{
    if (zoom != this->m_zoom) {
        this->m_zoom = zoom;
        emit this->zoomChanged();
    }
}

void FireElement::setThreshold(int threshold)
{
    if (threshold != this->m_threshold) {
        this->m_threshold = threshold;
        emit this->thresholdChanged();
    }
}

void FireElement::setLumaThreshold(int lumaThreshold)
{
    if (lumaThreshold != this->m_lumaThreshold) {
        this->m_lumaThreshold = lumaThreshold;
        emit this->lumaThresholdChanged();
    }
}

void FireElement::setAlphaDiff(int alphaDiff)
{
    if (alphaDiff != this->m_alphaDiff) {
        this->m_alphaDiff = alphaDiff;
        emit this->alphaDiffChanged();
    }
}

void FireElement::setAlphaVariation(int alphaVariation)
{
    if (alphaVariation != this->m_alphaVariation) {
        this->m_alphaVariation = alphaVariation;
        emit this->alphaVariationChanged();
    }
}

void FireElement::setNColors(int nColors)
{
    if (nColors < 1)
        nColors = 1;

    if (nColors != this->m_nColors) {
        this->m_nColors = nColors;
        emit this->nColorsChanged();
    }
}

void FireElement::resetMode()
{
    this->setMode("hard");
}

void FireElement::resetCool()
{
    this->setCool(-16);
}

void FireElement::resetDisolve()
{
    this->setDisolve(0.01);
}

void FireElement::resetBlur()
{
    this->setBlur(1.5);
}

void FireElement::resetZoom()
{
    this->setZoom(0.02);
}

void FireElement::resetThreshold()
{
    this->setThreshold(15);
}

void FireElement::resetLumaThreshold()
{
    this->setLumaThreshold(15);
}

void FireElement::resetAlphaDiff()
{
    this->setAlphaDiff(-12);
}

void FireElement::resetAlphaVariation()
{
    this->setAlphaVariation(127);
}

void FireElement::resetNColors()
{
    this->setNColors(8);
}

QbPacket FireElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame(src.size(), src.format());

    if (packet.caps() != this->m_caps) {
        this->m_fireBuffer = QImage();
        this->m_prevFrame = QImage();

        this->m_caps = packet.caps();
    }

    if (this->m_prevFrame.isNull()) {
        oFrame = src;
        this->m_fireBuffer = QImage(src.size(), src.format());
        this->m_fireBuffer.fill(qRgba(0, 0, 0, 0));
    }
    else {
        this->m_fireBuffer = this->zoomImage(this->m_fireBuffer, this->m_zoom);
        this->coolImage(this->m_fireBuffer, this->m_cool);
        this->imageAlphaDiff(this->m_fireBuffer, this->m_alphaDiff);
        this->disolveImage(this->m_fireBuffer, this->m_disolve);

        // Compute the difference between previous and current frame,
        // and save it to the buffer.
        QImage diff = this->imageDiff(this->m_prevFrame,
                                      src,
                                      this->m_nColors,
                                      this->m_threshold,
                                      this->m_lumaThreshold,
                                      this->m_alphaVariation,
                                      this->m_mode);

        QPainter painter;
        painter.begin(&this->m_fireBuffer);
        painter.drawImage(0, 0, diff);
        painter.end();

        this->m_fireBuffer = this->blurImage(this->m_fireBuffer, this->m_blur);

        // Apply buffer.
        painter.begin(&oFrame);
        painter.drawImage(0, 0, src);
        painter.drawImage(0, 0, this->burn(this->m_fireBuffer, this->m_palette));
        painter.end();
    }

    this->m_prevFrame = src.copy();

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
