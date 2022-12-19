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
#include <QVector>
#include <qrgb.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "colortransformelement.h"

class ColorTransformElementPrivate
{
    public:
        QMutex m_mutex;
        QVector<qreal> m_kernel;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
};

ColorTransformElement::ColorTransformElement(): AkElement()
{
    this->d = new ColorTransformElementPrivate;

    this->d->m_kernel = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0
    };
}

ColorTransformElement::~ColorTransformElement()
{
    delete this->d;
}

QVariantList ColorTransformElement::kernel() const
{
    QVariantList kernel;

    for (auto &e: this->d->m_kernel)
        kernel << e;

    return kernel;
}

QString ColorTransformElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/ColorTransform/share/qml/main.qml");
}

void ColorTransformElement::controlInterfaceConfigure(QQmlContext *context,
                                                      const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("ColorTransform", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket ColorTransformElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    this->d->m_mutex.lock();

    if (this->d->m_kernel.size() < 12) {
        this->d->m_mutex.unlock();

        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    auto kernel = this->d->m_kernel.data();

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            int r = qRed(srcLine[x]);
            int g = qGreen(srcLine[x]);
            int b = qBlue(srcLine[x]);

            int rt = int(r * kernel[0] + g * kernel[1] + b * kernel[2]  + kernel[3]);
            int gt = int(r * kernel[4] + g * kernel[5] + b * kernel[6]  + kernel[7]);
            int bt = int(r * kernel[8] + g * kernel[9] + b * kernel[10] + kernel[11]);

            rt = qBound(0, rt, 255);
            gt = qBound(0, gt, 255);
            bt = qBound(0, bt, 255);

            dstLine[x] = qRgba(rt, gt, bt, qAlpha(srcLine[x]));
        }
    }

    this->d->m_mutex.unlock();

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void ColorTransformElement::setKernel(const QVariantList &kernel)
{
    QVector<qreal> k;

    for (const QVariant &e: kernel)
        k << e.toReal();

    if (this->d->m_kernel == k)
        return;

    this->d->m_mutex.lock();
    this->d->m_kernel = k;
    this->d->m_mutex.unlock();
    emit this->kernelChanged(kernel);
}

void ColorTransformElement::resetKernel()
{
    QVariantList kernel {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0
    };

    this->setKernel(kernel);
}

#include "moc_colortransformelement.cpp"
