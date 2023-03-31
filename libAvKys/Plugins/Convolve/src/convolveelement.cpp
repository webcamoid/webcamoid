/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#include <QMutex>
#include <QQmlContext>
#include <QSize>
#include <QVariant>
#include <QVector>
#include <qrgb.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "convolveelement.h"

class ConvolveElementPrivate
{
    public:
        QVector<int> m_kernel;
        QSize m_kernelSize {3, 3};
        AkFrac m_factor {1, 1};
        QMutex m_mutex;
        int m_bias {0};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
};

ConvolveElement::ConvolveElement(): AkElement()
{
    this->d = new ConvolveElementPrivate;

    this->d->m_kernel = {
        0, 0, 0,
        0, 1, 0,
        0, 0, 0
    };
}

ConvolveElement::~ConvolveElement()
{
    delete this->d;
}

QVariantList ConvolveElement::kernel() const
{
    QVariantList kernel;

    for (auto &e: this->d->m_kernel)
        kernel << e;

    return kernel;
}

QSize ConvolveElement::kernelSize() const
{
    return this->d->m_kernelSize;
}

AkFrac ConvolveElement::factor() const
{
    return this->d->m_factor;
}

int ConvolveElement::bias() const
{
    return this->d->m_bias;
}

QString ConvolveElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Convolve/share/qml/main.qml");
}

void ConvolveElement::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Convolve", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket ConvolveElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    this->d->m_mutex.lock();

    if (this->d->m_kernel.size() < 9) {
        this->d->m_mutex.unlock();

        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    auto kernel = this->d->m_kernel.constData();
    qint64 factorNum = this->d->m_factor.num();
    qint64 factorDen = this->d->m_factor.den();
    int kernelWidth = this->d->m_kernelSize.width();
    int kernelHeight = this->d->m_kernelSize.height();

    int minI = -(kernelWidth - 1) / 2;
    int maxI = (kernelWidth + 1) / 2;
    int minJ = -(kernelHeight - 1) / 2;
    int maxJ = (kernelHeight + 1) / 2;

    for (int y = 0; y < src.caps().height(); y++) {
        auto iLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            int r = 0;
            int g = 0;
            int b = 0;

            for (int j = minJ, k = 0; j < maxJ; j++) {
                int yp = qBound(0, y + j, src.caps().height() - 1);
                auto iLine =
                        reinterpret_cast<const QRgb *>(src.constLine(0, yp));

                for (int i = minI; i < maxI; i++, k++) {
                    int xp = qBound(0, x + i, src.caps().width() - 1);

                    if (kernel[k]) {
                        auto pixel = iLine[xp];
                        r += kernel[k] * qRed(pixel);
                        g += kernel[k] * qGreen(pixel);
                        b += kernel[k] * qBlue(pixel);
                    }
                }
            }

            if (factorNum) {
                r = int(factorNum * r / factorDen + this->d->m_bias);
                g = int(factorNum * g / factorDen + this->d->m_bias);
                b = int(factorNum * b / factorDen + this->d->m_bias);

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

    this->d->m_mutex.unlock();

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void ConvolveElement::setKernel(const QVariantList &kernel)
{
    QVector<int> k;

    for (const QVariant &e: kernel)
        k << e.toInt();

    if (this->d->m_kernel == k)
        return;

    this->d->m_mutex.lock();
    this->d->m_kernel = k;
    this->d->m_mutex.unlock();
    emit this->kernelChanged(kernel);
}

void ConvolveElement::setKernelSize(const QSize &kernelSize)
{
    if (this->d->m_kernelSize == kernelSize)
        return;

    this->d->m_mutex.lock();
    this->d->m_kernelSize = kernelSize;
    this->d->m_mutex.unlock();
    emit this->kernelSizeChanged(kernelSize);
}

void ConvolveElement::setFactor(const AkFrac &factor)
{
    if (this->d->m_factor == factor)
        return;

    this->d->m_mutex.lock();
    this->d->m_factor = factor;
    this->d->m_mutex.unlock();
    emit this->factorChanged(factor);
}

void ConvolveElement::setBias(int bias)
{
    if (this->d->m_bias == bias)
        return;

    this->d->m_mutex.lock();
    this->d->m_bias = bias;
    this->d->m_mutex.unlock();
    emit this->biasChanged(bias);
}

void ConvolveElement::resetKernel()
{
    static const QVariantList kernel = {
        0, 0, 0,
        0, 1, 0,
        0, 0, 0
    };

    this->setKernel(kernel);
}

void ConvolveElement::resetKernelSize()
{
    this->setKernelSize(QSize(3, 3));
}

void ConvolveElement::resetFactor()
{
    this->setFactor(AkFrac(1, 1));
}

void ConvolveElement::resetBias()
{
    this->setBias(0);
}

#include "moc_convolveelement.cpp"
