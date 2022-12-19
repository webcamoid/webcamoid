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

#include <QDataStream>
#include <QDateTime>
#include <QMap>
#include <QQmlContext>
#include <QRandomGenerator>
#include <QSize>
#include <QVariant>
#include <QtMath>
#include <qrgb.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideomixer.h>
#include <akvideopacket.h>

#include "fireelement.h"

class FireElementPrivate
{
    public:
        FireElement::FireMode m_mode {FireElement::FireModeHard};
        int m_cool {-16};
        qreal m_dissolve {0.01};
        qreal m_zoom {0.02};
        int m_threshold {15};
        int m_lumaThreshold {15};
        int m_alphaDiff {-12};
        int m_alphaVariation {127};
        int m_nColors {8};
        QSize m_framSize;
        AkVideoPacket m_prevFrame;
        AkVideoPacket m_fireBuffer;
        QRgb m_palette[256];
        AkElementPtr m_blurFilter {akPluginManager->create<AkElement>("VideoFilter/Blur")};
        AkVideoConverter m_videoConverter;
        AkVideoMixer m_videoMixer;

        AkVideoPacket imageDiff(const AkVideoPacket &img1,
                                const AkVideoPacket &img2,
                                int colors,
                                int threshold,
                                int lumaThreshold,
                                int alphaVariation,
                                FireElement::FireMode mode);
        AkVideoPacket zoomImage(const AkVideoPacket &src, qreal factor);
        void coolImage(AkVideoPacket &src, int colorDiff);
        void imageAlphaDiff(AkVideoPacket &src, int alphaDiff);
        void dissolveImage(AkVideoPacket &src, qreal amount);
        AkVideoPacket burn(const AkVideoPacket &src,
                           const QRgb *palette);
        void createPalette();
};

FireElement::FireElement(): AkElement()
{
    this->d = new FireElementPrivate;
    this->d->createPalette();
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

FireElement::FireMode FireElement::mode() const
{
    return this->d->m_mode;
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

AkPacket FireElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.setOutputCaps({AkVideoCaps::Format_argbpack, 0, 0, {}});

    this->d->m_videoConverter.begin();
    this->d->m_videoConverter.setCacheIndex(0);
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    auto dst(src);

    QSize frameSize(src.caps().width(), src.caps().height());

    if (frameSize != this->d->m_framSize) {
        this->d->m_fireBuffer = AkVideoPacket();
        this->d->m_prevFrame = AkVideoPacket();
        this->d->m_framSize = frameSize;
    }

    if (!this->d->m_prevFrame) {
        this->d->m_fireBuffer = {src.caps(), true};
    } else {
        this->d->m_fireBuffer = this->d->zoomImage(this->d->m_fireBuffer,
                                                   this->d->m_zoom);
        this->d->coolImage(this->d->m_fireBuffer, this->d->m_cool);
        this->d->imageAlphaDiff(this->d->m_fireBuffer, this->d->m_alphaDiff);
        this->d->dissolveImage(this->d->m_fireBuffer, this->d->m_dissolve);

        int nColors = this->d->m_nColors > 0? this->d->m_nColors: 1;

        // Compute the difference between previous and current frame,
        // and save it to the buffer.
        auto diff = this->d->imageDiff(this->d->m_prevFrame,
                                       src,
                                       nColors,
                                       this->d->m_threshold,
                                       this->d->m_lumaThreshold,
                                       this->d->m_alphaVariation,
                                       this->d->m_mode);

        this->d->m_videoMixer.begin(&this->d->m_fireBuffer);
        this->d->m_videoMixer.setCacheIndex(0);
        this->d->m_videoMixer.draw(diff);
        this->d->m_videoMixer.end();

        this->d->m_fireBuffer = this->d->m_blurFilter->iStream(this->d->m_fireBuffer);

        // Apply buffer.
        this->d->m_videoMixer.begin(&dst);
        this->d->m_videoMixer.setCacheIndex(1);
        this->d->m_videoMixer.draw(this->d->burn(this->d->m_fireBuffer,
                                                 this->d->m_palette));
        this->d->m_videoMixer.end();
    }

    this->d->m_prevFrame = src;

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void FireElement::setMode(const FireMode &mode)
{
    if (this->d->m_mode == mode)
        return;

    this->d->m_mode = mode;
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
    this->setMode(FireElement::FireModeHard);
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

QDataStream &operator >>(QDataStream &istream, FireElement::FireMode &mode)
{
    int modeInt;
    istream >> modeInt;
    mode = static_cast<FireElement::FireMode>(modeInt);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, FireElement::FireMode mode)
{
    ostream << static_cast<int>(mode);

    return ostream;
}

AkVideoPacket FireElementPrivate::imageDiff(const AkVideoPacket &img1,
                                            const AkVideoPacket &img2,
                                            int colors,
                                            int threshold,
                                            int lumaThreshold,
                                            int alphaVariation,
                                            FireElement::FireMode mode)
{
    int width = qMin(img1.caps().width(), img2.caps().width());
    int height = qMin(img1.caps().height(), img2.caps().height());
    auto ocaps = img2.caps();
    ocaps.setWidth(width);
    ocaps.setHeight(height);
    AkVideoPacket diff(ocaps);
    diff.copyMetadata(img2);

    for (int y = 0; y < height; y++) {
        auto iLine1 = reinterpret_cast<const QRgb *>(img1.constLine(0, y));
        auto iLine2 = reinterpret_cast<const QRgb *>(img2.constLine(0, y));
        auto oLine = reinterpret_cast<QRgb *>(diff.line(0, y));

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
            alpha = int(sqrt(alpha / 3.0));

            if (mode == FireElement::FireModeSoft)
                alpha = alpha < threshold? 0: alpha;
            else
                alpha = alpha < threshold?
                          0: QRandomGenerator::global()->bounded(255 - alphaVariation, 256);

            int gray = qGray(iLine2[x]);
            alpha = gray < lumaThreshold? 0: alpha;
            int b = QRandomGenerator::global()->bounded(255 - colors, 256);
            oLine[x] = qRgba(0, 0, b, alpha);
        }
    }

    return diff;
}

AkVideoPacket FireElementPrivate::zoomImage(const AkVideoPacket &src, qreal factor)
{
    auto ocaps = src.caps();
    ocaps.setHeight(qRound((1.0 + factor) * src.caps().height()));
    this->m_videoConverter.setOutputCaps(ocaps);

    this->m_videoConverter.begin();
    this->m_videoConverter.setCacheIndex(1);
    auto scaled = this->m_videoConverter.convert(src);
    this->m_videoConverter.end();

    AkVideoPacket zoom(src.caps(), true);
    zoom.copyMetadata(src);

    this->m_videoMixer.begin(&zoom);
    this->m_videoMixer.setCacheIndex(2);
    this->m_videoMixer.draw(0,
                            src.caps().height() - scaled.caps().height(),
                            scaled);
    this->m_videoMixer.end();

    return zoom;
}

void FireElementPrivate::coolImage(AkVideoPacket &src, int colorDiff)
{
    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<QRgb *>(src.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto &pixel = srcLine[x];
            int b = qBound(0, qBlue(pixel) + colorDiff, 255);
            pixel = qRgba(0, 0, b, qAlpha(pixel));
        }
    }
}

void FireElementPrivate::imageAlphaDiff(AkVideoPacket &src, int alphaDiff)
{
    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<QRgb *>(src.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto &pixel = srcLine[x];
            int b = qBlue(pixel);
            int a = qBound(0, qAlpha(pixel) + alphaDiff, 255);
            pixel = qRgba(0, 0, b, a);
        }
    }
}

void FireElementPrivate::dissolveImage(AkVideoPacket &src, qreal amount)
{
    auto videoArea = qint64(src.caps().width()) * qint64(src.caps().height());
    auto n = qRound64(amount * videoArea);

    for (qint64 i = 0; i < n; i++) {
        int x = QRandomGenerator::global()->bounded(src.caps().width());
        int y = QRandomGenerator::global()->bounded(src.caps().height());
        auto pixel = src.pixel<QRgb>(0, x, y);
        int b = qBlue(pixel);
        int a = QRandomGenerator::global()->bounded(qAlpha(pixel) + 1);
        src.setPixel(0, x, y, qRgba(0, 0, b, a));
    }
}

AkVideoPacket FireElementPrivate::burn(const AkVideoPacket &src,
                                       const QRgb *palette)
{
    AkVideoPacket dest(src.caps());

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<QRgb *>(dest.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto &pixel = srcLine[x];
            int index = qBlue(srcLine[x]);
            auto &palPixel = palette[index];
            int r = qRed(palPixel);
            int g = qGreen(palPixel);
            int b = qBlue(palPixel);

            dstLine[x] = qRgba(r, g, b, qAlpha(pixel));
        }
    }

    return dest;
}

void FireElementPrivate::createPalette()
{
    for (int i = 0; i < 128; i++)
        this->m_palette[i] = qRgb(255,
                                  (3 * i +  128) >> 1,
                                  i >> 1);

    for (int i = 128; i < 256; i++)
        this->m_palette[i] = qRgb(255,
                                  255,
                                  (3 * i - 256) >> 1);
}

#include "moc_fireelement.cpp"
