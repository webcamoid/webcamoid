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

#include <QImage>
#include <QQmlContext>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "oilpaintelement.h"

class OilPaintElementPrivate
{
    public:
        int m_radius {2};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argb, 0, 0, {}}};
};

OilPaintElement::OilPaintElement(): AkElement()
{
    this->d = new OilPaintElementPrivate;
}

OilPaintElement::~OilPaintElement()
{
    delete this->d;
}

int OilPaintElement::radius() const
{
    return this->d->m_radius;
}

QString OilPaintElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/OilPaint/share/qml/main.qml");
}

void OilPaintElement::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("OilPaint", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket OilPaintElement::iVideoStream(const AkVideoPacket &packet)
{
    auto src = this->d->m_videoConverter.convertToImage(packet);

    if (src.isNull())
        return AkPacket();

    int radius = qMax(this->d->m_radius, 1);
    QImage oFrame(src.size(), src.format());
    int histogram[256];
    int scanBlockLen = (radius << 1) + 1;
    QVector<const QRgb *> scanBlock(scanBlockLen);

    for (int y = 0; y < src.height(); y++) {
        auto oLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int j = 0, pos = y - radius; j < scanBlockLen; j++, pos++) {
            int yp = qBound(0, pos, src.height() - 1);
            scanBlock[j] = reinterpret_cast<const QRgb *>(src.constScanLine(yp));
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

    auto oPacket = this->d->m_videoConverter.convert(oFrame, packet);

    if (oPacket)
        emit this->oStream(oPacket);

    return oPacket;
}

void OilPaintElement::setRadius(int radius)
{
    if (this->d->m_radius == radius)
        return;

    this->d->m_radius = radius;
    this->radiusChanged(radius);
}

void OilPaintElement::resetRadius()
{
    this->setRadius(2);
}

#include "moc_oilpaintelement.cpp"
