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
#include <QVariant>
#include <QVector>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "matrixtransform.h"

#define VALUE_SHIFT 8

class MatrixTransformPrivate
{
    public:
        MatrixTransform *self {nullptr};
        QString m_description {QObject::tr("Matrix transform")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        QVector<qreal> m_kernel;
        int m_ikernel[6];
        QMutex m_mutex;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        explicit MatrixTransformPrivate(MatrixTransform *self);
};

MatrixTransform::MatrixTransform(QObject *parent):
    QObject(parent)
{
    this->d = new MatrixTransformPrivate(this);
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

MatrixTransform::~MatrixTransform()
{
    delete this->d;
}

QString MatrixTransform::description() const
{
    return this->d->m_description;
}

AkElementType MatrixTransform::type() const
{
    return this->d->m_type;
}

AkElementCategory MatrixTransform::category() const
{
    return this->d->m_category;
}

void *MatrixTransform::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *MatrixTransform::create(const QString &id)
{
    Q_UNUSED(id)

    return new MatrixTransform;
}

int MatrixTransform::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/MatrixTransform",
                            this->d->m_description,
                            pluginPath,
                            QStringList(),
                            this->d->m_type,
                            this->d->m_category,
                            0,
                            this);
    akPluginManager->registerPlugin(pluginInfo);

    return 0;
}

QVariantList MatrixTransform::kernel() const
{
    QVariantList kernel;

    for (auto &e: this->d->m_kernel)
        kernel << e;

    return kernel;
}

void MatrixTransform::deleteThis(void *userData) const
{
    delete reinterpret_cast<MatrixTransform *>(userData);
}

QString MatrixTransform::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/MatrixTransform/share/qml/main.qml");
}

void MatrixTransform::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("MatrixTransform", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket MatrixTransform::iVideoStream(const AkVideoPacket &packet)
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
        this->oStream(dst);

    return dst;
}

void MatrixTransform::setKernel(const QVariantList &kernel)
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

void MatrixTransform::resetKernel()
{
    static const QVariantList kernel {
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0
    };

    this->setKernel(kernel);
}

MatrixTransformPrivate::MatrixTransformPrivate(MatrixTransform *self):
    self(self)
{

}

#include "moc_matrixtransform.cpp"
