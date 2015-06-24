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

#include "primariescolorselement.h"

PrimariesColorsElement::PrimariesColorsElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetFactor();
}

QObject *PrimariesColorsElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/PrimariesColors/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("PrimariesColors", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

int PrimariesColorsElement::factor() const
{
    return this->m_factor;
}

void PrimariesColorsElement::setFactor(int factor)
{
    if (factor != this->m_factor) {
        this->m_factor = factor;
        emit this->factorChanged();
    }
}

void PrimariesColorsElement::resetFactor()
{
    this->setFactor(2);
}

QbPacket PrimariesColorsElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    int videoArea = src.width() * src.height();

    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    int f = this->m_factor + 1;
    int factor127 = (f * f - 3) * 127;
    int factorTot = f * f;

    if (factor127 < 0) {
        factor127 = 0;
        factorTot = 3;
    }

    for (int i = 0; i < videoArea; i++) {
        QRgb pixel = srcBits[i];

        int ri = qRed(pixel);
        int gi = qGreen(pixel);
        int bi = qBlue(pixel);

        quint8 mean;

        if (f > 32)
            mean = 127;
        else
            mean = (ri + gi + bi + factor127) / factorTot;

        int r = ri > mean? 255: 0;
        int g = gi > mean? 255: 0;
        int b = bi > mean? 255: 0;

        destBits[i] = qRgba(r, g, b, qAlpha(pixel));
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
