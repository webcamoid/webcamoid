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
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "colortransform.h"

class ColorTransformPrivate;

class KernelCallbacks:
    public IAkObjectPropertyCallbacks<QVector<qreal>>
{
    public:
        KernelCallbacks(ColorTransformPrivate *self);
        void valueChanged(const QVector<qreal> &kernel) override;

    private:
        ColorTransformPrivate *self;
};

class ColorTransformPrivate
{
    public:
    ColorTransform *self {nullptr};
    QString m_description {QObject::tr("Color Matrix Transform")};
    AkElementType m_type {AkElementType_VideoFilter};
    AkElementCategory m_category {AkElementCategory_VideoFilter};
    IAkPropertyDoubleList m_kernel {QObject::tr("Kernel"), {
                                        1.0, 0.0, 0.0, 0.0,
                                        0.0, 1.0, 0.0, 0.0,
                                        0.0, 0.0, 1.0, 0.0
                                    }};
    KernelCallbacks *m_kernelCallbacks {nullptr};
    qreal m_kernelData[12];
    AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

    explicit ColorTransformPrivate(ColorTransform *self);
    ~ColorTransformPrivate();
};

ColorTransform::ColorTransform(QObject *parent):
      QObject(parent)
{
    this->d = new ColorTransformPrivate(this);
    this->registerProperty("kernel", &this->d->m_kernel);
    this->d->m_kernel.subscribe(this->d->m_kernelCallbacks);
    memcpy(this->d->m_kernelData,
           this->d->m_kernel.value().constData(),
           12 * sizeof(qreal));
}

ColorTransform::~ColorTransform()
{
    delete this->d;
}

QString ColorTransform::description() const
{
    return this->d->m_description;
}

AkElementType ColorTransform::type() const
{
    return this->d->m_type;
}

AkElementCategory ColorTransform::category() const
{
    return this->d->m_category;
}

void *ColorTransform::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *ColorTransform::create(const QString &id)
{
    Q_UNUSED(id)

    return new ColorTransform;
}

int ColorTransform::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/ColorTransform",
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

void ColorTransform::deleteThis(void *userData) const
{
    delete reinterpret_cast<ColorTransform *>(userData);
}

QString ColorTransform::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/ColorTransform/share/qml/main.qml");
}

void ColorTransform::controlInterfaceConfigure(QQmlContext *context,
                                                      const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("ColorTransform", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket ColorTransform::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    auto kernel = this->d->m_kernelData;

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

    if (dst)
        emit this->oStream(dst);

    return dst;
}

ColorTransformPrivate::ColorTransformPrivate(ColorTransform *self):
      self(self)
{
    this->m_kernelCallbacks = new KernelCallbacks(this);
}

ColorTransformPrivate::~ColorTransformPrivate()
{
    delete this->m_kernelCallbacks;
}

KernelCallbacks::KernelCallbacks(ColorTransformPrivate *self):
    self(self)
{

}

void KernelCallbacks::valueChanged(const QVector<qreal> &kernel)
{
    auto size = qMin(12, kernel.size());

    if (size < 12) {
        auto defaultSize = qMin(12, self->m_kernel.defaultValue().size());

        memcpy(self->m_kernelData,
               self->m_kernel.defaultValue().constData(),
               defaultSize * sizeof(qreal));
    }

    if (size > 0) {
        memcpy(self->m_kernelData,
               kernel.constData(),
               size * sizeof(qreal));
    }
}

#include "moc_colortransform.cpp"
