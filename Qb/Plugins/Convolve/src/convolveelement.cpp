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

#include "convolveelement.h"

ConvolveElement::ConvolveElement(): QbElement()
{
    this->m_kernel << 0 << 0 << 0
                   << 0 << 1 << 0
                   << 0 << 0 << 0;
    this->m_kernelSize = QSize(3, 3);
    this->m_factor = QbFrac(1, 1);
    this->m_bias = 0;
}

QObject *ConvolveElement::controlInterface(QQmlEngine *engine,
                                           const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Convolve/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Convolve", (QObject *) this);
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

QVariantList ConvolveElement::kernel() const
{
    QVariantList kernel;

    foreach (int e, this->m_kernel)
        kernel << e;

    return kernel;
}

QSize ConvolveElement::kernelSize() const
{
    return this->m_kernelSize;
}

QbFrac ConvolveElement::factor() const
{
    return this->m_factor;
}

int ConvolveElement::bias() const
{
    return this->m_bias;
}

void ConvolveElement::setKernel(const QVariantList &kernel)
{
    QVector<int> k;

    foreach (QVariant e, kernel)
        k << e.toInt();

    if (this->m_kernel == k)
        return;

    QMutexLocker(&this->m_mutex);
    this->m_kernel = k;
    emit this->kernelChanged(kernel);
}

void ConvolveElement::setKernelSize(const QSize &kernelSize)
{
    if (this->m_kernelSize == kernelSize)
        return;

    QMutexLocker(&this->m_mutex);
    this->m_kernelSize = kernelSize;
    emit this->kernelSizeChanged(kernelSize);
}

void ConvolveElement::setFactor(const QbFrac &factor)
{
    if (this->m_factor == factor)
        return;

    QMutexLocker(&this->m_mutex);
    this->m_factor = factor;
    emit this->factorChanged(factor);
}

void ConvolveElement::setBias(int bias)
{
    if (this->m_bias == bias)
        return;

    QMutexLocker(&this->m_mutex);
    this->m_bias = bias;
    emit this->biasChanged(bias);
}

void ConvolveElement::resetKernel()
{
    QVariantList kernel;

    kernel << 0 << 0 << 0
           << 0 << 1 << 0
           << 0 << 0 << 0;

    this->setKernel(kernel);
}

void ConvolveElement::resetKernelSize()
{
    this->setKernelSize(QSize(3, 3));
}

void ConvolveElement::resetFactor()
{
    this->setFactor(QbFrac(1, 1));
}

void ConvolveElement::resetBias()
{
    this->setBias(0);
}

QbPacket ConvolveElement::iStream(const QbPacket &packet)
{
    QImage src = QbUtils::packetToImage(packet);

    if (src.isNull())
        return QbPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());

    this->m_mutex.lock();
    QVector<int> kernel = this->m_kernel;
    const int *kernelBits = kernel.constData();
    int factorNum = this->m_factor.num();
    int factorDen = this->m_factor.den();
    int kernelWidth = this->m_kernelSize.width();
    int kernelHeight = this->m_kernelSize.height();
    this->m_mutex.unlock();

    int minI = -(kernelWidth - 1) / 2;
    int maxI = (kernelWidth + 1) / 2;
    int minJ = -(kernelHeight - 1) / 2;
    int maxJ = (kernelHeight + 1) / 2;

    for (int y = 0; y < src.height(); y++) {
        const QRgb *iLine = (const QRgb *) src.constScanLine(y);
        QRgb *oLine = (QRgb *) oFrame.scanLine(y);

        for (int x = 0; x < src.width(); x++) {
            int r = 0;
            int g = 0;
            int b = 0;

            for (int j = minJ, k = 0; j < maxJ; j++) {
                int yp = qBound(0, y + j, src.height() - 1);
                const QRgb *iLine = (const QRgb *) src.constScanLine(yp);

                for (int i = minI; i < maxI; i++, k++) {
                    int xp = qBound(0, x + i, src.width() - 1);

                    if (kernelBits[k]) {
                        r += kernelBits[k] * qRed(iLine[xp]);
                        g += kernelBits[k] * qGreen(iLine[xp]);
                        b += kernelBits[k] * qBlue(iLine[xp]);
                    }
                }
            }

            if (factorNum) {
                r = factorNum * r / factorDen + this->m_bias;
                g = factorNum * g / factorDen + this->m_bias;
                b = factorNum * b / factorDen + this->m_bias;

                r = qBound(0, r, 255);
                g = qBound(0, g, 255);
                b = qBound(0, b, 255);
            } else {
                r = 255;
                g = 255;
                b = 255;
            }

            oLine[x] = qRgba(r, g, b, qAlpha(iLine[x]));
        }
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, packet);
    qbSend(oPacket)
}
