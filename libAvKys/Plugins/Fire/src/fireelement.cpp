/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#include <QVariant>
#include <QMap>
#include <QPainter>
#include <QQmlContext>
#include <QtMath>
#include <akutils.h>
#include <akpacket.h>

#include "fireelement.h"

typedef QMap<FireElement::FireMode, QString> FireModeMap;

inline FireModeMap initFireModeMap()
{
    FireModeMap fireModeToStr {
        {FireElement::FireModeSoft, "soft"},
        {FireElement::FireModeHard, "hard"}
    };

    return fireModeToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(FireModeMap, fireModeToStr, (initFireModeMap()))

class FireElementPrivate
{
    public:
        FireElement::FireMode m_mode;
        int m_cool;
        qreal m_dissolve;
        qreal m_zoom;
        int m_threshold;
        int m_lumaThreshold;
        int m_alphaDiff;
        int m_alphaVariation;
        int m_nColors;
        QSize m_framSize;
        QImage m_prevFrame;
        QImage m_fireBuffer;
        QVector<QRgb> m_palette;
        AkElementPtr m_blurFilter;

        FireElementPrivate():
            m_mode(FireElement::FireModeHard),
            m_cool(-16),
            m_dissolve(0.01),
            m_zoom(0.02),
            m_threshold(15),
            m_lumaThreshold(15),
            m_alphaDiff(-12),
            m_alphaVariation(127),
            m_nColors(8)
        {
        }

        inline QImage imageDiff(const QImage &img1,
                                const QImage &img2,
                                int colors,
                                int threshold,
                                int lumaThreshold,
                                int alphaVariation,
                                FireElement::FireMode mode);
        inline QImage zoomImage(const QImage &src, qreal factor);
        inline void coolImage(QImage &src, int colorDiff);
        inline void imageAlphaDiff(QImage &src, int alphaDiff);
        inline void dissolveImage(QImage &src, qreal amount);
        inline QImage burn(const QImage &src, const QVector<QRgb> &palette);
        inline QVector<QRgb> createPalette();
};

FireElement::FireElement(): AkElement()
{
    this->d = new FireElementPrivate;
    this->d->m_palette = this->d->createPalette();
    this->d->m_blurFilter = AkElement::create("Blur");
    this->d->m_blurFilter->setProperty("radius", 2);

    QObject::connect(this->d->m_blurFilter.data(),
                     SIGNAL(radiusChanged(int)),
                     this,
                     SIGNAL(blurChanged(int)));
}

FireElement::~FireElement()
{
    delete this->d;
}

QString FireElement::mode() const
{
    return fireModeToStr->value(this->d->m_mode);
}

int FireElement::cool() const
{
    return this->d->m_cool;
}

qreal FireElement::dissolve() const
{
    return this->d->m_dissolve;
}

int FireElement::blur() const
{
    return this->d->m_blurFilter->property("radius").toInt();
}

qreal FireElement::zoom() const
{
    return this->d->m_zoom;
}

int FireElement::threshold() const
{
    return this->d->m_threshold;
}

int FireElement::lumaThreshold() const
{
    return this->d->m_lumaThreshold;
}

int FireElement::alphaDiff() const
{
    return this->d->m_alphaDiff;
}

int FireElement::alphaVariation() const
{
    return this->d->m_alphaVariation;
}

int FireElement::nColors() const
{
    return this->d->m_nColors;
}

QImage FireElementPrivate::imageDiff(const QImage &img1,
                                     const QImage &img2,
                                     int colors,
                                     int threshold,
                                     int lumaThreshold,
                                     int alphaVariation,
                                     FireElement::FireMode mode)
{
    int width = qMin(img1.width(), img2.width());
    int height = qMin(img1.height(), img2.height());
    QImage diff(width, height, QImage::Format_ARGB32);

    for (int y = 0; y < height; y++) {
        auto iLine1 = reinterpret_cast<const QRgb *>(img1.constScanLine(y));
        auto iLine2 = reinterpret_cast<const QRgb *>(img2.constScanLine(y));
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

            if (mode == FireElement::FireModeSoft)
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

QImage FireElementPrivate::zoomImage(const QImage &src, qreal factor)
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

void FireElementPrivate::coolImage(QImage &src, int colorDiff)
{
    for (int y = 0; y < src.height(); y++) {
        QRgb *srcLine = reinterpret_cast<QRgb *>(src.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            int b = qBound(0, qBlue(srcLine[x]) + colorDiff, 255);
            srcLine[x] = qRgba(0, 0, b, qAlpha(srcLine[x]));
        }
    }
}

void FireElementPrivate::imageAlphaDiff(QImage &src, int alphaDiff)
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

void FireElementPrivate::dissolveImage(QImage &src, qreal amount)
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

QImage FireElementPrivate::burn(const QImage &src, const QVector<QRgb> &palette)
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

QVector<QRgb> FireElementPrivate::createPalette()
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

QString FireElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Fire/share/qml/main.qml");
}

void FireElement::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Fire", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void FireElement::setMode(const QString &mode)
{
    FireMode modeEnum = fireModeToStr->key(mode, FireModeHard);

    if (this->d->m_mode == modeEnum)
        return;

    this->d->m_mode = modeEnum;
    emit this->modeChanged(mode);
}

void FireElement::setCool(int cool)
{
    if (this->d->m_cool == cool)
        return;

    this->d->m_cool = cool;
    emit this->coolChanged(cool);
}

void FireElement::setDissolve(qreal dissolve)
{
    if (qFuzzyCompare(this->d->m_dissolve, dissolve))
        return;

    this->d->m_dissolve = dissolve;
    emit this->dissolveChanged(dissolve);
}

void FireElement::setBlur(int blur)
{
    this->d->m_blurFilter->setProperty("radius", blur);
}

void FireElement::setZoom(qreal zoom)
{
    if (qFuzzyCompare(this->d->m_zoom, zoom))
        return;

    this->d->m_zoom = zoom;
    emit this->zoomChanged(zoom);
}

void FireElement::setThreshold(int threshold)
{
    if (this->d->m_threshold == threshold)
        return;

    this->d->m_threshold = threshold;
    emit this->thresholdChanged(threshold);
}

void FireElement::setLumaThreshold(int lumaThreshold)
{
    if (this->d->m_lumaThreshold == lumaThreshold)
        return;

    this->d->m_lumaThreshold = lumaThreshold;
    emit this->lumaThresholdChanged(lumaThreshold);
}

void FireElement::setAlphaDiff(int alphaDiff)
{
    if (this->d->m_alphaDiff == alphaDiff)
        return;

    this->d->m_alphaDiff = alphaDiff;
    emit this->alphaDiffChanged(alphaDiff);
}

void FireElement::setAlphaVariation(int alphaVariation)
{
    if (this->d->m_alphaVariation == alphaVariation)
        return;

    this->d->m_alphaVariation = alphaVariation;
    emit this->alphaVariationChanged(alphaVariation);
}

void FireElement::setNColors(int nColors)
{
    if (this->d->m_nColors == nColors)
        return;

    this->d->m_nColors = nColors;
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

void FireElement::resetDissolve()
{
    this->setDissolve(0.01);
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

    if (src.size() != this->d->m_framSize) {
        this->d->m_fireBuffer = QImage();
        this->d->m_prevFrame = QImage();
        this->d->m_framSize = src.size();
    }

    if (this->d->m_prevFrame.isNull()) {
        oFrame = src;
        this->d->m_fireBuffer = QImage(src.size(), src.format());
        this->d->m_fireBuffer.fill(qRgba(0, 0, 0, 0));
    } else {
        this->d->m_fireBuffer = this->d->zoomImage(this->d->m_fireBuffer,
                                                   this->d->m_zoom);
        this->d->coolImage(this->d->m_fireBuffer, this->d->m_cool);
        this->d->imageAlphaDiff(this->d->m_fireBuffer, this->d->m_alphaDiff);
        this->d->dissolveImage(this->d->m_fireBuffer, this->d->m_dissolve);

        int nColors = this->d->m_nColors > 0? this->d->m_nColors: 1;

        // Compute the difference between previous and current frame,
        // and save it to the buffer.
        QImage diff =
                this->d->imageDiff(this->d->m_prevFrame,
                                   src,
                                   nColors,
                                   this->d->m_threshold,
                                   this->d->m_lumaThreshold,
                                   this->d->m_alphaVariation,
                                   this->d->m_mode);

        QPainter painter;
        painter.begin(&this->d->m_fireBuffer);
        painter.drawImage(0, 0, diff);
        painter.end();

        auto firePacket = AkUtils::imageToPacket(this->d->m_fireBuffer, packet);
        auto blurPacket = this->d->m_blurFilter->iStream(firePacket);
        this->d->m_fireBuffer = AkUtils::packetToImage(blurPacket);

        // Apply buffer.
        painter.begin(&oFrame);
        painter.drawImage(0, 0, src);
        painter.drawImage(0, 0, this->d->burn(this->d->m_fireBuffer,
                                              this->d->m_palette));
        painter.end();
    }

    this->d->m_prevFrame = src.copy();

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}

#include "moc_fireelement.cpp"
