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

#include <limits>
#include <QDateTime>
#include <QMutex>
#include <QQmlContext>
#include <QSize>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideomixer.h>
#include <akvideopacket.h>

#include "cartoonelement.h"

#define PALETTE_SIZE (1UL << 16)

class CartoonElementPrivate
{
    public:
        int m_ncolors {8};
        int m_colorDiff {10};
        bool m_showEdges {true};
        int m_thresholdLow {85};
        int m_thresholdHi {171};
        QRgb m_lineColor {qRgb(0, 0, 0)};
        QSize m_scanSize {320, 240};
        QRgb *m_palette {nullptr};
        qint64 m_id {-1};
        qint64 m_lastTime {0};
        QMutex m_mutex;
        AkVideoConverter m_videoConverter;
        AkVideoMixer m_videoMixer;

        void updatePalette(const AkVideoPacket &img,
                           int ncolors,
                           int colorDiff);
        quint16 nearestColor(const quint16 *palette,
                             size_t paletteSize,
                             quint16 color) const;
        AkVideoPacket edges(const AkVideoPacket &src,
                            int thLow,
                            int thHi,
                            QRgb color) const;
        inline quint16 rgb24Torgb16(QRgb color);
        inline void rgb16Torgb24(int *r, int *g, int *b, quint16 color);
        inline QRgb rgb16Torgb24(quint16 color);
};

CartoonElement::CartoonElement(): AkElement()
{
    this->d = new CartoonElementPrivate;
    this->d->m_videoConverter.setAspectRatioMode(AkVideoConverter::AspectRatioMode_Keep);
    this->d->m_palette = new QRgb [PALETTE_SIZE];
}

CartoonElement::~CartoonElement()
{
    delete [] this->d->m_palette;
    delete this->d;
}

int CartoonElement::ncolors() const
{
    return this->d->m_ncolors;
}

int CartoonElement::colorDiff() const
{
    return this->d->m_colorDiff;
}

bool CartoonElement::showEdges() const
{
    return this->d->m_showEdges;
}

int CartoonElement::thresholdLow() const
{
    return this->d->m_thresholdLow;
}

int CartoonElement::thresholdHi() const
{
    return this->d->m_thresholdHi;
}

QRgb CartoonElement::lineColor() const
{
    return this->d->m_lineColor;
}

QSize CartoonElement::scanSize() const
{
    return this->d->m_scanSize;
}

QString CartoonElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Cartoon/share/qml/main.qml");
}

void CartoonElement::controlInterfaceConfigure(QQmlContext *context,
                                               const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Cartoon", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket CartoonElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_mutex.lock();
    QSize scanSize(this->d->m_scanSize);
    this->d->m_mutex.unlock();

    if (scanSize.isEmpty()) {
        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    this->d->m_videoConverter.begin();
    this->d->m_videoConverter.setOutputCaps({AkVideoCaps::Format_argbpack,
                                             0,
                                             0,
                                             {}});
    auto src = this->d->m_videoConverter.convert(packet);

    if (!src) {
        this->d->m_videoConverter.end();
        return {};
    }

    this->d->m_videoConverter.setOutputCaps({AkVideoCaps::Format_none,
                                             scanSize.width(),
                                             scanSize.height(),
                                             {}});
    auto srcScaled = this->d->m_videoConverter.convert(src);
    this->d->m_videoConverter.end();

    bool updatePalette = false;

    if (this->d->m_id != packet.id()) {
        this->d->m_id = packet.id();
        this->d->m_lastTime = QDateTime::currentMSecsSinceEpoch();
        updatePalette = true;
    }

    qint64 time = QDateTime::currentMSecsSinceEpoch();

    // This code stabilize the color change between frames.
    if (updatePalette || (time - this->d->m_lastTime) >= 3 * 1000) {
        this->d->updatePalette(srcScaled,
                               this->d->m_ncolors,
                               this->d->m_colorDiff);

        this->d->m_lastTime = time;
    }

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    for (int y = 0; y < src.caps().height(); ++y) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto pixel = this->d->m_palette[this->d->rgb24Torgb16(srcLine[x])];
            dstLine[x] = qRgba(qRed(pixel),
                               qGreen(pixel),
                               qBlue(pixel),
                               qAlpha(srcLine[x]));
        }
    }

    // Draw the edges.
    if (this->d->m_showEdges) {
        this->d->m_videoMixer.begin(&dst);
        auto edges = this->d->edges(src,
                                    this->d->m_thresholdLow,
                                    this->d->m_thresholdHi,
                                    this->d->m_lineColor);
        this->d->m_videoMixer.draw(edges);
        this->d->m_videoMixer.end();
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void CartoonElement::setNColors(int ncolors)
{
    if (this->d->m_ncolors == ncolors)
        return;

    this->d->m_ncolors = ncolors;
    emit this->ncolorsChanged(ncolors);
}

void CartoonElement::setColorDiff(int colorDiff)
{
    if (this->d->m_colorDiff == colorDiff)
        return;

    this->d->m_colorDiff = colorDiff;
    emit this->colorDiffChanged(colorDiff);
}

void CartoonElement::setShowEdges(bool showEdges)
{
    if (this->d->m_showEdges == showEdges)
        return;

    this->d->m_showEdges = showEdges;
    emit this->showEdgesChanged(showEdges);
}

void CartoonElement::setThresholdLow(int thresholdLow)
{
    if (this->d->m_thresholdLow == thresholdLow)
        return;

    this->d->m_thresholdLow = thresholdLow;
    emit this->thresholdLowChanged(thresholdLow);
}

void CartoonElement::setThresholdHi(int thresholdHi)
{
    if (this->d->m_thresholdHi == thresholdHi)
        return;

    this->d->m_thresholdHi = thresholdHi;
    emit this->thresholdHiChanged(thresholdHi);
}

void CartoonElement::setLineColor(QRgb lineColor)
{
    if (this->d->m_lineColor == lineColor)
        return;

    this->d->m_lineColor = lineColor;
    emit this->lineColorChanged(lineColor);
}

void CartoonElement::setScanSize(const QSize &scanSize)
{
    if (this->d->m_scanSize == scanSize)
        return;

    this->d->m_mutex.lock();
    this->d->m_scanSize = scanSize;
    this->d->m_mutex.unlock();
    emit this->scanSizeChanged(scanSize);
}

void CartoonElement::resetNColors()
{
    this->setNColors(8);
}

void CartoonElement::resetColorDiff()
{
    this->setColorDiff(95);
}

void CartoonElement::resetShowEdges()
{
    this->setShowEdges(true);
}

void CartoonElement::resetThresholdLow()
{
    this->setThresholdLow(85);
}

void CartoonElement::resetThresholdHi()
{
    this->setThresholdHi(171);
}

void CartoonElement::resetLineColor()
{
    this->setLineColor(qRgb(0, 0, 0));
}

void CartoonElement::resetScanSize()
{
    this->setScanSize(QSize(320, 240));
}

void CartoonElementPrivate::updatePalette(const AkVideoPacket &packet,
                                          int ncolors,
                                          int colorDiff)
{
    using WeightType = quint64;

    // Create a histogram of 66k colors.
    WeightType histogram[PALETTE_SIZE];
    memset(histogram, 0, sizeof(WeightType) * PALETTE_SIZE);

    for (int y = 0; y < packet.caps().height(); y++) {
        auto line = reinterpret_cast<const QRgb *>(packet.constLine(0, y));

        for (int x = 0; x < packet.caps().width(); x++)
            // Pixels must be converted from 24 bits to 16 bits color depth.
            histogram[this->rgb24Torgb16(line[x])]++;
    }

    int colorDiff2 = colorDiff * colorDiff;
    quint16 palette[ncolors];
    WeightType ceilWeight = std::numeric_limits<WeightType>::max();
    int j = 0;

    // Create a palette with n-colors.
    for (; j < ncolors;) {
        quint16 color = 0;
        WeightType maxWeight = 0;

        for (int i = 0; i < PALETTE_SIZE; ++i) {
            int weight = histogram[i];

            if (weight > maxWeight && weight < ceilWeight) {
                maxWeight = weight;
                color = i;
            }
        }

        ceilWeight = maxWeight;
        bool add = true;

        int r = (color >> 11) & 0x1f;
        int g = (color >> 5) & 0x3f;
        int b = color & 0x1f;

        for (int i = 0; i < j; ++i) {
            auto color_ = palette[i];

            int cr = (color_ >> 11) & 0x1f;
            int cg = (color_ >> 5) & 0x3f;
            int cb = color_ & 0x1f;

            int dr = cr - r;
            int dg = cg - g;
            int db = cb - b;

            int k = dr * dr + dg * dg + db * db;

            if (k < colorDiff2) {
                add = false;

                break;
            }
        }

        if (add) {
            palette[j] = color;
            j++;
        }

        if (maxWeight < 1)
            break;
    }

    // Create a look-up table for speed-up the conversion from 16-24 bits
    // to palettized format.
    for (int i = 0; i < PALETTE_SIZE; i++) {
        auto color = this->nearestColor(palette, j, i);
        this->m_palette[i] = this->rgb16Torgb24(color);
    }
}

quint16 CartoonElementPrivate::nearestColor(const quint16 *palette,
                                            size_t paletteSize,
                                            quint16 color) const
{
    if (paletteSize < 1)
        return color;

    int k = std::numeric_limits<int>::max();
    int index = 0;

    int r = (color >> 11) & 0x1f;
    int g = (color >> 5) & 0x3f;
    int b = color & 0x1f;

    for (int i = 0; i < paletteSize; i++) {
        auto color_ = palette[i];

        int cr = (color_ >> 11) & 0x1f;
        int cg = (color_ >> 5) & 0x3f;
        int cb = color_ & 0x1f;

        int dr = cr - r;
        int dg = cg - g;
        int db = cb - b;
        int q = dr * dr + dg * dg + db * db;

        if (q < k) {
            k = q;
            index = i;
        }
    }

    return palette[index];
}

AkVideoPacket CartoonElementPrivate::edges(const AkVideoPacket &src,
                                           int thLow,
                                           int thHi,
                                           QRgb color) const
{
    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    if (thLow > thHi)
        std::swap(thLow, thHi);

    static const int ncolors = 256;
    QRgb colors[ncolors];

    for (int i = 0; i < ncolors; i++) {
        int alpha = i < thLow? 0: i > thHi? 255: i;
        colors[i] = qRgba(qRed(color), qGreen(color), qBlue(color), alpha);
    }

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        auto srcLine_m1 = y < 1? srcLine: srcLine - src.caps().width();
        auto srcLine_p1 = y >= src.caps().height() - 1? srcLine: srcLine + src.caps().width();

        for (int x = 0; x < src.caps().width(); x++) {
            int x_m1 = x < 1? x: x - 1;
            int x_p1 = x >= src.caps().width() - 1? x: x + 1;

            int s_m1_p1 = qGray(srcLine_m1[x_p1]);
            int s_p1_p1 = qGray(srcLine_p1[x_p1]);
            int s_m1_m1 = qGray(srcLine_m1[x_m1]);
            int s_p1_m1 = qGray(srcLine_p1[x_m1]);

            int gradX = s_m1_p1
                      + 2 * qGray(srcLine[x_p1])
                      + s_p1_p1
                      - s_m1_m1
                      - 2 * qGray(srcLine[x_m1])
                      - s_p1_m1;

            int gradY = s_m1_m1
                      + 2 * qGray(srcLine_m1[x])
                      + s_m1_p1
                      - s_p1_m1
                      - 2 * qGray(srcLine_p1[x])
                      - s_p1_p1;

            int grad = qAbs(gradX) + qAbs(gradY);
            grad = qBound(0, grad, 255);
            dstLine[x] = colors[grad];
        }
    }

    return dst;
}

quint16 CartoonElementPrivate::rgb24Torgb16(QRgb color)
{
    return ((qRed(color) >> 3) << 11)
            | ((qGreen(color) >> 2) << 5)
            | (qBlue(color) >> 3);
}

void CartoonElementPrivate::rgb16Torgb24(int *r, int *g, int *b, quint16 color)
{
    *r = (color >> 11) & 0x1f;
    *g = (color >> 5) & 0x3f;
    *b = color & 0x1f;
    *r = 0xff * *r / 0x1f;
    *g = 0xff * *g / 0x3f;
    *b = 0xff * *b / 0x1f;
}

QRgb CartoonElementPrivate::rgb16Torgb24(quint16 color)
{
    int r;
    int g;
    int b;
    rgb16Torgb24(&r, &g, &b, color);

    return qRgb(r, g, b);
}

#include "moc_cartoonelement.cpp"
