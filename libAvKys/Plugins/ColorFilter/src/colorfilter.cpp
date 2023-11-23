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

#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "colorfilter.h"

class ColorFilterPrivate
{
    public:
        ColorFilter *self {nullptr};
        QString m_description {QObject::tr("Color Filter")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        IAkPropertyColor m_color {QObject::tr("Color"), qRgb(0, 0, 0)};
        IAkPropertyInt m_radius {QObject::tr("Radius"), 1};
        IAkPropertyBool m_soft {QObject::tr("Soft coloring"), false};
        IAkPropertyBool m_disable {QObject::tr("Disable"), false};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        explicit ColorFilterPrivate(ColorFilter *self);
};

ColorFilter::ColorFilter(QObject *parent):
      QObject(parent)
{
    this->d = new ColorFilterPrivate(this);
    this->registerProperty("color", &this->d->m_color);
    this->registerProperty("radius", &this->d->m_radius);
    this->registerProperty("soft", &this->d->m_soft);
    this->registerProperty("disable", &this->d->m_disable);
}

ColorFilter::~ColorFilter()
{
    delete this->d;
}

QString ColorFilter::description() const
{
    return this->d->m_description;
}

AkElementType ColorFilter::type() const
{
    return this->d->m_type;
}

AkElementCategory ColorFilter::category() const
{
    return this->d->m_category;
}

void *ColorFilter::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *ColorFilter::create(const QString &id)
{
    Q_UNUSED(id)

    return new ColorFilter;
}

int ColorFilter::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/ColorFilter",
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

void ColorFilter::deleteThis(void *userData) const
{
    delete reinterpret_cast<ColorFilter *>(userData);
}

QString ColorFilter::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/ColorFilter/share/qml/main.qml");
}

void ColorFilter::controlInterfaceConfigure(QQmlContext *context,
                                                   const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("ColorFilter", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket ColorFilter::iVideoStream(const AkVideoPacket &packet)
{
    if (this->d->m_disable) {
        if (packet)
            this->oStream(packet);

        return packet;
    }

    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    int rf = qRed(this->d->m_color);
    int gf = qGreen(this->d->m_color);
    int bf = qBlue(this->d->m_color);

    auto radius = int(this->d->m_radius);
    auto radius2 = radius * radius;

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto pixel = srcLine[x];
            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);

            int rd = r - rf;
            int gd = g - gf;
            int bd = b - bf;

            auto k = rd * rd + gd * gd + bd * bd;

            if (k <= radius2) {
                if (this->d->m_soft) {
                    qreal p = qSqrt(k) / radius;

                    int gray = qGray(pixel);

                    r = int(p * (gray - r) + r);
                    g = int(p * (gray - g) + g);
                    b = int(p * (gray - b) + b);

                    dstLine[x] = qRgba(r, g, b, qAlpha(pixel));
                } else {
                    dstLine[x] = pixel;
                }
            } else {
                int gray = qGray(pixel);
                dstLine[x] = qRgba(gray, gray, gray, qAlpha(pixel));
            }
        }
    }

    if (dst)
        this->oStream(dst);

    return dst;
}

ColorFilterPrivate::ColorFilterPrivate(ColorFilter *self):
      self(self)
{

}

#include "moc_colorfilter.cpp"
