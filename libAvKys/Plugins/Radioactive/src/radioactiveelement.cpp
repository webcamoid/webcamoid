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
#include <QQmlContext>
#include <QSize>
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideomixer.h>
#include <akvideopacket.h>

#include "radioactiveelement.h"

class RadioactiveElementPrivate
{
    public:
        QSize m_frameSize;
        AkVideoPacket m_prevFrame;
        AkVideoPacket m_blurZoomBuffer;
        AkElementPtr m_blurFilter {akPluginManager->create<AkElement>("VideoFilter/Blur")};
        AkElementPtr m_zoomFilter {akPluginManager->create<AkElement>("VideoFilter/Zoom")};
        RadioactiveElement::RadiationMode m_mode {RadioactiveElement::RadiationModeHardColor};
        int m_threshold {31};
        int m_lumaThreshold {95};
        int m_alphaDiff {8};
        QRgb m_radColor {qRgb(0, 255, 0)};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        AkVideoMixer m_videoMixer;
        AkVideoMixer m_bzVideoMixer;

        AkVideoPacket imageDiff(const AkVideoPacket &img1,
                                const AkVideoPacket &img2,
                                int threshold,
                                int lumaThreshold,
                                QRgb radColor,
                                RadioactiveElement::RadiationMode mode);
        AkVideoPacket imageAlphaDiff(const AkVideoPacket &src, int alphaDiff);
};

RadioactiveElement::RadioactiveElement(): AkElement()
{
    this->d = new RadioactiveElementPrivate;
    this->d->m_blurFilter->setProperty("radius", 2);
    this->d->m_zoomFilter->setProperty("zoom", 1.1);

    QObject::connect(this->d->m_blurFilter.data(),
                     SIGNAL(radiusChanged(int)),
                     this,
                     SIGNAL(blurChanged(int)));
    QObject::connect(this->d->m_zoomFilter.data(),
                     SIGNAL(zoomChanged(qreal)),
                     this,
                     SIGNAL(zoomChanged(qreal)));
}

RadioactiveElement::~RadioactiveElement()
{
    delete this->d;
}

RadioactiveElement::RadiationMode RadioactiveElement::mode() const
{
    return this->d->m_mode;
}

int RadioactiveElement::blur() const
{
    return this->d->m_blurFilter->property("radius").toInt();
}

qreal RadioactiveElement::zoom() const
{
    return this->d->m_zoomFilter->property("zoom").toReal();
}

int RadioactiveElement::threshold() const
{
    return this->d->m_threshold;
}

int RadioactiveElement::lumaThreshold() const
{
    return this->d->m_lumaThreshold;
}

int RadioactiveElement::alphaDiff() const
{
    return this->d->m_alphaDiff;
}

QRgb RadioactiveElement::radColor() const
{
    return this->d->m_radColor;
}

QString RadioactiveElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Radioactive/share/qml/main.qml");
}

void RadioactiveElement::controlInterfaceConfigure(QQmlContext *context,
                                                   const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Radioactive", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket RadioactiveElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    auto dst = src;
    QSize frameSize(src.caps().width(), src.caps().height());

    if (frameSize != this->d->m_frameSize) {
        this->d->m_blurZoomBuffer = AkVideoPacket();
        this->d->m_prevFrame = AkVideoPacket();
        this->d->m_frameSize = frameSize;
    }

    if (!this->d->m_prevFrame) {
        this->d->m_blurZoomBuffer = AkVideoPacket(src.caps(), true);
    } else {
        // Compute the difference between previous and current frame,
        // and save it to the buffer.
        auto diff = this->d->imageDiff(this->d->m_prevFrame,
                                       src,
                                       this->d->m_threshold,
                                       this->d->m_lumaThreshold,
                                       this->d->m_radColor,
                                       this->d->m_mode);

        this->d->m_bzVideoMixer.begin(&this->d->m_blurZoomBuffer);
        this->d->m_bzVideoMixer.draw(diff);
        this->d->m_bzVideoMixer.end();

        // Blur buffer.
        AkVideoPacket blur =
                this->d->m_blurFilter->iStream(this->d->m_blurZoomBuffer);

        // Zoom buffer.
        AkVideoPacket zoom = this->d->m_zoomFilter->iStream(blur);

        // Reduce alpha.
        auto alphaDiff = this->d->imageAlphaDiff(zoom, this->d->m_alphaDiff);
        this->d->m_blurZoomBuffer = alphaDiff;

        // Apply buffer.
        this->d->m_videoMixer.begin(&dst);
        this->d->m_videoMixer.draw(this->d->m_blurZoomBuffer);
        this->d->m_videoMixer.end();
    }

    this->d->m_prevFrame = src;

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void RadioactiveElement::setMode(RadiationMode mode)
{
    if (this->d->m_mode == mode)
        return;

    this->d->m_mode = mode;
    emit this->modeChanged(mode);
}

void RadioactiveElement::setBlur(int blur)
{
    this->d->m_blurFilter->setProperty("radius", blur);
}

void RadioactiveElement::setZoom(qreal zoom)
{
    this->d->m_zoomFilter->setProperty("zoom", zoom);
}

void RadioactiveElement::setThreshold(int threshold)
{
    if (this->d->m_threshold == threshold)
        return;

    this->d->m_threshold = threshold;
    emit this->thresholdChanged(threshold);
}

void RadioactiveElement::setLumaThreshold(int lumaThreshold)
{
    if (this->d->m_lumaThreshold == lumaThreshold)
        return;

    this->d->m_lumaThreshold = lumaThreshold;
    emit this->lumaThresholdChanged(lumaThreshold);
}

void RadioactiveElement::setAlphaDiff(int alphaDiff)
{
    if (this->d->m_alphaDiff == alphaDiff)
        return;

    this->d->m_alphaDiff = alphaDiff;
    emit this->alphaDiffChanged(alphaDiff);
}

void RadioactiveElement::setRadColor(QRgb radColor)
{
    if (this->d->m_radColor == radColor)
        return;

    this->d->m_radColor = radColor;
    emit this->radColorChanged(radColor);
}

void RadioactiveElement::resetMode()
{
    this->setMode(RadiationModeHardColor);
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
    this->setAlphaDiff(8);
}

void RadioactiveElement::resetRadColor()
{
    this->setRadColor(qRgb(0, 255, 0));
}

QDataStream &operator >>(QDataStream &istream, RadioactiveElement::RadiationMode &mode)
{
    int modeInt;
    istream >> modeInt;
    mode = static_cast<RadioactiveElement::RadiationMode>(modeInt);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, RadioactiveElement::RadiationMode mode)
{
    ostream << static_cast<int>(mode);

    return ostream;
}

AkVideoPacket RadioactiveElementPrivate::imageDiff(const AkVideoPacket &img1,
                                                   const AkVideoPacket &img2,
                                                   int threshold,
                                                   int lumaThreshold,
                                                   QRgb radColor,
                                                   RadioactiveElement::RadiationMode mode)
{
    int width = qMin(img1.caps().width(), img2.caps().width());
    int height = qMin(img1.caps().height(), img2.caps().height());
    auto ocaps = img1.caps();
    ocaps.setWidth(width);
    ocaps.setHeight(height);
    AkVideoPacket diff(ocaps);

    int radR = qRed(radColor);
    int radG = qGreen(radColor);
    int radB = qBlue(radColor);

    for (int y = 0; y < height; y++) {
        auto iLine1 = reinterpret_cast<const QRgb *>(img1.constLine(0, y));
        auto iLine2 = reinterpret_cast<const QRgb *>(img2.constLine(0, y));
        auto oLine = reinterpret_cast<QRgb *>(diff.line(0, y));

        for (int x = 0; x < width; x++) {
            auto &pixel1 = iLine1[x];
            int r1 = qRed(pixel1);
            int g1 = qGreen(pixel1);
            int b1 = qBlue(pixel1);

            auto &pixel2 = iLine2[x];
            int r2 = qRed(pixel2);
            int g2 = qGreen(pixel2);
            int b2 = qBlue(pixel2);

            int dr = r1 - r2;
            int dg = g1 - g2;
            int db = b1 - b2;

            int alpha = dr * dr + dg * dg + db * db;
            alpha = int(qSqrt(alpha / 3.0));

            if (mode == RadioactiveElement::RadiationModeSoftNormal
                || mode == RadioactiveElement::RadiationModeSoftColor)
                alpha = alpha < threshold? 0: alpha;
            else
                alpha = alpha < threshold? 0: 255;

            int gray = qGray(pixel2);

            alpha = gray < lumaThreshold? 0: alpha;

            int r;
            int g;
            int b;

            if (mode == RadioactiveElement::RadiationModeHardNormal
                || mode == RadioactiveElement::RadiationModeSoftNormal) {
                r = r2;
                g = g2;
                b = b2;
            } else {
                r = radR;
                g = radG;
                b = radB;
            }

            oLine[x] = qRgba(r, g, b, alpha);
        }
    }

    return diff;
}

AkVideoPacket RadioactiveElementPrivate::imageAlphaDiff(const AkVideoPacket &src,
                                                        int alphaDiff)
{
    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);
    alphaDiff = qBound(0, alphaDiff, 255);

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto &pixel = srcLine[x];
            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);
            int a = qBound(0, qAlpha(pixel) - alphaDiff, 255);
            dstLine[x] = qRgba(r, g, b, a);
        }
    }

    return dst;
}

#include "moc_radioactiveelement.cpp"
