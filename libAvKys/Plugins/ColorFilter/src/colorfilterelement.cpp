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
#include <QtMath>
#include <akvideopacket.h>

#include "colorfilterelement.h"

class ColorFilterElementPrivate
{
    public:
        QRgb m_color {qRgb(0, 0, 0)};
        qreal m_radius {1.0};
        bool m_soft {false};
        bool m_disable {false};
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

qreal ColorFilterElement::radius() const
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

void ColorFilterElement::setColor(QRgb color)
{
    if (this->d->m_color == color)
        return;

    this->d->m_color = color;
    emit this->colorChanged(color);
}

void ColorFilterElement::setRadius(qreal radius)
{
    if (qFuzzyCompare(this->d->m_radius, radius))
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
    this->setRadius(1.0);
}

void ColorFilterElement::resetSoft()
{
    this->setSoft(false);
}

void ColorFilterElement::resetDisable()
{
    this->setDisable(false);
}

AkPacket ColorFilterElement::iStream(const AkPacket &packet)
{
    if (this->d->m_disable)
        akSend(packet)

    AkVideoPacket videoPacket(packet);
    auto src = videoPacket.toImage();

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());

    for (int y = 0; y < src.height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        auto dstLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            int r = qRed(srcLine[x]);
            int g = qGreen(srcLine[x]);
            int b = qBlue(srcLine[x]);

            int rf = qRed(this->d->m_color);
            int gf = qGreen(this->d->m_color);
            int bf = qBlue(this->d->m_color);

            int rd = r - rf;
            int gd = g - gf;
            int bd = b - bf;

            qreal k = sqrt(rd * rd + gd * gd + bd * bd);

            if (k <= this->d->m_radius) {
                if (this->d->m_soft) {
                    qreal p = k / this->d->m_radius;

                    int gray = qGray(srcLine[x]);

                    r = int(p * (gray - r) + r);
                    g = int(p * (gray - g) + g);
                    b = int(p * (gray - b) + b);

                    dstLine[x] = qRgba(r, g, b, qAlpha(srcLine[x]));
                } else
                    dstLine[x] = srcLine[x];
            } else {
                int gray = qGray(srcLine[x]);
                dstLine[x] = qRgba(gray, gray, gray, qAlpha(srcLine[x]));
            }
        }
    }

    auto oPacket = AkVideoPacket::fromImage(oFrame, videoPacket).toPacket();
    akSend(oPacket)
}

#include "moc_colorfilterelement.cpp"
