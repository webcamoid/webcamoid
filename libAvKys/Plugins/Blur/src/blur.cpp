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

#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "blur.h"
#include "pixel.h"

class BlurPrivate
{
    public:
        Blur *self {nullptr};
        QString m_description {QObject::tr("Blur")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        IAkPropertyInt m_radius {QObject::tr("Radius"), 5};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        explicit BlurPrivate(Blur *self);
        void integralImage(const AkVideoPacket &src, PixelU32 *integral);
};

Blur::Blur(QObject *parent):
      QObject(parent)
{
    this->d = new BlurPrivate(this);
    this->registerProperty("", &this->d->m_radius);
}

Blur::~Blur()
{
    delete this->d;
}

QString Blur::description() const
{
    return this->d->m_description;
}

AkElementType Blur::type() const
{
    return this->d->m_type;
}

AkElementCategory Blur::category() const
{
    return this->d->m_category;
}

void *Blur::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Blur::create(const QString &id)
{
    Q_UNUSED(id)

    return new Blur;
}

int Blur::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Blur",
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

void Blur::deleteThis(void *userData) const
{
    delete reinterpret_cast<Blur *>(userData);
}

QString Blur::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Blur/share/qml/main.qml");
}

void Blur::controlInterfaceConfigure(QQmlContext *context,
                                     const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Blur", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Blur::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    int oWidth = src.caps().width() + 1;
    int oHeight = src.caps().height() + 1;
    auto integral = new PixelU32[oWidth * oHeight];
    this->d->integralImage(src, integral);

    int radius = this->d->m_radius;

    for (int y = 0; y < src.caps().height(); ++y) {
        auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));
        int yp = qMax(y - radius, 0);
        int kh = qMin(y + radius, src.caps().height() - 1) - yp + 1;

        for (int x = 0; x < src.caps().width(); ++x) {
            int xp = qMax(x - radius, 0);
            int kw = qMin(x + radius, src.caps().width() - 1) - xp + 1;

            PixelU32 sum = integralSum(integral, oWidth, xp, yp, kw, kh);
            PixelU32 mean = sum / quint32(kw * kh);

            oLine[x] = qRgba(int(mean.r), int(mean.g), int(mean.b), int(mean.a));
        }
    }

    delete [] integral;

    if (dst)
        emit this->oStream(dst);

    return dst;
}

BlurPrivate::BlurPrivate(Blur *self):
    self(self)
{

}

void BlurPrivate::integralImage(const AkVideoPacket &src, PixelU32 *integral)
{
    int oWidth = src.caps().width() + 1;
    int oHeight = src.caps().height() + 1;

    auto integralLine = integral + oWidth;
    auto prevIntegralLine = integral;

    for (int y = 1; y < oHeight; ++y) {
        auto line = reinterpret_cast<const QRgb *>(src.constLine(0, y - 1));

        // Reset current line summation.
        PixelU32 sum;

        for (int x = 1; x < oWidth; ++x) {
            // Accumulate pixels in current line.
            sum += line[x - 1];

            // Accumulate current line and previous line.
            integralLine[x] = sum + prevIntegralLine[x];
        }

        integralLine += oWidth;
        prevIntegralLine += oWidth;
    }
}

#include "moc_blur.cpp"
