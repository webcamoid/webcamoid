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
#include <limits>

#include "cartoonelement.h"

CartoonElement::CartoonElement(): AkElement()
{
    this->m_threshold = 95;
    this->m_levels = 8;
    this->m_scanSize = QSize(160, 120);
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

int CartoonElement::levels() const
{
    return this->m_levels;
}

QSize CartoonElement::scanSize() const
{
    return this->m_scanSize;
}

QVector<QRgb> CartoonElement::palette(const QImage &img, int colors) const
{
    if (colors < 1)
        return QVector<QRgb>();

    QVector<QRgb> palette(colors);
    int imgArea = img.width() * img.height();
    const QRgb *bits = (const QRgb *) img.constBits();

    for (int i = 0; i < colors; i++)
        palette[i] = bits[i];

    for (int j = colors; j < imgArea; j++) {
        int k = std::numeric_limits<int>::max();
        int index = 0;
        QRgb color = bits[j];
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

        palette[index] = qRgba((r + qRed(palette[index])) / 2,
                               (g + qGreen(palette[index])) / 2,
                               (b + qBlue(palette[index])) / 2,
                               (a + qAlpha(palette[index])) / 2);
    }

    return palette;
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

void CartoonElement::setLevels(int levels)
{
    if (this->m_levels == levels)
        return;

    this->m_levels = levels;
    emit this->levelsChange(levels);
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

void CartoonElement::resetLevels()
{
    this->setLevels(8);
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

    QVector<QRgb> palette =
            this->palette(src.scaled(scanSize, Qt::KeepAspectRatio),
                          this->m_levels);

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
