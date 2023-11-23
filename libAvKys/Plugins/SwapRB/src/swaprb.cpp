/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

#include <qrgb.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "swaprb.h"

class SwapRBPrivate
{
    public:
    SwapRB *self {nullptr};
    QString m_description {QObject::tr("Swap red and blue")};
    AkElementType m_type {AkElementType_VideoFilter};
    AkElementCategory m_category {AkElementCategory_VideoFilter};
    AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

    explicit SwapRBPrivate(SwapRB *self);
};

SwapRB::SwapRB(QObject *parent):
      QObject(parent)
{
    this->d = new SwapRBPrivate(this);
}

SwapRB::~SwapRB()
{
    delete this->d;
}

QString SwapRB::description() const
{
    return this->d->m_description;
}

AkElementType SwapRB::type() const
{
    return this->d->m_type;
}

AkElementCategory SwapRB::category() const
{
    return this->d->m_category;
}

void *SwapRB::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *SwapRB::create(const QString &id)
{
    Q_UNUSED(id)

    return new SwapRB;
}

int SwapRB::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/SwapRB",
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

void SwapRB::deleteThis(void *userData) const
{
    delete reinterpret_cast<SwapRB *>(userData);
}

AkPacket SwapRB::iVideoStream(const AkVideoPacket &packet)
{
    if (!packet)
        return {};

    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    for (int y = 0; y < src.caps().height(); ++y) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); ++x)
            dstLine[x] = qRgba(qBlue(srcLine[x]),
                               qGreen(srcLine[x]),
                               qRed(srcLine[x]),
                               qAlpha(srcLine[x]));
    }

    if (dst)
        this->oStream(dst);

    return dst;
}

SwapRBPrivate::SwapRBPrivate(SwapRB *self):
    self(self)
{

}

#include "moc_swaprb.cpp"
