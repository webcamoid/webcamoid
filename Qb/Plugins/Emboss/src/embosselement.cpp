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

#include "embosselement.h"

EmbossElement::EmbossElement(): QbElement()
{
    this->m_factor = 1;
    this->m_bias = 128;
}

QObject *EmbossElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Emboss/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Emboss", (QObject *) this);
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

qreal EmbossElement::factor() const
{
    return this->m_factor;
}

qreal EmbossElement::bias() const
{
    return this->m_bias;
}

void EmbossElement::setFactor(qreal factor)
{
    if (this->m_factor == factor)
        return;

    this->m_factor = factor;
    emit this->factorChanged(factor);
}

void EmbossElement::setBias(qreal bias)
{
    if (bias != this->m_bias)
        return;

    this->m_bias = bias;
    emit this->biasChanged(bias);
}

void EmbossElement::resetFactor()
{
    this->setFactor(1);
}

void EmbossElement::resetBias()
{
    this->setBias(128);
}

QbPacket EmbossElement::iStream(const QbPacket &packet)
{
    QImage src = QbUtils::packetToImage(packet);

    if (src.isNull())
        return QbPacket();

    src = src.convertToFormat(QImage::Format_Grayscale8);
    QImage oFrame(src.size(), src.format());

    quint8 *srcBits = (quint8 *) src.bits();
    quint8 *destBits = (quint8 *) oFrame.bits();

    int widthMin = src.width() - 1;
    int widthMax = src.width() + 1;
    int heightMin = src.height() - 1;

    memcpy(oFrame.scanLine(0),
           src.constScanLine(0),
           src.width());

    memcpy(oFrame.scanLine(heightMin),
           src.constScanLine(0),
           src.width());

    for (int y = 0; y < src.height(); y++) {
        int xOffset = y * src.width();

        destBits[xOffset] = srcBits[xOffset];
        destBits[xOffset + widthMin] = srcBits[xOffset + widthMin];
    }

    for (int y = 1; y < heightMin; y++) {
        int xOffset = y * src.width();

        for (int x = 1; x < widthMin; x++) {
            int pixel = x + xOffset;

            int gray = - srcBits[pixel - widthMax]
                       - srcBits[pixel - src.width()]
                       - srcBits[pixel - 1]
                       + srcBits[pixel + 1]
                       + srcBits[pixel + src.width()]
                       + srcBits[pixel + widthMax];

            gray = this->m_factor * gray + this->m_bias;

            destBits[pixel] = qBound(0, gray, 255);
        }
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, packet);
    qbSend(oPacket)
}
