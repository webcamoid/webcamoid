/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "oilpaintelement.h"
#include "defs.h"

OilPaintElement::OilPaintElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    this->resetRadius();
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
    if (radius < 0)
        radius = 0;

    if (radius != this->m_radius) {
        this->m_radius = radius;
        this->radiusChanged();
    }
}

void OilPaintElement::resetRadius()
{
    this->setRadius(2);
}

QbPacket OilPaintElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    int radius = this->m_radius;
    QImage oFrame(src.size(), src.format());
    quint16 histogram[256];
    QRgb *oLine = (QRgb *) oFrame.bits();
    int scanBlockLen = (radius << 1) + 1;
    QRgb *scanBlock[scanBlockLen];

    for (int y = 0; y < src.height(); y++) {
        int pos = y - radius;

        for (int j = 0; j < scanBlockLen; j++) {
            scanBlock[j] = (QRgb *) src.constScanLine(qBound(0, pos, src.height()));
            pos++;
        }

        for (int x = 0; x < src.width(); x++) {
            int minI = x - radius;
            int maxI = x + radius + 1;

            if (minI < 0)
                minI = 0;

            if (maxI > src.width())
                maxI = src.width();

            memset(histogram, 0, 512);
            quint16 max = 0;

            for (int j = 0; j < scanBlockLen; j++)
                for (int i = minI; i < maxI; i++) {
                    Pixel *p = (Pixel *) &scanBlock[j][i];
                    quint16 value = ++histogram[(p->r + p->g + p->b) / 3];

                    if (value > max) {
                        max = value;
                        *oLine = scanBlock[j][i];
                    }
                }

            oLine++;
        }
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
