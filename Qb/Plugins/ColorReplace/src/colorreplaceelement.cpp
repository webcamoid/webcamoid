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
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "colorreplaceelement.h"

ColorReplaceElement::ColorReplaceElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    qRegisterMetaType<QRgb>("QRgb");

    this->resetFrom();
    this->resetTo();
    this->resetRadius();
    this->resetDisable();
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
    if (from != this->m_from) {
        this->m_from = from;
        emit this->fromChanged();
    }
}

void ColorReplaceElement::setTo(QRgb to)
{
    if (to != this->m_to) {
        this->m_to = to;
        emit this->toChanged();
    }
}

void ColorReplaceElement::setRadius(qreal radius)
{
    if (radius != this->m_radius) {
        this->m_radius = radius;
        emit this->radiusChanged();
    }
}

void ColorReplaceElement::setDisable(bool disable)
{
    if (disable != this->m_disable) {
        this->m_disable = disable;
        emit this->disableChanged();
    }
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

    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

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

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
