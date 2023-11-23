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
#include <QSize>
#include <QVariant>
#include <QVector>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "convolve.h"

class ConvolvePrivate;

class KernelCallbacks: public IAkObjectPropertyCallbacks<QVector<qint32>>
{
    public:
        KernelCallbacks(ConvolvePrivate *self);
        void valueChanged(const QVector<qint32> &value) override;

    private:
        ConvolvePrivate *self;
};

class ConvolvePrivate
{
    public:
        Convolve *self {nullptr};
        QString m_description {QObject::tr("Convolve Matrix")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        IAkPropertyIntList m_kernel {QObject::tr("Kernel"), {
                                        0, 0, 0,
                                        0, 1, 0,
                                        0, 0, 0
                                     }};
        IAkPropertyFrac m_factor {QObject::tr("Factor"), {1, 1}};
        IAkPropertyInt m_bias {QObject::tr("Bias"), 0};
        KernelCallbacks *m_kernelCallbacks {nullptr};
        int m_kernelData[9];
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        explicit ConvolvePrivate(Convolve *self);
        ~ConvolvePrivate();
};

Convolve::Convolve(QObject *parent):
      QObject(parent)
{
    this->d = new ConvolvePrivate(this);

    this->registerProperty("kernel", &this->d->m_kernel);
    this->registerProperty("factor", &this->d->m_factor);
    this->registerProperty("bias", &this->d->m_bias);

    this->d->m_kernel.subscribe(this->d->m_kernelCallbacks);

    memcpy(this->d->m_kernelData,
           this->d->m_kernel.value().constData(),
           9 * sizeof(int));
}

Convolve::~Convolve()
{
    delete this->d;
}

QString Convolve::description() const
{
    return this->d->m_description;
}

AkElementType Convolve::type() const
{
    return this->d->m_type;
}

AkElementCategory Convolve::category() const
{
    return this->d->m_category;
}

void *Convolve::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Convolve::create(const QString &id)
{
    Q_UNUSED(id)

    return new Convolve;
}

int Convolve::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Convolve",
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

void Convolve::deleteThis(void *userData) const
{
    delete reinterpret_cast<Convolve *>(userData);
}

QString Convolve::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Convolve/share/qml/main.qml");
}

void Convolve::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Convolve", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Convolve::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    auto &kernel = this->d->m_kernelData;
    auto factor = this->d->m_factor.value();
    qint64 factorNum = factor.num();
    qint64 factorDen = factor.den();

    for (int y = 0; y < src.caps().height(); ++y) {
        auto iLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto y_m1 = qMax(y - 1, 0);
        auto iLine_m1 = reinterpret_cast<const QRgb *>(src.constLine(0, y_m1));
        auto y_p1 = qMax(y + 1, src.caps().height() - 1);
        auto iLine_p1 = reinterpret_cast<const QRgb *>(src.constLine(0, y_p1));
        auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); ++x) {
            auto x_m1 = qMax(x - 1, 0);
            auto x_p1 = qMax(x + 1, src.caps().width() - 1);

            const QRgb pixel [] {
                iLine_m1[x_m1], iLine_m1[x], iLine_m1[x_p1],
                   iLine[x_m1],    iLine[x],    iLine[x_p1],
                iLine_p1[x_m1], iLine_p1[x], iLine_p1[x_p1]
            };

            qint64 r = 0;
            qint64 g = 0;
            qint64 b = 0;

            for (int i = 0; i < 9; ++i) {
                r += kernel[i] * qRed(pixel[i]);
                g += kernel[i] * qGreen(pixel[i]);
                b += kernel[i] * qBlue(pixel[i]);
            }

            if (factorNum) {
                r = (factorNum * r + this->d->m_bias * factorDen) / factorDen;
                g = (factorNum * g + this->d->m_bias * factorDen) / factorDen;
                b = (factorNum * b + this->d->m_bias * factorDen) / factorDen;

                r = qBound(0, r, 255);
                g = qBound(0, g, 255);
                b = qBound(0, b, 255);
            } else {
                r = 255;
                g = 255;
                b = 255;
            }

            oLine[x] = qRgba(int(r), int(g), int(b), qAlpha(iLine[x]));
        }
    }

    if (dst)
        this->oStream(dst);

    return dst;
}

KernelCallbacks::KernelCallbacks(ConvolvePrivate *self):
    self(self)
{

}

void KernelCallbacks::valueChanged(const QVector<qint32> &value)
{
    auto size = qMin(9, value.size());

    if (size < 9) {
        auto defaultSize = qMin(9, self->m_kernel.defaultValue().size());

        memcpy(self->m_kernelData,
               self->m_kernel.defaultValue().constData(),
               defaultSize * sizeof(qint32));
    }

    if (size > 0)
        memcpy(self->m_kernelData,
               value.constData(),
               size * sizeof(qint32));
}

ConvolvePrivate::ConvolvePrivate(Convolve *self):
      self(self)
{
    this->m_kernelCallbacks = new KernelCallbacks(this);
}

ConvolvePrivate::~ConvolvePrivate()
{
    delete this->m_kernelCallbacks;
}

#include "moc_convolve.cpp"
