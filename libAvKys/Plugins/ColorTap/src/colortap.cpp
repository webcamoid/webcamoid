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

#include <QImage>
#include <QMutex>
#include <QStandardPaths>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "colortap.h"

class ColorTapPrivate;

class ColorTableCallbacks:
    public IAkObjectPropertyCallbacks<QString>
{
    public:
        ColorTableCallbacks(ColorTapPrivate *self);
        void valueChanged(const QString &tableName) override;

    private:
        ColorTapPrivate *self;
};

class ColorTapPrivate
{
    public:
        ColorTap *self {nullptr};
        QString m_description {QObject::tr("Color from Palette")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        IAkPropertyString m_tableName {QObject::tr("Color table"), ":/ColorTap/share/tables/base.bmp"};
        ColorTableCallbacks *m_colorTableCallbacks {nullptr};
        QImage m_table;
        QMutex m_mutex;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        explicit ColorTapPrivate(ColorTap *self);
        ~ColorTapPrivate();
};

ColorTap::ColorTap(QObject *parent):
      QObject(parent)
{
    this->d = new ColorTapPrivate(this);
    this->registerProperty("table", &this->d->m_tableName);
    this->d->m_tableName.subscribe(this->d->m_colorTableCallbacks);

    QImage table(this->d->m_tableName.value());
    table = table.scaled(16, 16);
    this->d->m_table = table;
}

ColorTap::~ColorTap()
{
    delete this->d;
}

QString ColorTap::description() const
{
    return this->d->m_description;
}

AkElementType ColorTap::type() const
{
    return this->d->m_type;
}

AkElementCategory ColorTap::category() const
{
    return this->d->m_category;
}

void *ColorTap::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *ColorTap::create(const QString &id)
{
    Q_UNUSED(id)

    return new ColorTap;
}

int ColorTap::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/ColorTap",
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

void ColorTap::deleteThis(void *userData) const
{
    delete reinterpret_cast<ColorTap *>(userData);
}

QString ColorTap::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/ColorTap/share/qml/main.qml");
}

void ColorTap::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("ColorTap", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());

    QStringList picturesPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    context->setContextProperty("picturesPath", picturesPath[0]);
}

AkPacket ColorTap::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_mutex.lock();

    if (this->d->m_table.isNull()) {
        this->d->m_mutex.unlock();
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
    auto tableBits = reinterpret_cast<const QRgb *>(this->d->m_table.constBits());

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto &ipixel = srcLine[x];
            int r = qRed(ipixel);
            int g = qGreen(ipixel);
            int b = qBlue(ipixel);
            int a = qAlpha(ipixel);

            int ro = qRed(tableBits[r]);
            int go = qGreen(tableBits[g]);
            int bo = qBlue(tableBits[b]);

            dstLine[x] = qRgba(ro, go, bo, a);
        }
    }

    this->d->m_mutex.unlock();

    if (dst)
        this->oStream(dst);

    return dst;
}

ColorTapPrivate::ColorTapPrivate(ColorTap *self):
      self(self)
{
    this->m_colorTableCallbacks = new ColorTableCallbacks(this);
}

ColorTapPrivate::~ColorTapPrivate()
{
    delete this->m_colorTableCallbacks;
}

ColorTableCallbacks::ColorTableCallbacks(ColorTapPrivate *self):
    self(self)
{

}

void ColorTableCallbacks::valueChanged(const QString &tableName)
{
    QImage tableImg;

    if (!tableName.isEmpty()) {
        tableImg = QImage(tableName);

        if (!tableImg.isNull())
            tableImg = tableImg.scaled(16, 16);
    }

    self->m_mutex.lock();
    self->m_table = tableImg;
    self->m_mutex.unlock();
}

#include "moc_colortap.cpp"
