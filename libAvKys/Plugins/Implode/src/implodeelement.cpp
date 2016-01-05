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

#include "implodeelement.h"

ImplodeElement::ImplodeElement(): AkElement()
{
    this->m_amount = 1.0;
}

QObject *ImplodeElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Implode/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Implode", (QObject *) this);
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

qreal ImplodeElement::amount() const
{
    return this->m_amount;
}

void ImplodeElement::setAmount(qreal amount)
{
    if (this->m_amount == amount)
        return;

    this->m_amount = amount;
    emit this->amountChanged(amount);
}

void ImplodeElement::resetAmount()
{
    this->setAmount(1.0);
}

AkPacket ImplodeElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());

    int xc = src.width() >> 1;
    int yc = src.height() >> 1;
    int radius = qMin(xc, yc);

    for (int y = 0; y < src.height(); y++) {
        const QRgb *iLine = (const QRgb *) src.constScanLine(y);
        QRgb *oLine = (QRgb *) oFrame.scanLine(y);
        int yDiff = y - yc;

        for (int x = 0; x < src.width(); x++) {
            int xDiff = x - xc;
            qreal distance = sqrt(xDiff * xDiff + yDiff * yDiff);

            if (distance >= radius)
                oLine[x] = iLine[x];
            else {
                qreal factor = pow(distance / radius, this->m_amount);

                int xp = factor * xDiff + xc;
                int yp = factor * yDiff + yc;

                xp = qBound(0, xp, oFrame.width() - 1);
                yp = qBound(0, yp, oFrame.height() - 1);

                const QRgb *line = (const QRgb *) src.constScanLine(yp);
                oLine[x] = line[xp];
            }
        }
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
