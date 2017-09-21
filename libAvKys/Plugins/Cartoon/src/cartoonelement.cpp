/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include "cartoonelement.h"

CartoonElement::CartoonElement(): AkElement()
{
    this->m_ncolors = 8;
    this->m_colorDiff = 95;
    this->m_showEdges = true;
    this->m_thresholdLow = 85;
    this->m_thresholdHi = 171;
    this->m_lineColor = qRgb(0, 0, 0);
    this->m_scanSize = QSize(320, 240);
    this->m_id = -1;
    this->m_lastTime = 0;
}

CartoonElement::~CartoonElement()
{

}

QObject *CartoonElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Cartoon/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Cartoon", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
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

int CartoonElement::ncolors() const
{
    return this->m_ncolors;
}

int CartoonElement::colorDiff() const
{
    return this->m_colorDiff;
}

bool CartoonElement::showEdges() const
{
    return this->m_showEdges;
}

int CartoonElement::thresholdLow() const
{
    return this->m_thresholdLow;
}

int CartoonElement::thresholdHi() const
{
    return this->m_thresholdHi;
}

QRgb CartoonElement::lineColor() const
{
    return this->m_lineColor;
}

QSize CartoonElement::scanSize() const
{
    return this->m_scanSize;
}

QVector<QRgb> CartoonElement::palette(const QImage &img,
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
            const QRgb *line = reinterpret_cast<const QRgb *>(img.constScanLine(y));

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

            for (const QRgb &color: palette) {
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
            this->m_palette[i] = this->nearestColor(NULL,
                                                    NULL,
                                                    palette,
                                                    this->rgb16Torgb24(i));

        this->m_lastTime = time;
    }

    QVector<QRgb> palette = this->m_palette;

    return palette;
}

QRgb CartoonElement::nearestColor(int *index,
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

QImage CartoonElement::edges(const QImage &src, int thLow, int thHi, QRgb color) const
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
        const QRgb *srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        QRgb *dstLine = reinterpret_cast<QRgb *>(dst.scanLine(y));

        const QRgb *srcLine_m1 = y < 1? srcLine: srcLine - src.width();
        const QRgb *srcLine_p1 = y >= src.height() - 1? srcLine: srcLine + src.width();

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

void CartoonElement::setNColors(int ncolors)
{
    if (this->m_ncolors == ncolors)
        return;

    this->m_ncolors = ncolors;
    emit this->ncolorsChanged(ncolors);
}

void CartoonElement::setColorDiff(int colorDiff)
{
    if (this->m_colorDiff == colorDiff)
        return;

    this->m_colorDiff = colorDiff;
    emit this->colorDiffChanged(colorDiff);
}

void CartoonElement::setShowEdges(bool showEdges)
{
    if (this->m_showEdges == showEdges)
        return;

    this->m_showEdges = showEdges;
    emit this->showEdgesChanged(showEdges);
}

void CartoonElement::setThresholdLow(int thresholdLow)
{
    if (this->m_thresholdLow == thresholdLow)
        return;

    this->m_thresholdLow = thresholdLow;
    emit this->thresholdLowChanged(thresholdLow);
}

void CartoonElement::setThresholdHi(int thresholdHi)
{
    if (this->m_thresholdHi == thresholdHi)
        return;

    this->m_thresholdHi = thresholdHi;
    emit this->thresholdHiChanged(thresholdHi);
}

void CartoonElement::setLineColor(QRgb lineColor)
{
    if (this->m_lineColor == lineColor)
        return;

    this->m_lineColor = lineColor;
    emit this->lineColorChanged(lineColor);
}

void CartoonElement::setScanSize(const QSize &scanSize)
{
    if (this->m_scanSize == scanSize)
        return;

    this->m_mutex.lock();
    this->m_scanSize = scanSize;
    this->m_mutex.unlock();
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

AkPacket CartoonElement::iStream(const AkPacket &packet)
{
    this->m_mutex.lock();
    QSize scanSize(this->m_scanSize);
    this->m_mutex.unlock();

    if (scanSize.isEmpty())
        akSend(packet)

    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());

    if (this->m_id != packet.id()) {
        this->m_id = packet.id();
        this->m_palette.clear();
        this->m_lastTime = QDateTime::currentMSecsSinceEpoch();
    }

    // Palettize image.
    QVector<QRgb> palette =
            this->palette(src.scaled(scanSize, Qt::KeepAspectRatio), this->m_ncolors, this->m_colorDiff);

    for (int y = 0; y < src.height(); y++) {
        const QRgb *srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        QRgb *dstLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < src.width(); x++)
            dstLine[x] = palette[this->rgb24Torgb16(srcLine[x])];
    }

    // Draw the edges.
    if (this->m_showEdges) {
        QPainter painter;
        painter.begin(&oFrame);
        QImage edges = this->edges(src,
                                   this->m_thresholdLow,
                                   this->m_thresholdHi,
                                   this->m_lineColor);
        painter.drawImage(0, 0, edges);
        painter.end();
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
