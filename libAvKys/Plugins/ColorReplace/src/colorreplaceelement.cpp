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

#include "colorreplaceelement.h"

class ColorReplaceElementPrivate
{
    public:
        QRgb m_from {qRgb(0, 0, 0)};
        QRgb m_to {qRgb(0, 0, 0)};
        int m_radius {1};
        bool m_soft {true};
        bool m_disable {false};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
};

ColorReplaceElement::ColorReplaceElement(): AkElement()
{
    this->d = new ColorReplaceElementPrivate;
}

ColorReplaceElement::~ColorReplaceElement()
{
    delete this->d;
}

QRgb ColorReplaceElement::from() const
{
    return this->d->m_from;
}

QRgb ColorReplaceElement::to() const
{
    return this->d->m_to;
}

int ColorReplaceElement::radius() const
{
    return this->d->m_radius;
}

bool ColorReplaceElement::soft() const
{
    return this->d->m_soft;
}

bool ColorReplaceElement::disable() const
{
    return this->d->m_disable;
}

QString ColorReplaceElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/ColorReplace/share/qml/main.qml");
}

void ColorReplaceElement::controlInterfaceConfigure(QQmlContext *context,
                                                    const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("ColorReplace", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket ColorReplaceElement::iVideoStream(const AkVideoPacket &packet)
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

    int rf = qRed(this->d->m_from);
    int gf = qGreen(this->d->m_from);
    int bf = qBlue(this->d->m_from);

    int rt = qRed(this->d->m_to);
    int gt = qGreen(this->d->m_to);
    int bt = qBlue(this->d->m_to);

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

                    r = int(p * (r - rt) + rt);
                    g = int(p * (g - gt) + gt);
                    b = int(p * (b - bt) + bt);

                    dstLine[x] = qRgba(r, g, b, qAlpha(pixel));
                } else {
                    dstLine[x] = qRgba(rt, gt, bt, qAlpha(pixel));
                }
            } else {
                dstLine[x] = pixel;
            }
        }
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void ColorReplaceElement::setFrom(QRgb from)
{
    if (this->d->m_from == from)
        return;

    this->d->m_from = from;
    emit this->fromChanged(from);
}

void ColorReplaceElement::setTo(QRgb to)
{
    if (this->d->m_to == to)
        return;

    this->d->m_to = to;
    emit this->toChanged(to);
}

void ColorReplaceElement::setRadius(int radius)
{
    if (this->d->m_radius == radius)
        return;

    this->d->m_radius = radius;
    emit this->radiusChanged(radius);
}

void ColorReplaceElement::setSoft(bool soft)
{
    if (this->d->m_soft == soft)
        return;

    this->d->m_soft = soft;
    emit this->softChanged(soft);
}

void ColorReplaceElement::setDisable(bool disable)
{
    if (this->d->m_disable == disable)
        return;

    this->d->m_disable = disable;
    emit this->disableChanged(disable);
}

void ColorReplaceElement::resetFrom()
{
    this->setFrom(qRgb(0, 0, 0));
}

void ColorReplaceElement::resetTo()
{
    this->setTo(qRgb(0, 0, 0));
}

void ColorReplaceElement::resetRadius()
{
    this->setRadius(1);
}

void ColorReplaceElement::resetSoft()
{
    this->setSoft(true);
}

void ColorReplaceElement::resetDisable()
{
    this->setDisable(false);
}

#include "moc_colorreplaceelement.cpp"
