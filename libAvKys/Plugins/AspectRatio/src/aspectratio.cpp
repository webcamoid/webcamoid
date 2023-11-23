/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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

#include <QImage>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "aspectratio.h"

class AspectRatioPrivate
{
    public:
        AspectRatio *self {nullptr};
        QString m_description {QObject::tr("Aspect Ratio")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        IAkPropertyInt m_width {QObject::tr("Width"), 16};
        IAkPropertyInt m_height {QObject::tr("Height"), 9};
        AkVideoConverter m_videoConverter;

        explicit AspectRatioPrivate(AspectRatio *self);
};

AspectRatio::AspectRatio(QObject *parent):
    QObject(parent)
{
    this->d = new AspectRatioPrivate(this);
    this->registerProperty("width", &this->d->m_width);
    this->registerProperty("height", &this->d->m_height);
    this->d->m_videoConverter.setAspectRatioMode(AkVideoConverter::AspectRatioMode_Expanding);
}

AspectRatio::~AspectRatio()
{
    delete this->d;
}

QString AspectRatio::description() const
{
    return this->d->m_description;
}

AkElementType AspectRatio::type() const
{
    return this->d->m_type;
}

AkElementCategory AspectRatio::category() const
{
    return this->d->m_category;
}

void *AspectRatio::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *AspectRatio::create(const QString &id)
{
    Q_UNUSED(id)

    return new AspectRatio;
}

int AspectRatio::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/AspectRatio",
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

void AspectRatio::deleteThis(void *userData) const
{
    delete reinterpret_cast<AspectRatio *>(userData);
}

QString AspectRatio::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/AspectRatio/share/qml/main.qml");
}

void AspectRatio::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("AspectRatio", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket AspectRatio::iVideoStream(const AkVideoPacket &packet)
{
    if (!packet)
        return {};

    int width = this->d->m_width;
    int height = this->d->m_height;
    int oWidth = qRound(qreal(packet.caps().height())
                        * qMax(width, 1)
                        / qMax(height, 1));
    oWidth = qMin(oWidth, packet.caps().width());
    int oHeight = qRound(qreal(packet.caps().width())
                         * qMax(height, 1)
                         / qMax(width, 1));
    oHeight = qMin(oHeight, packet.caps().height());

    AkVideoCaps oCaps(packet.caps());
    oCaps.setWidth(oWidth);
    oCaps.setHeight(oHeight);
    this->d->m_videoConverter.setOutputCaps(oCaps);

    this->d->m_videoConverter.begin();
    auto dst = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (dst)
        this->oStream(dst);

    return dst;
}

AspectRatioPrivate::AspectRatioPrivate(AspectRatio *self):
      self(self)
{

}

#include "moc_aspectratio.cpp"
