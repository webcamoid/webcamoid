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

#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "gamma.h"

class GammaPrivate
{
    public:
    Gamma *self {nullptr};
    QString m_description {QObject::tr("Gamma adjust")};
    AkElementType m_type {AkElementType_VideoFilter};
    AkElementCategory m_category {AkElementCategory_VideoFilter};
    int m_gamma {0};
    quint8 *m_gammaTable {nullptr};
    AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

    explicit GammaPrivate(Gamma *self);
    inline void initGammaTable();
};

Gamma::Gamma(QObject *parent):
      QObject(parent)
{
    this->d = new GammaPrivate(this);
    this->d->initGammaTable();
}

Gamma::~Gamma()
{
    if (this->d->m_gammaTable)
        delete [] this->d->m_gammaTable;

    delete this->d;
}

QString Gamma::description() const
{
    return this->d->m_description;
}

AkElementType Gamma::type() const
{
    return this->d->m_type;
}

AkElementCategory Gamma::category() const
{
    return this->d->m_category;
}

void *Gamma::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Gamma::create(const QString &id)
{
    Q_UNUSED(id)

    return new Gamma;
}

int Gamma::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Gamma",
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

int Gamma::gamma() const
{
    return this->d->m_gamma;
}

void Gamma::deleteThis(void *userData) const
{
    delete reinterpret_cast<Gamma *>(userData);
}

QString Gamma::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Gamma/share/qml/main.qml");
}

void Gamma::controlInterfaceConfigure(QQmlContext *context,
                                             const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Gamma", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Gamma::iVideoStream(const AkVideoPacket &packet)
{
    if (this->d->m_gamma == 0) {
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

    auto gamma = qBound(-255, this->d->m_gamma, 255);
    size_t gammaOffset = size_t(gamma + 255) << 8;

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto &pixel = srcLine[x];
            auto &r = this->d->m_gammaTable[gammaOffset | qRed(pixel)];
            auto &g = this->d->m_gammaTable[gammaOffset | qGreen(pixel)];
            auto &b = this->d->m_gammaTable[gammaOffset | qBlue(pixel)];
            dstLine[x] = qRgba(r, g, b, qAlpha(pixel));
        }
    }

    if (dst)
        this->oStream(dst);

    return dst;
}

void Gamma::setGamma(int gamma)
{
    if (this->d->m_gamma == gamma)
        return;

    this->d->m_gamma = gamma;
    emit this->gammaChanged(gamma);
}

void Gamma::resetGamma()
{
    this->setGamma(0);
}

GammaPrivate::GammaPrivate(Gamma *self):
    self(self)
{

}

void GammaPrivate::initGammaTable()
{
    static const int grayLevels = 256;
    static const int minGamma = -254;
    static const int maxGamma = 256;
    this->m_gammaTable = new quint8 [grayLevels * (maxGamma - minGamma + 1)];
    size_t j = 0;

    for (int i = 0; i < grayLevels; i++) {
        auto ig = uint8_t(255. * qPow(i / 255., 255));
        this->m_gammaTable[j] = ig;
        j++;
    }

    for (int gamma = minGamma; gamma < maxGamma; gamma++) {
        double k = 255. / (gamma + 255);

        for (int i = 0; i < grayLevels; i++) {
            auto ig = uint8_t(255. * qPow(i / 255., k));
            this->m_gammaTable[j] = ig;
            j++;
        }
    }
}

#include "moc_gamma.cpp"
