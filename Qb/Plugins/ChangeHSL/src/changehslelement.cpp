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

#include "changehslelement.h"

ChangeHSLElement::ChangeHSLElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetKernel();
}

QObject *ChangeHSLElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/ChangeHSL/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("ChangeHSL", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

QVariantList ChangeHSLElement::kernel() const
{
    QVariantList kernel;

    foreach (qreal e, this->m_kernel)
        kernel << e;

    return kernel;
}

void ChangeHSLElement::setKernel(const QVariantList &kernel)
{
    QVector<qreal> k;

    foreach (QVariant e, kernel)
        k << e.toReal();

    if (k != this->m_kernel)
        this->m_kernel = k;
}

void ChangeHSLElement::resetKernel()
{
    QVariantList kernel;

    kernel << 1 << 0 << 0 << 0
           << 0 << 1 << 0 << 0
           << 0 << 0 << 1 << 0;

    this->setKernel(kernel);
}

QbPacket ChangeHSLElement::iStream(const QbPacket &packet)
{
    if (this->m_kernel.size() < 12)
        qbSend(packet)

    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    int videoArea = src.width() * src.height();

    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    QVector<qreal> kernel = this->m_kernel;

    for (int i = 0; i < videoArea; i++) {
        int h;
        int s;
        int l;
        int a;

        QColor(srcBits[i]).getHsl(&h, &s, &l, &a);

        int ht = h * kernel[0] + s * kernel[1] + l * kernel[2]  + kernel[3];
        int st = h * kernel[4] + s * kernel[5] + l * kernel[6]  + kernel[7];
        int lt = h * kernel[8] + s * kernel[9] + l * kernel[10] + kernel[11];

        ht = qMax(0, ht);
        st = qBound(0, st, 255);
        lt = qBound(0, lt, 255);

        QColor color;
        color.setHsl(ht, st, lt, a);

        destBits[i] = color.rgba();
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
