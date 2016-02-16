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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
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
    context->setContextProperty("ColorFilter", (QObject *) this);
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
    if (this->m_radius == radius)
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

    int videoArea = src.width() * src.height();
    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    for (int i = 0; i < videoArea; i++) {
        int r = qRed(srcBits[i]);
        int g = qGreen(srcBits[i]);
        int b = qBlue(srcBits[i]);

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

                int gray = qGray(srcBits[i]);

                r = p * (gray - r) + r;
                g = p * (gray - g) + g;
                b = p * (gray - b) + b;

                destBits[i] = qRgba(r, g, b, qAlpha(srcBits[i]));
            } else
                destBits[i] = srcBits[i];
        } else {
            int gray = qGray(srcBits[i]);
            destBits[i] = qRgba(gray, gray, gray, qAlpha(srcBits[i]));
        }
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
