/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#include <QtMath>
#include <QPainter>

#include "fireelement.h"

typedef QMap<FireElement::FireMode, QString> FireModeMap;

inline FireModeMap initFireModeMap()
{
    FireModeMap fireModeToStr;
    fireModeToStr[FireElement::FireModeSoft] = "soft";
    fireModeToStr[FireElement::FireModeHard] = "hard";

    return fireModeToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(FireModeMap, fireModeToStr, (initFireModeMap()))

FireElement::FireElement(): AkElement()
{
    this->m_mode = FireModeHard;
    this->m_cool = -16;
    this->m_disolve = 0.01;
    this->m_zoom = 0.02;
    this->m_threshold = 15;
    this->m_lumaThreshold = 15;
    this->m_alphaDiff = -12;
    this->m_alphaVariation = 127;
    this->m_nColors = 8;

    this->m_palette = this->createPalette();
    this->m_blurFilter = AkElement::create("Blur");
    this->m_blurFilter->setProperty("radius", 2);

    QObject::connect(this->m_blurFilter.data(),
                     SIGNAL(radiusChanged(int)),
                     this,
                     SIGNAL(blurChanged(int)));
}

QObject *FireElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Fire/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Fire", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);

    if (!item) {
        delete context;

        return NULL;
    }

    context->setParent(item);

    return item;
}

QString FireElement::mode() const
{
    return fireModeToStr->value(this->m_mode);
}

int FireElement::cool() const
{
    return this->m_cool;
}

qreal FireElement::disolve() const
{
    return this->m_disolve;
}

int FireElement::blur() const
{
    return this->m_blurFilter->property("radius").toInt();
}

qreal FireElement::zoom() const
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

    for (int y = 0; y < height; y++) {
        const QRgb *iLine1 = reinterpret_cast<const QRgb *>(img1.constScanLine(y));
        const QRgb *iLine2 = reinterpret_cast<const QRgb *>(img2.constScanLine(y));
        QRgb *oLine = reinterpret_cast<QRgb *>(diff.scanLine(y));

        for (int x = 0; x < width; x++) {
            int r1 = qRed(iLine1[x]);
            int g1 = qGreen(iLine1[x]);
            int b1 = qBlue(iLine1[x]);

            int r2 = qRed(iLine2[x]);
            int g2 = qGreen(iLine2[x]);
            int b2 = qBlue(iLine2[x]);

            int dr = r1 - r2;
            int dg = g1 - g2;
            int db = b1 - b2;

            int alpha = dr * dr + dg * dg + db * db;
            alpha = int(sqrt(alpha / 3));

            if (mode == FireModeSoft)
                alpha = alpha < threshold? 0: alpha;
            else
                alpha = alpha < threshold?
                            0: (256 - alphaVariation)
                            + qrand() % alphaVariation;

            int gray = qGray(iLine2[x]);

            alpha = gray < lumaThreshold? 0: alpha;
            int b = (256 - colors) + qrand() % colors;

            oLine[x] = qRgba(0, 0, b, alpha);
        }
    }

    return diff;
}

QImage FireElement::zoomImage(const QImage &src, qreal factor)
{
    QImage scaled = src.scaled(src.width(),
                               int((1 + factor) * src.height()));

    QPoint p(0, src.height() - scaled.height());

    QImage zoom(src.size(), src.format());
    zoom.fill(qRgba(0, 0, 0, 0));

    QPainter painter;
    painter.begin(&zoom);
    painter.drawImage(p, src);
    painter.end();

    return zoom;
}

void FireElement::coolImage(QImage &src, int colorDiff)
{
    for (int y = 0; y < src.height(); y++) {
        QRgb *srcLine = reinterpret_cast<QRgb *>(src.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            int b = qBound(0, qBlue(srcLine[x]) + colorDiff, 255);
            srcLine[x] = qRgba(0, 0, b, qAlpha(srcLine[x]));
        }
    }
}

void FireElement::imageAlphaDiff(QImage &src, int alphaDiff)
{
    for (int y = 0; y < src.height(); y++) {
        QRgb *srcLine = reinterpret_cast<QRgb *>(src.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            QRgb pixel = srcLine[x];
            int b = qBlue(pixel);
            int a = qBound(0, qAlpha(pixel) + alphaDiff, 255);
            srcLine[x] = qRgba(0, 0, b, a);
        }
    }
}

void FireElement::disolveImage(QImage &src, qreal amount)
{
    qint64 videoArea = src.width() * src.height();
    qint64 n = qint64(amount * videoArea);

    for (qint64 i = 0; i < n; i++) {
        int x = qrand() % src.width();
        int y = qrand() % src.height();
        QRgb pixel = src.pixel(x, y);
        int b = qBlue(pixel);
        int a = qAlpha(pixel) < 1? 0: qrand() % qAlpha(pixel);

        src.setPixel(x, y, qRgba(0, 0, b, a));
    }
}

QImage FireElement::burn(const QImage &src, const QVector<QRgb> &palette)
{
    QImage dest(src.size(), src.format());

    for (int y = 0; y < src.height(); y++) {
        const QRgb *srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        QRgb *dstLine = reinterpret_cast<QRgb *>(dest.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            int index = qBlue(srcLine[x]);
            int r = qRed(palette[index]);
            int g = qGreen(palette[index]);
            int b = qBlue(palette[index]);

            dstLine[x] = qRgba(r, g, b, qAlpha(srcLine[x]));
        }
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
    FireMode modeEnum = fireModeToStr->key(mode, FireModeHard);

    if (this->m_mode == modeEnum)
        return;

    this->m_mode = modeEnum;
    emit this->modeChanged(mode);
}

void FireElement::setCool(int cool)
{
    if (this->m_cool == cool)
        return;

    this->m_cool = cool;
    emit this->coolChanged(cool);
}

void FireElement::setDisolve(qreal disolve)
{
    if (qFuzzyCompare(this->m_disolve, disolve))
        return;

    this->m_disolve = disolve;
    emit this->disolveChanged(disolve);
}

void FireElement::setBlur(int blur)
{
    this->m_blurFilter->setProperty("radius", blur);
}

void FireElement::setZoom(qreal zoom)
{
    if (qFuzzyCompare(this->m_zoom, zoom))
        return;

    this->m_zoom = zoom;
    emit this->zoomChanged(zoom);
}

void FireElement::setThreshold(int threshold)
{
    if (this->m_threshold == threshold)
        return;

    this->m_threshold = threshold;
    emit this->thresholdChanged(threshold);
}

void FireElement::setLumaThreshold(int lumaThreshold)
{
    if (this->m_lumaThreshold == lumaThreshold)
        return;

    this->m_lumaThreshold = lumaThreshold;
    emit this->lumaThresholdChanged(lumaThreshold);
}

void FireElement::setAlphaDiff(int alphaDiff)
{
    if (this->m_alphaDiff == alphaDiff)
        return;

    this->m_alphaDiff = alphaDiff;
    emit this->alphaDiffChanged(alphaDiff);
}

void FireElement::setAlphaVariation(int alphaVariation)
{
    if (this->m_alphaVariation == alphaVariation)
        return;

    this->m_alphaVariation = alphaVariation;
    emit this->alphaVariationChanged(alphaVariation);
}

void FireElement::setNColors(int nColors)
{
    if (this->m_nColors == nColors)
        return;

    this->m_nColors = nColors;
    emit this->nColorsChanged(nColors);
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
    this->setBlur(2);
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

AkPacket FireElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());

    if (src.size() != this->m_framSize) {
        this->m_fireBuffer = QImage();
        this->m_prevFrame = QImage();
        this->m_framSize = src.size();
    }

    if (this->m_prevFrame.isNull()) {
        oFrame = src;
        this->m_fireBuffer = QImage(src.size(), src.format());
        this->m_fireBuffer.fill(qRgba(0, 0, 0, 0));
    } else {
        this->m_fireBuffer = this->zoomImage(this->m_fireBuffer, this->m_zoom);
        this->coolImage(this->m_fireBuffer, this->m_cool);
        this->imageAlphaDiff(this->m_fireBuffer, this->m_alphaDiff);
        this->disolveImage(this->m_fireBuffer, this->m_disolve);

        int nColors = this->m_nColors > 0? this->m_nColors: 1;

        // Compute the difference between previous and current frame,
        // and save it to the buffer.
        QImage diff = this->imageDiff(this->m_prevFrame,
                                      src,
                                      nColors,
                                      this->m_threshold,
                                      this->m_lumaThreshold,
                                      this->m_alphaVariation,
                                      this->m_mode);

        QPainter painter;
        painter.begin(&this->m_fireBuffer);
        painter.drawImage(0, 0, diff);
        painter.end();

        AkPacket firePacket = AkUtils::imageToPacket(this->m_fireBuffer, packet);
        AkPacket blurPacket = this->m_blurFilter->iStream(firePacket);
        this->m_fireBuffer = AkUtils::packetToImage(blurPacket);

        // Apply buffer.
        painter.begin(&oFrame);
        painter.drawImage(0, 0, src);
        painter.drawImage(0, 0, this->burn(this->m_fireBuffer, this->m_palette));
        painter.end();
    }

    this->m_prevFrame = src.copy();

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
