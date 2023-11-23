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

#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "contrast.h"

class ContrastPrivate
{
    public:
        Contrast *self {nullptr};
        QString m_description {QObject::tr("Contrast")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        IAkPropertyInt m_contrast {QObject::tr("Contrast"), 0};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        explicit ContrastPrivate(Contrast *self);
        const QVector<quint8> &contrastTable() const;
        QVector<quint8> initContrastTable() const;
};

Contrast::Contrast(QObject *parent):
      QObject(parent)
{
    this->d = new ContrastPrivate(this);
    this->registerProperty("contrast", &this->d->m_contrast);
}

Contrast::~Contrast()
{
    delete this->d;
}

QString Contrast::description() const
{
    return this->d->m_description;
}

AkElementType Contrast::type() const
{
    return this->d->m_type;
}

AkElementCategory Contrast::category() const
{
    return this->d->m_category;
}

void *Contrast::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Contrast::create(const QString &id)
{
    Q_UNUSED(id)

    return new Contrast;
}

int Contrast::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Contrast",
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

void Contrast::deleteThis(void *userData) const
{
    delete reinterpret_cast<Contrast *>(userData);
}

QString Contrast::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Contrast/share/qml/main.qml");
}

void Contrast::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Contrast", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Contrast::iVideoStream(const AkVideoPacket &packet)
{
    if (this->d->m_contrast == 0) {
        if (packet)
            this->oStream(packet);

        return packet;
    }

    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src) {
        if (packet)
            this->oStream(packet);

        return packet;
    }

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);
    auto dataCt = this->d->contrastTable();
    auto contrast = qBound<int>(-255, this->d->m_contrast, 255);
    size_t contrastOffset = size_t(contrast + 255) << 8;

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto destLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto pixel = srcLine[x];
            auto r = dataCt[contrastOffset | qRed(pixel)];
            auto g = dataCt[contrastOffset | qGreen(pixel)];
            auto b = dataCt[contrastOffset | qBlue(pixel)];
            destLine[x] = qRgba(r, g, b, qAlpha(pixel));
        }
    }

    if (dst)
        this->oStream(dst);

    return dst;
}

ContrastPrivate::ContrastPrivate(Contrast *self):
      self(self)
{

}

const QVector<quint8> &ContrastPrivate::contrastTable() const
{
    static auto contrastTable = this->initContrastTable();

    return contrastTable;
}

QVector<quint8> ContrastPrivate::initContrastTable() const
{
    QVector<quint8> contrastTable;

    for (int contrast = -255; contrast < 256; contrast++) {
        double f = 259. * (255 + contrast) / (255. * (259 - contrast));

        for (int i = 0; i < 256; i++) {
            int ic = int(f * (i - 128) + 128.);
            contrastTable << quint8(qBound(0, ic, 255));
        }
    }

    return contrastTable;
}

#include "moc_contrast.cpp"
