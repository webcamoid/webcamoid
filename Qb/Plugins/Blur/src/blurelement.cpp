/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "blurelement.h"

BlurElement::BlurElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetRadius();
}

QObject *BlurElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Blur/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Blur", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

int BlurElement::radius() const
{
    return this->m_radius;
}

void BlurElement::integralImage(const QImage &image,
                                int oWidth, int oHeight,
                                PixelUint32 *integral)
{
    for (int y = 1; y < oHeight; y++) {
        const QRgb *line = (const QRgb *) image.constScanLine(y - 1);

        // Reset current line summation.
        quint32 sumR = 0;
        quint32 sumG = 0;
        quint32 sumB = 0;

        for (int x = 1; x < oWidth; x++) {
            QRgb pixel = line[x - 1];

            // Accumulate pixels in current line.
            sumR += qRed(pixel);
            sumG += qGreen(pixel);
            sumB += qBlue(pixel);

            // Offset to the current line.
            int offset = x + y * oWidth;

            // Offset to the previous line.
            // equivalent to x + (y - 1) * oWidth;
            int offsetPrevious = offset - oWidth;

            // Accumulate current line and previous line.
            integral[offset].r = sumR + integral[offsetPrevious].r;
            integral[offset].g = sumG + integral[offsetPrevious].g;
            integral[offset].b = sumB + integral[offsetPrevious].b;
        }
    }
}

void BlurElement::setRadius(int radius)
{
    if (radius != this->m_radius) {
        this->m_radius = radius;
        emit this->radiusChanged();
    }
}

void BlurElement::resetRadius()
{
    this->setRadius(5);
}

QbPacket BlurElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame(src.size(), src.format());

    int oWidth = src.width() + 1;
    int oHeight = src.height() + 1;
    PixelUint32 *integral = new PixelUint32[oWidth * oHeight];
    this->integralImage(src, oWidth, oHeight, integral);

    int radius = this->m_radius;
    int radiusOffset = (radius - 1) / 2;

    for (int y = 0; y < src.height(); y++) {
        const QRgb *iLine = (const QRgb *) src.constScanLine(y);
        QRgb *oLine = (QRgb *) oFrame.scanLine(y);
        int yp = qMax(y - radiusOffset, 0);
        int kh = qMin(y + radiusOffset, src.height() - 1) - yp + 1;

        for (int x = 0; x < src.width(); x++) {
            int xp = qMax(x - radiusOffset, 0);
            int kw = qMin(x + radiusOffset, src.width() - 1) - xp + 1;

            PixelUint32 *p0 = integral + xp + yp * oWidth;
            PixelUint32 *p1 = p0 + kw;
            PixelUint32 *p2 = p0 + kh * oWidth;
            PixelUint32 *p3 = p2 + kw;

            qreal sumR = p0->r - p1->r - p2->r + p3->r;
            qreal sumG = p0->g - p1->g - p2->g + p3->g;
            qreal sumB = p0->b - p1->b - p2->b + p3->b;

            int ks = kw * kh;

            quint8 r = qBound(0., sumR / ks, 255.);
            quint8 g = qBound(0., sumG / ks, 255.);
            quint8 b = qBound(0., sumB / ks, 255.);

            oLine[x] = qRgba(r, g, b, qAlpha(iLine[x]));
        }
    }

    delete [] integral;

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
