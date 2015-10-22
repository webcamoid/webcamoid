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

#include "oilpaintelement.h"

OilPaintElement::OilPaintElement(): QbElement()
{
    this->m_radius = 2;
}

QObject *OilPaintElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/OilPaint/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("OilPaint", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

int OilPaintElement::radius() const
{
    return this->m_radius;
}

void OilPaintElement::setRadius(int radius)
{
    if (this->m_radius == radius)
        return;

    this->m_radius = radius;
    this->radiusChanged(radius);
}

void OilPaintElement::resetRadius()
{
    this->setRadius(2);
}

QbPacket OilPaintElement::iStream(const QbPacket &packet)
{
    QImage src = QbUtils::packetToImage(packet);

    if (src.isNull())
        return QbPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);

    int radius = this->m_radius > 0? this->m_radius: 1;
    QImage oFrame(src.size(), src.format());
    int histogram[256];
    int scanBlockLen = (radius << 1) + 1;
    const QRgb *scanBlock[scanBlockLen];

    for (int y = 0; y < src.height(); y++) {
        QRgb *oLine = (QRgb *) oFrame.scanLine(y);

        for (int j = 0, pos = y - radius; j < scanBlockLen; j++, pos++) {
            int yp = qBound(0, pos, src.height());
            scanBlock[j] = (const QRgb *) src.constScanLine(yp);
        }

        for (int x = 0; x < src.width(); x++) {
            int minI = x - radius;
            int maxI = x + radius + 1;

            if (minI < 0)
                minI = 0;

            if (maxI > src.width())
                maxI = src.width();

            memset(histogram, 0, 256 * sizeof(int));
            int max = 0;
            QRgb oPixel = 0;

            for (int j = 0; j < scanBlockLen; j++)
                for (int i = minI; i < maxI; i++) {
                    QRgb pixel = scanBlock[j][i];
                    int value = ++histogram[qGray(pixel)];

                    if (value > max) {
                        max = value;
                        oPixel = pixel;
                    }
                }

            oLine[x] = oPixel;
        }
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, packet);
    qbSend(oPacket)
}
