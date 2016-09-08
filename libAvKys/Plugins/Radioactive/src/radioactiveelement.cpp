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

#include "radioactiveelement.h"

typedef QMap<RadioactiveElement::RadiationMode, QString> RadiationModeMap;

inline RadiationModeMap initRadiationModeMap()
{
    RadiationModeMap radiationModeToStr;
    radiationModeToStr[RadioactiveElement::RadiationModeSoftNormal] = "softNormal";
    radiationModeToStr[RadioactiveElement::RadiationModeHardNormal] = "hardNormal";
    radiationModeToStr[RadioactiveElement::RadiationModeSoftColor] = "softColor";
    radiationModeToStr[RadioactiveElement::RadiationModeHardColor] = "hardColor";

    return radiationModeToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(RadiationModeMap, radiationModeToStr, (initRadiationModeMap()))

RadioactiveElement::RadioactiveElement(): AkElement()
{
    this->m_mode = RadiationModeSoftNormal;
    this->m_zoom = 1.1;
    this->m_threshold = 31;
    this->m_lumaThreshold = 95;
    this->m_alphaDiff = -8;
    this->m_radColor = qRgb(0, 255, 0);
    this->m_blurFilter = AkElement::create("Blur");
    this->m_blurFilter->setProperty("radius", 2);

    QObject::connect(this->m_blurFilter.data(),
                     SIGNAL(radiusChanged(int)),
                     this,
                     SIGNAL(blurChanged(int)));
}

QObject *RadioactiveElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Radioactive/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Radioactive", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
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

QString RadioactiveElement::mode() const
{
    return radiationModeToStr->value(this->m_mode);
}

int RadioactiveElement::blur() const
{
    return this->m_blurFilter->property("radius").toInt();
}

qreal RadioactiveElement::zoom() const
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

            if (mode == RadiationModeSoftNormal
                || mode == RadiationModeSoftColor)
                alpha = alpha < threshold? 0: alpha;
            else
                alpha = alpha < threshold? 0: 255;

            int gray = qGray(iLine2[x]);

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

            oLine[x] = qRgba(r, g, b, alpha);
        }
    }

    return diff;
}

QImage RadioactiveElement::imageAlphaDiff(const QImage &src, int alphaDiff)
{
    QImage dest(src.size(), src.format());

    for (int y = 0; y < src.height(); y++) {
        const QRgb *srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        QRgb *dstLine = reinterpret_cast<QRgb *>(dest.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            QRgb pixel = srcLine[x];
            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);
            int a = qBound(0, qAlpha(pixel) + alphaDiff, 255);
            dstLine[x] = qRgba(r, g, b, a);
        }
    }

    return dest;
}

void RadioactiveElement::setMode(const QString &mode)
{
    RadiationMode modeEnum = radiationModeToStr->key(mode,
                                                     RadiationModeSoftNormal);

    if (this->m_mode == modeEnum)
        return;

    this->m_mode = modeEnum;
    emit this->modeChanged(mode);
}

void RadioactiveElement::setBlur(int blur)
{
    this->m_blurFilter->setProperty("radius", blur);
}

void RadioactiveElement::setZoom(qreal zoom)
{
    if (qFuzzyCompare(this->m_zoom, zoom))
        return;

    this->m_zoom = zoom;
    emit this->zoomChanged(zoom);
}

void RadioactiveElement::setThreshold(int threshold)
{
    if (this->m_threshold == threshold)
        return;

    this->m_threshold = threshold;
    emit this->thresholdChanged(threshold);
}

void RadioactiveElement::setLumaThreshold(int lumaThreshold)
{
    if (this->m_lumaThreshold == lumaThreshold)
        return;

    this->m_lumaThreshold = lumaThreshold;
    emit this->lumaThresholdChanged(lumaThreshold);
}

void RadioactiveElement::setAlphaDiff(int alphaDiff)
{
    if (this->m_alphaDiff == alphaDiff)
        return;

    this->m_alphaDiff = alphaDiff;
    emit this->alphaDiffChanged(alphaDiff);
}

void RadioactiveElement::setRadColor(QRgb radColor)
{
    if (this->m_radColor == radColor)
        return;

    this->m_radColor = radColor;
    emit this->radColorChanged(radColor);
}

void RadioactiveElement::resetMode()
{
    this->setMode("softNormal");
}

void RadioactiveElement::resetBlur()
{
    this->setBlur(2);
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

AkPacket RadioactiveElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());

    if (src.size() != this->m_frameSize) {
        this->m_blurZoomBuffer = QImage();
        this->m_prevFrame = QImage();
        this->m_frameSize = src.size();
    }

    if (this->m_prevFrame.isNull()) {
        oFrame = src;
        this->m_blurZoomBuffer = QImage(src.size(), src.format());
        this->m_blurZoomBuffer.fill(qRgba(0, 0, 0, 0));
    } else {
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
        AkPacket blurZoomPacket = AkUtils::imageToPacket(this->m_blurZoomBuffer, packet);
        AkPacket blurPacket = this->m_blurFilter->iStream(blurZoomPacket);
        QImage blur = AkUtils::packetToImage(blurPacket);

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

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
