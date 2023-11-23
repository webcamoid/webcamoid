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

#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "flip.h"

class FlipPrivate
{
    public:
        Flip *self {nullptr};
        QString m_description {QObject::tr("Flip")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        IAkPropertyBool m_horizontalFlip {QObject::tr("Horizontal flip")};
        IAkPropertyBool m_verticalFlip {QObject::tr("Vertical flip")};

        explicit FlipPrivate(Flip *self);
        inline void copy(quint8 *dst,
                         const quint8 *src,
                         size_t bytes) const
        {
            for (size_t i = 0; i < bytes; ++i)
                dst[i] = src[i];
        }
};

Flip::Flip(QObject *parent):
      QObject(parent)
{
    this->d = new FlipPrivate(this);
    this->registerProperty("horizontalFlip", &this->d->m_horizontalFlip);
    this->registerProperty("verticalFlip", &this->d->m_verticalFlip);
}

Flip::~Flip()
{
    delete this->d;
}

QString Flip::description() const
{
    return this->d->m_description;
}

AkElementType Flip::type() const
{
    return this->d->m_type;
}

AkElementCategory Flip::category() const
{
    return this->d->m_category;
}

void *Flip::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Flip::create(const QString &id)
{
    Q_UNUSED(id)

    return new Flip;
}

int Flip::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Flip",
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

void Flip::deleteThis(void *userData) const
{
    delete reinterpret_cast<Flip *>(userData);
}

QString Flip::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Flip/share/qml/main.qml");
}

void Flip::controlInterfaceConfigure(QQmlContext *context,
                                     const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Flip", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Flip::iVideoStream(const AkVideoPacket &packet)
{
    if (!packet || (!this->d->m_horizontalFlip && !this->d->m_verticalFlip)) {
        if (packet)
            this->oStream(packet);

        return packet;
    }

    AkVideoPacket oPacket(packet.caps());
    oPacket.copyMetadata(packet);

    if (this->d->m_horizontalFlip && this->d->m_verticalFlip) {
        auto specs = AkVideoCaps::formatSpecs(packet.caps().format());

        for (int plane = 0; plane < packet.planes(); ++plane) {
            auto pixelSize = specs.plane(plane).pixelSize();
            auto width = packet.caps().width() >> oPacket.widthDiv(plane);

            for (int ys = 0, yd = packet.caps().height() - 1;
                 ys < packet.caps().height();
                 ++ys, --yd) {
                auto srcLine = packet.constLine(plane, ys);
                auto dstLine = oPacket.line(plane, yd);

                for (int xs = 0, xd = width - 1; xs < width; ++xs, --xd)
                    this->d->copy(dstLine + pixelSize * xd,
                                  srcLine + pixelSize * xs,
                                  pixelSize);
            }
        }
    } else if (this->d->m_horizontalFlip) {
        auto specs = AkVideoCaps::formatSpecs(packet.caps().format());

        for (int plane = 0; plane < packet.planes(); ++plane) {
            auto pixelSize = specs.plane(plane).pixelSize();
            auto width = packet.caps().width() >> oPacket.widthDiv(plane);

            for (int y = 0; y < packet.caps().height(); ++y) {
                auto srcLine = packet.constLine(plane, y);
                auto dstLine = oPacket.line(plane, y);

                for (int xs = 0, xd = width - 1; xs < width; ++xs, --xd)
                    this->d->copy(dstLine + pixelSize * xd,
                                  srcLine + pixelSize * xs,
                                  pixelSize);
            }
        }
    } else if (this->d->m_verticalFlip) {
        for (int plane = 0; plane < packet.planes(); ++plane) {
            auto iLineSize = packet.lineSize(plane);
            auto oLineSize = oPacket.lineSize(plane);
            auto lineSize = qMin(iLineSize, oLineSize);

            for (int ys = 0, yd = packet.caps().height() - 1;
                 ys < packet.caps().height();
                 ++ys, --yd) {
                auto srcLine = packet.constLine(plane, ys);
                auto dstLine = oPacket.line(plane, yd);
                memcpy(dstLine, srcLine, lineSize);
            }
        }
    }

    if (oPacket)
        this->oStream(oPacket);

    return oPacket;
}

FlipPrivate::FlipPrivate(Flip *self):
      self(self)
{

}

#include "moc_flip.cpp"
