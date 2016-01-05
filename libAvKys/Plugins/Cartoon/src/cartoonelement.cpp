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

#include "cartoonelement.h"

CartoonElement::CartoonElement(): AkElement()
{
    this->m_threshold = 95;
    this->m_levels = 8;
}

QObject *CartoonElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Cartoon/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Cartoon", (QObject *) this);
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

int CartoonElement::threshold() const
{
    return this->m_threshold;
}

int CartoonElement::levels() const
{
    return this->m_levels;
}

void CartoonElement::setThreshold(int threshold)
{
    if (this->m_threshold == threshold)
        return;

    this->m_threshold = threshold;
    emit this->thresholdChange(threshold);
}

void CartoonElement::setLevels(int levels)
{
    if (this->m_levels == levels)
        return;

    this->m_levels = levels;
    emit this->levelsChange(levels);
}

void CartoonElement::resetThreshold()
{
    this->setThreshold(95);
}

void CartoonElement::resetLevels()
{
    this->setLevels(8);
}

AkPacket CartoonElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());

    int videoArea = src.width() * src.height();
    const QRgb *srcPtr = (const QRgb *) src.constBits();
    QVector<quint8> gray(videoArea);
    quint8 *grayPtr = gray.data();

    for (int i = 0; i < videoArea; i++)
        grayPtr[i] = qGray(srcPtr[i]);

    qreal k = log(1531) / 255.;
    int threshold = exp(k * (255 - this->m_threshold)) - 1;

    for (int y = 0; y < src.height(); y++) {
        const QRgb *srcLine = (const QRgb *) src.constScanLine(y);;
        QRgb *dstLine = (QRgb *) oFrame.constScanLine(y);;

        size_t yOffset = y * src.width();
        const quint8 *grayLine = gray.constData() + yOffset;

        const quint8 *grayLine_m1 = y < 1? grayLine: grayLine - src.width();
        const quint8 *grayLine_p1 = y >= src.height() - 1? grayLine: grayLine + src.width();

        for (int x = 0; x < src.width(); x++) {
            int x_m1 = x < 1? x: x - 1;
            int x_p1 = x >= src.width() - 1? x: x + 1;

            int gradX = grayLine_m1[x_p1]
                      + 2 * grayLine[x_p1]
                      + grayLine_p1[x_p1]
                      - grayLine_m1[x_m1]
                      - 2 * grayLine[x_m1]
                      - grayLine_p1[x_m1];

            int gradY = grayLine_m1[x_m1]
                      + 2 * grayLine_m1[x]
                      + grayLine_m1[x_p1]
                      - grayLine_p1[x_m1]
                      - 2 * grayLine_p1[x]
                      - grayLine_p1[x_p1];

            int grad = qAbs(gradX) + qAbs(gradY);

            if (grad >= threshold)
                dstLine[x] = qRgb(0, 0, 0);
            else {
                int r = this->threshold(qRed(srcLine[x]), this->m_levels);
                int g = this->threshold(qGreen(srcLine[x]), this->m_levels);
                int b = this->threshold(qBlue(srcLine[x]), this->m_levels);

                dstLine[x] = qRgb(r, g, b);
            }
        }
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
