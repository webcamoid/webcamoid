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

#include <QtMath>

#include "colorreplaceelement.h"

ColorReplaceElement::ColorReplaceElement(): QbElement()
{
    this->m_from = qRgb(0, 0, 0);
    this->m_to = qRgb(0, 0, 0);
    this->m_radius = 1.0;
    this->m_disable = false;
}

QObject *ColorReplaceElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/ColorReplace/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("ColorReplace", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

QRgb ColorReplaceElement::from() const
{
    return this->m_from;
}

QRgb ColorReplaceElement::to() const
{
    return this->m_to;
}

qreal ColorReplaceElement::radius() const
{
    return this->m_radius;
}

bool ColorReplaceElement::disable() const
{
    return this->m_disable;
}

void ColorReplaceElement::setFrom(QRgb from)
{
    if (this->m_from == from)
        return;

    this->m_from = from;
    emit this->fromChanged(from);
}

void ColorReplaceElement::setTo(QRgb to)
{
    if (this->m_to == to)
        return;

    this->m_to = to;
    emit this->toChanged(to);
}

void ColorReplaceElement::setRadius(qreal radius)
{
    if (this->m_radius == radius)
        return;

    this->m_radius = radius;
    emit this->radiusChanged(radius);
}

void ColorReplaceElement::setDisable(bool disable)
{
    if (this->m_disable == disable)
        return;

    this->m_disable = disable;
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
    this->setRadius(1.0);
}

void ColorReplaceElement::resetDisable()
{
    this->setDisable(false);
}

QbPacket ColorReplaceElement::iStream(const QbPacket &packet)
{
    if (this->m_disable)
        qbSend(packet)

    QImage src = QbUtils::packetToImage(packet);

    if (src.isNull())
        return QbPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    int videoArea = src.width() * src.height();

    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    for (int i = 0; i < videoArea; i++) {
        int r = qRed(srcBits[i]);
        int g = qGreen(srcBits[i]);
        int b = qBlue(srcBits[i]);

        int rf = qRed(this->m_from);
        int gf = qGreen(this->m_from);
        int bf = qBlue(this->m_from);

        int rd = r - rf;
        int gd = g - gf;
        int bd = b - bf;

        qreal k = sqrt(rd * rd + gd * gd + bd * bd);

        if (k <= this->m_radius) {
            qreal p = k / this->m_radius;

            int rt = qRed(this->m_to);
            int gt = qGreen(this->m_to);
            int bt = qBlue(this->m_to);

            r = p * (r - rt) + rt;
            g = p * (g - gt) + gt;
            b = p * (b - bt) + bt;

            destBits[i] = qRgba(r, g, b, qAlpha(srcBits[i]));
        }
        else
            destBits[i] = srcBits[i];
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, packet);
    qbSend(oPacket)
}
