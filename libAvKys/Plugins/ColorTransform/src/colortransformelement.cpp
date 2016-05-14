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

#include "colortransformelement.h"

ColorTransformElement::ColorTransformElement(): AkElement()
{
    this->m_kernel << 1 << 0 << 0 << 0
                   << 0 << 1 << 0 << 0
                   << 0 << 0 << 1 << 0;
}

QObject *ColorTransformElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/ColorTransform/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("ColorTransform", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
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

QVariantList ColorTransformElement::kernel() const
{
    QVariantList kernel;

    foreach (qreal e, this->m_kernel)
        kernel << e;

    return kernel;
}

void ColorTransformElement::setKernel(const QVariantList &kernel)
{
    QVector<qreal> k;

    foreach (QVariant e, kernel)
        k << e.toReal();

    if (this->m_kernel == k)
        return;

    this->m_kernel = k;
    emit this->kernelChanged(kernel);
}

void ColorTransformElement::resetKernel()
{
    QVariantList kernel;

    kernel << 1 << 0 << 0 << 0
           << 0 << 1 << 0 << 0
           << 0 << 0 << 1 << 0;

    this->setKernel(kernel);
}

AkPacket ColorTransformElement::iStream(const AkPacket &packet)
{
    if (this->m_kernel.size() < 12)
        akSend(packet)

    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    int videoArea = src.width() * src.height();

    QImage oFrame(src.size(), src.format());

    const QRgb *srcBits = reinterpret_cast<const QRgb *>(src.constBits());
    QRgb *destBits = reinterpret_cast<QRgb *>(oFrame.bits());

    QVector<qreal> kernel = this->m_kernel;

    for (int i = 0; i < videoArea; i++) {
        int r = qRed(srcBits[i]);
        int g = qGreen(srcBits[i]);
        int b = qBlue(srcBits[i]);

        int rt = int(r * kernel[0] + g * kernel[1] + b * kernel[2]  + kernel[3]);
        int gt = int(r * kernel[4] + g * kernel[5] + b * kernel[6]  + kernel[7]);
        int bt = int(r * kernel[8] + g * kernel[9] + b * kernel[10] + kernel[11]);

        rt = qBound(0, rt, 255);
        gt = qBound(0, gt, 255);
        bt = qBound(0, bt, 255);

        destBits[i] = qRgba(rt, gt, bt, qAlpha(srcBits[i]));
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
