/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include <QColor>

#include "changehslelement.h"

ChangeHSLElement::ChangeHSLElement(): AkElement()
{
    this->m_kernel = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0
    };
}

QObject *ChangeHSLElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/ChangeHSL/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("ChangeHSL", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
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

QVariantList ChangeHSLElement::kernel() const
{
    QVariantList kernel;

    for (const qreal &e: this->m_kernel)
        kernel << e;

    return kernel;
}

void ChangeHSLElement::setKernel(const QVariantList &kernel)
{
    QVector<qreal> k;

    for (const QVariant &e: kernel)
        k << e.toReal();

    if (this->m_kernel == k)
        return;

    this->m_kernel = k;
    emit this->kernelChanged(kernel);
}

void ChangeHSLElement::resetKernel()
{
    static const QVariantList kernel = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0
    };

    this->setKernel(kernel);
}

AkPacket ChangeHSLElement::iStream(const AkPacket &packet)
{
    if (this->m_kernel.size() < 12)
        akSend(packet)

    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());
    QVector<qreal> kernel = this->m_kernel;

    for (int y = 0; y < src.height(); y++) {
        const QRgb *srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        QRgb *dstLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            int h;
            int s;
            int l;
            int a;

            QColor(srcLine[x]).getHsl(&h, &s, &l, &a);

            int ht = int(h * kernel[0] + s * kernel[1] + l * kernel[2]  + kernel[3]);
            int st = int(h * kernel[4] + s * kernel[5] + l * kernel[6]  + kernel[7]);
            int lt = int(h * kernel[8] + s * kernel[9] + l * kernel[10] + kernel[11]);

            ht = qBound(0, ht, 359);
            st = qBound(0, st, 255);
            lt = qBound(0, lt, 255);

            QColor color;
            color.setHsl(ht, st, lt, a);

            dstLine[x] = color.rgba();
        }
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
