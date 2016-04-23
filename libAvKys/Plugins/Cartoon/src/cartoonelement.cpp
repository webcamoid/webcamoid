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

#include <QDateTime>
#include <QtMath>
#include <limits>

#include "cartoonelement.h"

typedef QVector<PixelInt> ColorPalette;

inline ColorPalette initDefaultPalette()
{
    ColorPalette defaultPalette;
    defaultPalette << qRgb(255,   0,   0);
    defaultPalette << qRgb(  0, 255,   0);
    defaultPalette << qRgb(  0,   0, 255);
    defaultPalette << qRgb(255, 255,   0);
    defaultPalette << qRgb(255,   0, 255);
    defaultPalette << qRgb(  0, 255, 255);
    defaultPalette << qRgb(  0,   0,   0);
    defaultPalette << qRgb(255, 255, 255);
    defaultPalette << qRgb(127, 127, 127);

    return defaultPalette;
}

Q_GLOBAL_STATIC_WITH_ARGS(ColorPalette, defaultPalette, (initDefaultPalette()))

CartoonElement::CartoonElement(): AkElement()
{
    this->m_threshold = 95;
    this->m_scanSize = QSize(160, 120);
    this->m_id = -1;
    this->m_lastTime = 0;
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
    context->setContextProperty("Cartoon", (QObject *) this);
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

int CartoonElement::threshold() const
{
    return this->m_threshold;
}

QSize CartoonElement::scanSize() const
{
    return this->m_scanSize;
}

QVector<QRgb> CartoonElement::palette(const QImage &img)
{
    int imgArea = img.width() * img.height();
    const QRgb *bits = (const QRgb *) img.constBits();

    for (int j = 0; j < imgArea; j++) {
        int k = std::numeric_limits<int>::max();
        int index = 0;
        QRgb color = bits[j];
        int r = qRed(color);
        int g = qGreen(color);
        int b = qBlue(color);
        int a = qAlpha(color);

        // Find the most similar color in the palette.
        for (int i = 0; i < this->m_palette.size(); i++) {
            int n = this->m_palette[i].n;

            if (!n)
                n = 1;

            int rdiff = r - this->m_palette[i].r / n;
            int gdiff = g - this->m_palette[i].g / n;
            int bdiff = b - this->m_palette[i].b / n;
            int adiff = a - this->m_palette[i].a / n;
            int q = rdiff * rdiff
                    + gdiff * gdiff
                    + bdiff * bdiff
                    + adiff * adiff;

            if (q < k) {
                k = q;
                index = i;
            }
        }

        // Calculate the media of all similar colors.
        this->m_palette[index] += color;
    }

    QVector<QRgb> pal;

    for (int i = 0; i < this->m_palette.size(); i++)
        // Only append colors that exists in the image.
        if (this->m_palette[i].n > 1)
            pal << this->m_palette[i];

    qint64 time = QDateTime::currentMSecsSinceEpoch();

    // This code stabilize the color change between frames.
    if ((time - this->m_lastTime) >= 5 * 1000) {
        // Reset to default palette every 5 secs.
        this->m_palette = *defaultPalette;
        this->m_lastTime = time;
    }

    return pal;
}

QRgb CartoonElement::nearestColor(const QVector<QRgb> &palette, QRgb color) const
{
    if (palette.isEmpty())
        return color;

    int k = std::numeric_limits<int>::max();
    int index = 0;
    int r = qRed(color);
    int g = qGreen(color);
    int b = qBlue(color);
    int a = qAlpha(color);

    for (int i = 0; i < palette.count(); i++) {
        int rdiff = r - qRed(palette[i]);
        int gdiff = g - qGreen(palette[i]);
        int bdiff = b - qBlue(palette[i]);
        int adiff = a - qAlpha(palette[i]);
        int q = rdiff * rdiff
                + gdiff * gdiff
                + bdiff * bdiff
                + adiff * adiff;

        if (q < k) {
            k = q;
            index = i;
        }
    }

    return palette[index];
}

void CartoonElement::setThreshold(int threshold)
{
    if (this->m_threshold == threshold)
        return;

    this->m_threshold = threshold;
    emit this->thresholdChange(threshold);
}

void CartoonElement::setScanSize(QSize scanSize)
{
    if (this->m_scanSize == scanSize)
        return;

    this->m_scanSize = scanSize;
    emit this->scanSizeChanged(scanSize);
}

void CartoonElement::resetThreshold()
{
    this->setThreshold(95);
}

void CartoonElement::resetScanSize()
{
    this->setScanSize(QSize(160, 120));
}

AkPacket CartoonElement::iStream(const AkPacket &packet)
{
    QSize scanSize(this->m_scanSize);

    if (scanSize.isEmpty())
        akSend(packet)

    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());

    int videoArea = src.width() * src.height();
    const QRgb *srcPtr = (const QRgb *) src.constBits();
    QVector<quint8> gray(videoArea);
    quint8 *grayPtr = gray.data();

    for (int i = 0; i < videoArea; i++)
        grayPtr[i] = qGray(srcPtr[i]);

    if (this->m_id != packet.id()) {
        this->m_palette = *defaultPalette;
        this->m_id = packet.id();
        this->m_lastTime = QDateTime::currentMSecsSinceEpoch();
    }

    QVector<QRgb> palette =
            this->palette(src.scaled(scanSize, Qt::KeepAspectRatio));

    qreal k = log(1531) / 255.;
    int threshold = exp(k * (255 - this->m_threshold)) - 1;

    for (int y = 0; y < src.height(); y++) {
        const QRgb *srcLine = (const QRgb *) src.constScanLine(y);;
        QRgb *dstLine = (QRgb *) oFrame.constScanLine(y);;

        size_t yOffset = y * src.width();
        const quint8 *grayLine = gray.constData() + yOffset;

        const quint8 *grayLine_m1 = y < 1? grayLine: grayLine - src.width();
        const quint8 *grayLine_p1 = y >= src.height() - 1? grayLine: grayLine + src.width();

        for (int x = 0; x < src.width(); x++) {
            int x_m1 = x < 1? x: x - 1;
            int x_p1 = x >= src.width() - 1? x: x + 1;

            int gradX = grayLine_m1[x_p1]
                      + 2 * grayLine[x_p1]
                      + grayLine_p1[x_p1]
                      - grayLine_m1[x_m1]
                      - 2 * grayLine[x_m1]
                      - grayLine_p1[x_m1];

            int gradY = grayLine_m1[x_m1]
                      + 2 * grayLine_m1[x]
                      + grayLine_m1[x_p1]
                      - grayLine_p1[x_m1]
                      - 2 * grayLine_p1[x]
                      - grayLine_p1[x_p1];

            int grad = qAbs(gradX) + qAbs(gradY);

            if (grad >= threshold)
                dstLine[x] = qRgb(0, 0, 0);
            else
                dstLine[x] = this->nearestColor(palette, srcLine[x]);
        }
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
