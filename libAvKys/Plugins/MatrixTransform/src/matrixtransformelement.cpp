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
#include <QVariant>
#include <QVector>
#include <qrgb.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "matrixtransformelement.h"

#define VALUE_SHIFT 8

class MatrixTransformElementPrivate
{
    public:
        QVector<qreal> m_kernel;
        int m_ikernel[6];
        QMutex m_mutex;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
};

MatrixTransformElement::MatrixTransformElement(): AkElement()
{
    this->d = new MatrixTransformElementPrivate;
    this->d->m_kernel = {
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0
    };

    auto mult = 1 << VALUE_SHIFT;

    int ik[] {
        mult, 0, 0,
        mult, 0, 0,
    };
    memcpy(this->d->m_ikernel, ik, 6 * sizeof(int));
}

MatrixTransformElement::~MatrixTransformElement()
{
    delete this->d;
}

QVariantList MatrixTransformElement::kernel() const
{
    QVariantList kernel;

    for (auto &e: this->d->m_kernel)
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

AkPacket MatrixTransformElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    this->d->m_mutex.lock();

    int cx = src.caps().width() >> 1;
    int cy = src.caps().height() >> 1;
    int dxi = -(cx + this->d->m_ikernel[2]);
    int dy  = -(cy + this->d->m_ikernel[5]);
    auto mult = 1 << VALUE_SHIFT;

    for (int y = 0; y < src.caps().height(); y++) {
        auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));
        int dx = dxi;

        for (int x = 0; x < src.caps().width(); x++) {
            int xp = (dx * this->d->m_ikernel[0] + dy * this->d->m_ikernel[1] + cx * mult) >> VALUE_SHIFT;
            int yp = (dy * this->d->m_ikernel[3] + dx * this->d->m_ikernel[4] + cy * mult) >> VALUE_SHIFT;

            if (xp >= 0
                && xp < src.caps().width()
                && yp >= 0
                && yp < src.caps().height()) {
                auto iLine = reinterpret_cast<const QRgb *>(src.constLine(0, yp));
                oLine[x] = iLine[xp];
            } else {
                oLine[x] = qRgba(0, 0, 0, 0);
            }

            dx++;
        }

        dy++;
    }

    this->d->m_mutex.unlock();

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void MatrixTransformElement::setKernel(const QVariantList &kernel)
{
    QVector<qreal> k;

    for (const QVariant &e: kernel)
        k << e.toReal();

    if (this->d->m_kernel == k)
        return;

    this->d->m_kernel = k;

    auto det = k[0] * k[4] - k[1] * k[3];

    if (qFuzzyCompare(det, 0.0))
        det = 0.01;

    auto mult = (1 << VALUE_SHIFT) / det;

    int ik[] {
        qRound(mult * k[4]), -qRound(mult * k[3]), qRound(k[2]),
        qRound(mult * k[0]), -qRound(mult * k[1]), qRound(k[5]),
    };

    this->d->m_mutex.lock();
    memcpy(this->d->m_ikernel, ik, 6 * sizeof(int));
    this->d->m_mutex.unlock();

    emit this->kernelChanged(kernel);
}

void MatrixTransformElement::resetKernel()
{
    static const QVariantList kernel {
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0
    };

    this->setKernel(kernel);
}

#include "moc_matrixtransformelement.cpp"
