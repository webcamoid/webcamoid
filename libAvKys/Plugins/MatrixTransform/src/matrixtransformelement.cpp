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

#include <QVariant>
#include <QVector>
#include <QImage>
#include <QMutex>
#include <QQmlContext>
#include <akutils.h>
#include <akpacket.h>

#include "matrixtransformelement.h"

class MatrixTransformElementPrivate
{
    public:
        QVector<qreal> m_kernel;
        QMutex m_mutex;
};

MatrixTransformElement::MatrixTransformElement(): AkElement()
{
    this->d = new MatrixTransformElementPrivate;
    this->d->m_kernel = {
        1, 0, 0,
        0, 1, 0
    };
}

MatrixTransformElement::~MatrixTransformElement()
{
    delete this->d;
}

QVariantList MatrixTransformElement::kernel() const
{
    QVariantList kernel;

    for (const qreal &e: this->d->m_kernel)
        kernel << e;

    return kernel;
}

QString MatrixTransformElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/MatrixTransform/share/qml/main.qml");
}

void MatrixTransformElement::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("MatrixTransform", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void MatrixTransformElement::setKernel(const QVariantList &kernel)
{
    QVector<qreal> k;

    for (const QVariant &e: kernel)
        k << e.toReal();

    if (this->d->m_kernel == k)
        return;

    QMutexLocker(&this->d->m_mutex);
    this->d->m_kernel = k;
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

    this->d->m_mutex.lock();
    QVector<qreal> kernel = this->d->m_kernel;
    this->d->m_mutex.unlock();

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

#include "moc_matrixtransformelement.cpp"
