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

#include "matrixtransformelement.h"

MatrixTransformElement::MatrixTransformElement(): AkElement()
{
    this->m_kernel = {
        1, 0, 0,
        0, 1, 0
    };
}

QObject *MatrixTransformElement::controlInterface(QQmlEngine *engine,
                                                  const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/MatrixTransform/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("MatrixTransform", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
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

QVariantList MatrixTransformElement::kernel() const
{
    QVariantList kernel;

    for (const qreal &e: this->m_kernel)
        kernel << e;

    return kernel;
}

void MatrixTransformElement::setKernel(const QVariantList &kernel)
{
    QVector<qreal> k;

    for (const QVariant &e: kernel)
        k << e.toReal();

    if (this->m_kernel == k)
        return;

    QMutexLocker(&this->m_mutex);
    this->m_kernel = k;
    emit this->kernelChanged(kernel);
}

void MatrixTransformElement::resetKernel()
{
    static const QVariantList kernel = {
        1, 0, 0,
        0, 1, 0
    };

    this->setKernel(kernel);
}

AkPacket MatrixTransformElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame = QImage(src.size(), src.format());

    this->m_mutex.lock();
    QVector<qreal> kernel = this->m_kernel;
    this->m_mutex.unlock();

    qreal det = kernel[0] * kernel[4] - kernel[1] * kernel[3];

    QRect rect(0, 0, src.width(), src.height());
    int cx = src.width() >> 1;
    int cy = src.height() >> 1;

    for (int y = 0; y < src.height(); y++) {
        QRgb *oLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            int dx = int(x - cx - kernel[2]);
            int dy = int(y - cy - kernel[5]);

            int xp = int(cx + (dx * kernel[4] - dy * kernel[3]) / det);
            int yp = int(cy + (dy * kernel[0] - dx * kernel[1]) / det);

            if (rect.contains(xp, yp)) {
                const QRgb *iLine = reinterpret_cast<const QRgb *>(src.constScanLine(yp));
                oLine[x] = iLine[xp];
            } else
                oLine[x] = qRgba(0, 0, 0, 0);
        }
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
