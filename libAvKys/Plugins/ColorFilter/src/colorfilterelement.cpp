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
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "colorfilterelement.h"

class ColorFilterElementPrivate
{
    public:
        QRgb m_color {qRgb(0, 0, 0)};
        int m_radius {1};
        bool m_soft {false};
        bool m_disable {false};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
};

ColorFilterElement::ColorFilterElement(): AkElement()
{
    this->d = new ColorFilterElementPrivate;
}

ColorFilterElement::~ColorFilterElement()
{
    delete this->d;
}

QRgb ColorFilterElement::color() const
{
    return this->d->m_color;
}

int ColorFilterElement::radius() const
{
    return this->d->m_radius;
}

bool ColorFilterElement::soft() const
{
    return this->d->m_soft;
}

bool ColorFilterElement::disable() const
{
    return this->d->m_disable;
}

QString ColorFilterElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/ColorFilter/share/qml/main.qml");
}

void ColorFilterElement::controlInterfaceConfigure(QQmlContext *context,
                                                   const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("ColorFilter", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket ColorFilterElement::iVideoStream(const AkVideoPacket &packet)
{
    if (this->d->m_disable) {
        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    int rf = qRed(this->d->m_color);
    int gf = qGreen(this->d->m_color);
    int bf = qBlue(this->d->m_color);

    auto radius = this->d->m_radius;
    auto radius2 = radius * radius;

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto pixel = srcLine[x];
            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);

            int rd = r - rf;
            int gd = g - gf;
            int bd = b - bf;

            auto k = rd * rd + gd * gd + bd * bd;

            if (k <= radius2) {
                if (this->d->m_soft) {
                    qreal p = qSqrt(k) / radius;

                    int gray = qGray(pixel);

                    r = int(p * (gray - r) + r);
                    g = int(p * (gray - g) + g);
                    b = int(p * (gray - b) + b);

                    dstLine[x] = qRgba(r, g, b, qAlpha(pixel));
                } else {
                    dstLine[x] = pixel;
                }
            } else {
                int gray = qGray(pixel);
                dstLine[x] = qRgba(gray, gray, gray, qAlpha(pixel));
            }
        }
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void ColorFilterElement::setColor(QRgb color)
{
    if (this->d->m_color == color)
        return;

    this->d->m_color = color;
    emit this->colorChanged(color);
}

void ColorFilterElement::setRadius(int radius)
{
    if (this->d->m_radius == radius)
        return;

    this->d->m_radius = radius;
    emit this->radiusChanged(radius);
}

void ColorFilterElement::setSoft(bool soft)
{
    if (this->d->m_soft == soft)
        return;

    this->d->m_soft = soft;
    emit this->softChanged(soft);
}

void ColorFilterElement::setDisable(bool disable)
{
    if (this->d->m_disable == disable)
        return;

    this->d->m_disable = disable;
    emit this->disableChanged(disable);
}

void ColorFilterElement::resetColor()
{
    this->setColor(qRgb(0, 0, 0));
}

void ColorFilterElement::resetRadius()
{
    this->setRadius(1);
}

void ColorFilterElement::resetSoft()
{
    this->setSoft(false);
}

void ColorFilterElement::resetDisable()
{
    this->setDisable(false);
}

#include "moc_colorfilterelement.cpp"
