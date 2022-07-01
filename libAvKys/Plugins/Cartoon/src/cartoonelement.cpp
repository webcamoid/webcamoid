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
#include <QtMath>
#include <QPainter>
#include <QDateTime>
#include <QMutex>
#include <QQmlContext>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "cartoonelement.h"

class CartoonElementPrivate
{
    public:
        int m_ncolors {8};
        int m_colorDiff {95};
        bool m_showEdges {true};
        int m_thresholdLow {85};
        int m_thresholdHi {171};
        QRgb m_lineColor {qRgb(0, 0, 0)};
        QSize m_scanSize {320, 240};
        QVector<QRgb> m_palette;
        qint64 m_id {-1};
        qint64 m_lastTime {0};
        QMutex m_mutex;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argb, 0, 0, {}}};

        QVector<QRgb> palette(const QImage &img,
                              int ncolors,
                              int colorDiff);
        QRgb nearestColor(int *index,
                          int *diff,
                          const QVector<QRgb> &palette,
                          QRgb color) const;
        QImage edges(const QImage &src,
                     int thLow,
                     int thHi,
                     QRgb color) const;
        int rgb24Torgb16(QRgb color);
        void rgb16Torgb24(int *r, int *g, int *b, int color);
        QRgb rgb16Torgb24(int color);
};

CartoonElement::CartoonElement(): AkElement()
{
    this->d = new CartoonElementPrivate;
}

CartoonElement::~CartoonElement()
{
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

QVector<QRgb> CartoonElementPrivate::palette(const QImage &img,
                                             int ncolors,
                                             int colorDiff)
{
    qint64 time = QDateTime::currentMSecsSinceEpoch();

    // This code stabilize the color change between frames.
    if (this->m_palette.isEmpty() || (time - this->m_lastTime) >= 3 * 1000) {
        // Create a histogram of 66k colors.
        QVector<QPair<int, int>> histogram(1 << 16);

        for (int i = 0; i < histogram.size(); i++)
            histogram[i].second = i;

        for (int y = 0; y < img.height(); y++) {
            auto line = reinterpret_cast<const QRgb *>(img.constScanLine(y));

            for (int x = 0; x < img.width(); x++)
                // Pixels must be converted from 24 bits to 16 bits color depth.
                histogram[this->rgb24Torgb16(line[x])].first++;
        }

        // Sort the histogram by weights.
        std::sort(histogram.begin(), histogram.end());
        QVector<QRgb> palette;

        if (ncolors < 1)
            ncolors = 1;

        // Create a palette with n-colors, starting from tail.
        for (int i = histogram.size() - 1; i >= 0 && palette.size() < ncolors; i--) {
            int r;
            int g;
            int b;
            this->rgb16Torgb24(&r, &g, &b, histogram[i].second);
            bool add = true;

            for (auto &color: palette) {
                int dr = r - qRed(color);
                int dg = g - qGreen(color);
                int db = b - qBlue(color);
                int k = qRound(qSqrt(dr * dr + dg * dg + db * db));

                // The color to add must be different enough for not repeating
                // similar colors in the palette.
                if (k < colorDiff) {
                    add = false;

                    break;
                }
            }

            if (add)
                palette << qRgb(r, g, b);
        }

        // Create a look-up table for speed-up the conversion from 16-24 bits
        // to palettized format.
        this->m_palette.resize(1 << 16);

        for (int i = 0; i < this->m_palette.size(); i++)
            this->m_palette[i] = this->nearestColor(nullptr,
                                                    nullptr,
                                                    palette,
                                                    this->rgb16Torgb24(i));

        this->m_lastTime = time;
    }

    auto palette = this->m_palette;

    return palette;
}

QRgb CartoonElementPrivate::nearestColor(int *index,
                                         int *diff,
                                         const QVector<QRgb> &palette,
                                         QRgb color) const
{
    if (palette.isEmpty()) {
        if (index)
            *index = -1;

        if (diff)
            *diff = std::numeric_limits<int>::max();

        return color;
    }

    int k = std::numeric_limits<int>::max();
    int index_ = 0;
    int r = qRed(color);
    int g = qGreen(color);
    int b = qBlue(color);

    for (int i = 0; i < palette.count(); i++) {
        int rdiff = r - qRed(palette[i]);
        int gdiff = g - qGreen(palette[i]);
        int bdiff = b - qBlue(palette[i]);
        int q = rdiff * rdiff
                + gdiff * gdiff
                + bdiff * bdiff;

        if (q < k) {
            k = q;
            index_ = i;
        }
    }

    if (index)
        *index = index_;

    if (diff)
        *diff = qRound(qSqrt(k));

    return palette[index_];
}

QImage CartoonElementPrivate::edges(const QImage &src,
                                    int thLow,
                                    int thHi,
                                    QRgb color) const
{
    QImage dst(src.size(), src.format());

    if (thLow > thHi)
        std::swap(thLow, thHi);

    QVector<QRgb> colors(256);

    for (int i = 0; i < colors.size(); i++) {
        int alpha = i < thLow? 0: i > thHi? 255: i;
        colors[i] = qRgba(qRed(color), qGreen(color), qBlue(color), alpha);
    }

    for (int y = 0; y < src.height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.scanLine(y));

        auto srcLine_m1 = y < 1? srcLine: srcLine - src.width();
        auto srcLine_p1 = y >= src.height() - 1? srcLine: srcLine + src.width();

        for (int x = 0; x < src.width(); x++) {
            int x_m1 = x < 1? x: x - 1;
            int x_p1 = x >= src.width() - 1? x: x + 1;

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

int CartoonElementPrivate::rgb24Torgb16(QRgb color)
{
    return ((qRed(color) >> 3) << 11)
            | ((qGreen(color) >> 2) << 5)
            | (qBlue(color) >> 3);
}

void CartoonElementPrivate::rgb16Torgb24(int *r, int *g, int *b, int color)
{
    *r = (color >> 11) & 0x1f;
    *g = (color >> 5) & 0x3f;
    *b = color & 0x1f;
    *r = 0xff * *r / 0x1f;
    *g = 0xff * *g / 0x3f;
    *b = 0xff * *b / 0x1f;
}

QRgb CartoonElementPrivate::rgb16Torgb24(int color)
{
    int r;
    int g;
    int b;
    rgb16Torgb24(&r, &g, &b, color);

    return qRgb(r, g, b);
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

    if (scanSize.isEmpty())
        akSend(packet)

    auto src = this->d->m_videoConverter.convertToImage(packet);

    if (src.isNull())
        return AkPacket();

    QImage oFrame(src.size(), src.format());

    if (this->d->m_id != packet.id()) {
        this->d->m_id = packet.id();
        this->d->m_palette.clear();
        this->d->m_lastTime = QDateTime::currentMSecsSinceEpoch();
    }

    // Palettize image.
    auto palette =
            this->d->palette(src.scaled(scanSize, Qt::KeepAspectRatio),
                             this->d->m_ncolors,
                             this->d->m_colorDiff);

    for (int y = 0; y < src.height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        auto dstLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < src.width(); x++)
            dstLine[x] = palette[this->d->rgb24Torgb16(srcLine[x])];
    }

    // Draw the edges.
    if (this->d->m_showEdges) {
        QPainter painter;
        painter.begin(&oFrame);
        auto edges = this->d->edges(src,
                                    this->d->m_thresholdLow,
                                    this->d->m_thresholdHi,
                                    this->d->m_lineColor);
        painter.drawImage(0, 0, edges);
        painter.end();
    }

    auto oPacket = this->d->m_videoConverter.convert(oFrame, packet);
    akSend(oPacket)
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

#include "moc_cartoonelement.cpp"
