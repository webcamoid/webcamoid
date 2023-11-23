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
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "pixelate.h"

class PixelatePrivate
{
    public:
        Pixelate *self {nullptr};
        QString m_description {QObject::tr("Pixelate")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        QSize m_blockSize {8, 8};
        QMutex m_mutex;
        AkVideoConverter m_videoConverter;

        explicit PixelatePrivate(Pixelate *self);
};

Pixelate::Pixelate(QObject *parent):
    QObject(parent)
{
    this->d = new PixelatePrivate(this);
}

Pixelate::~Pixelate()
{
    delete this->d;
}

QString Pixelate::description() const
{
    return this->d->m_description;
}

AkElementType Pixelate::type() const
{
    return this->d->m_type;
}

AkElementCategory Pixelate::category() const
{
    return this->d->m_category;
}

void *Pixelate::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Pixelate::create(const QString &id)
{
    Q_UNUSED(id)

    return new Pixelate;
}

int Pixelate::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Pixelate",
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

QSize Pixelate::blockSize() const
{
    return this->d->m_blockSize;
}

void Pixelate::deleteThis(void *userData) const
{
    delete reinterpret_cast<Pixelate *>(userData);
}

QString Pixelate::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Pixelate/share/qml/main.qml");
}

void Pixelate::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Pixelate", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Pixelate::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_mutex.lock();
    auto blockSize = this->d->m_blockSize;
    this->d->m_mutex.unlock();

    if (blockSize.isEmpty()) {
        if (packet)
            this->oStream(packet);

        return packet;
    }

    this->d->m_videoConverter.begin();
    auto dcaps = packet.caps();
    dcaps.setWidth(packet.caps().width() / blockSize.width());
    dcaps.setHeight(packet.caps().height() / blockSize.height());
    this->d->m_videoConverter.setOutputCaps(dcaps);
    auto downScaled = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.setOutputCaps(packet.caps());
    auto dst = this->d->m_videoConverter.convert(downScaled);
    this->d->m_videoConverter.end();

    if (dst)
        this->oStream(dst);

    return dst;
}

void Pixelate::setBlockSize(const QSize &blockSize)
{
    if (blockSize == this->d->m_blockSize)
        return;

    this->d->m_mutex.lock();
    this->d->m_blockSize = blockSize;
    this->d->m_mutex.unlock();
    emit this->blockSizeChanged(blockSize);
}

void Pixelate::resetBlockSize()
{
    this->setBlockSize({8, 8});
}

PixelatePrivate::PixelatePrivate(Pixelate *self):
    self(self)
{

}

#include "moc_pixelate.cpp"
