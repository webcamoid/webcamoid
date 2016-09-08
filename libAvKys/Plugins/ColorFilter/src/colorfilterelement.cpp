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

#include "colorfilterelement.h"

ColorFilterElement::ColorFilterElement(): AkElement()
{
    this->m_color = qRgb(0, 0, 0);
    this->m_radius = 1.0;
    this->m_soft = false;
    this->m_disable = false;
}

QObject *ColorFilterElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/ColorFilter/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("ColorFilter", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
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

QRgb ColorFilterElement::color() const
{
    return this->m_color;
}

qreal ColorFilterElement::radius() const
{
    return this->m_radius;
}

bool ColorFilterElement::soft() const
{
    return this->m_soft;
}

bool ColorFilterElement::disable() const
{
    return this->m_disable;
}

void ColorFilterElement::setColor(QRgb color)
{
    if (this->m_color == color)
        return;

    this->m_color = color;
    emit this->colorChanged(color);
}

void ColorFilterElement::setRadius(qreal radius)
{
    if (qFuzzyCompare(this->m_radius, radius))
        return;

    this->m_radius = radius;
    emit this->radiusChanged(radius);
}

void ColorFilterElement::setSoft(bool soft)
{
    if (this->m_soft == soft)
        return;

    this->m_soft = soft;
    emit this->softChanged(soft);
}

void ColorFilterElement::setDisable(bool disable)
{
    if (this->m_disable == disable)
        return;

    this->m_disable = disable;
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
    if (this->m_disable)
        akSend(packet)

    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());

    for (int y = 0; y < src.height(); y++) {
        const QRgb *srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        QRgb *dstLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            int r = qRed(srcLine[x]);
            int g = qGreen(srcLine[x]);
            int b = qBlue(srcLine[x]);

            int rf = qRed(this->m_color);
            int gf = qGreen(this->m_color);
            int bf = qBlue(this->m_color);

            int rd = r - rf;
            int gd = g - gf;
            int bd = b - bf;

            qreal k = sqrt(rd * rd + gd * gd + bd * bd);

            if (k <= this->m_radius) {
                if (this->m_soft) {
                    qreal p = k / this->m_radius;

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

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
