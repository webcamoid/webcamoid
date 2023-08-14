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

#include <QQmlContext>
#include <qrgb.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "oilpaintelement.h"

class OilPaintElementPrivate
{
    public:
        int m_radius {2};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
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
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    int radius = qMax(this->d->m_radius, 1);
    int scanBlockLen = (radius << 1) + 1;
    const QRgb *scanBlock[scanBlockLen];
    int histogram[256];

    for (int y = 0; y < src.caps().height(); y++) {
        for (int j = 0, pos = y - radius; j < scanBlockLen; j++, pos++) {
            int yp = qBound(0, pos, src.caps().height() - 1);
            scanBlock[j] = reinterpret_cast<const QRgb *>(src.constLine(0, yp));
        }

        auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            int minI = qMax(x - radius, 0);
            int maxI = qMin(x + radius + 1, src.caps().width());

            memset(histogram, 0, 256 * sizeof(int));
            int max = 0;
            QRgb oPixel = 0;

            for (int j = 0; j < scanBlockLen; j++) {
                auto line = scanBlock[j];

                for (int i = minI; i < maxI; i++) {
                    auto &pixel = line[i];
                    int value = ++histogram[qGray(pixel)];

                    if (value > max) {
                        max = value;
                        oPixel = pixel;
                    }
                }
            }

            oLine[x] = oPixel;
        }
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
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
