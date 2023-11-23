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

#include "grayscale.h"

class GrayScalePrivate
{
    public:
        GrayScale *self {nullptr};
        QString m_description {QObject::tr("Gray")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_ya88pack, 0, 0, {}}};

        explicit GrayScalePrivate(GrayScale *self);
};

GrayScale::GrayScale(QObject *parent):
      QObject(parent)
{
    this->d = new GrayScalePrivate(this);
}

GrayScale::~GrayScale()
{
    delete this->d;
}

QString GrayScale::description() const
{
    return this->d->m_description;
}

AkElementType GrayScale::type() const
{
    return this->d->m_type;
}

AkElementCategory GrayScale::category() const
{
    return this->d->m_category;
}

void *GrayScale::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *GrayScale::create(const QString &id)
{
    Q_UNUSED(id)

    return new GrayScale;
}

int GrayScale::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/GrayScale",
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

void GrayScale::deleteThis(void *userData) const
{
    delete reinterpret_cast<GrayScale *>(userData);
}

AkPacket GrayScale::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto dst = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!dst)
        return {};

    this->oStream(dst);

    return dst;
}

GrayScalePrivate::GrayScalePrivate(GrayScale *self):
      self(self)
{

}

#include "moc_grayscale.cpp"
