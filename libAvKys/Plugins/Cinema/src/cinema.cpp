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

#include "cinema.h"

class CinemaPrivate
{
    public:
        Cinema *self {nullptr};
        QString m_description {QObject::tr("Cinema")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        IAkPropertyDouble m_stripSize {QObject::tr("Strip size"), 0.5};
        IAkPropertyColor m_stripColor {QObject::tr("Strip color"), qRgb(0, 0, 0)};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        qint64 *m_aiMultTable {nullptr};
        qint64 *m_aoMultTable {nullptr};
        qint64 *m_alphaDivTable {nullptr};

        explicit CinemaPrivate(Cinema *self);
};

Cinema::Cinema(QObject *parent):
      QObject(parent)
{
    this->d = new CinemaPrivate(this);

    this->registerProperty("stripSize", &this->d->m_stripSize);
    this->registerProperty("stripColor", &this->d->m_stripColor);

    constexpr qint64 maxAi = 255;
    constexpr qint64 maxAi2 = maxAi * maxAi;
    constexpr qint64 alphaMult = 1 << 16;
    this->d->m_aiMultTable = new qint64 [alphaMult];
    this->d->m_aoMultTable = new qint64 [alphaMult];
    this->d->m_alphaDivTable = new qint64 [alphaMult];

    for (qint64 ai = 0; ai < 256; ai++)
        for (qint64 ao = 0; ao < 256; ao++) {
            auto alphaMask = (ai << 8) | ao;
            auto a = maxAi2 - (maxAi - ai) * (maxAi - ao);
            this->d->m_aiMultTable[alphaMask] = a? alphaMult * ai * maxAi / a: 0;
            this->d->m_aoMultTable[alphaMask] = a? alphaMult * ao * (maxAi - ai) / a: 0;
            this->d->m_alphaDivTable[alphaMask] = a / maxAi;
        }
}

Cinema::~Cinema()
{
    delete [] this->d->m_aiMultTable;
    delete [] this->d->m_aoMultTable;
    delete [] this->d->m_alphaDivTable;

    delete this->d;
}

QString Cinema::description() const
{
    return this->d->m_description;
}

AkElementType Cinema::type() const
{
    return this->d->m_type;
}

AkElementCategory Cinema::category() const
{
    return this->d->m_category;
}

void *Cinema::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Cinema::create(const QString &id)
{
    Q_UNUSED(id)

    return new Cinema;
}

int Cinema::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Cinema",
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

void Cinema::deleteThis(void *userData) const
{
    delete reinterpret_cast<Cinema *>(userData);
}

QString Cinema::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Cinema/share/qml/main.qml");
}

void Cinema::controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Cinema", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Cinema::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);
    int cy = src.caps().height() >> 1;
    auto stripSize = int(cy * qreal(this->d->m_stripSize));
    qint64 ri = qRed(this->d->m_stripColor);
    qint64 gi = qGreen(this->d->m_stripColor);
    qint64 bi = qBlue(this->d->m_stripColor);
    qint64 ai = qAlpha(this->d->m_stripColor);
    auto iLineSize = src.lineSize(0);
    auto oLineSize = dst.lineSize(0);
    auto lineSize = qMin(iLineSize, oLineSize);

    for (int y = 0; y < src.caps().height(); y++) {
        int k = cy - qAbs(y - cy);
        auto iLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        if (k > stripSize)
            memcpy(oLine, iLine, lineSize);
        else
            for (int x = 0; x < src.caps().width(); x++) {
                auto &pixel = iLine[x];

                qint64 ro = qRed(pixel);
                qint64 go = qGreen(pixel);
                qint64 bo = qBlue(pixel);
                qint64 ao = qAlpha(pixel);

                auto alphaMask = (ai << 8) | ao;
                qint64 rt = (ri * this->d->m_aiMultTable[alphaMask] + ro * this->d->m_aoMultTable[alphaMask]) >> 16;
                qint64 gt = (gi * this->d->m_aiMultTable[alphaMask] + go * this->d->m_aoMultTable[alphaMask]) >> 16;
                qint64 bt = (bi * this->d->m_aiMultTable[alphaMask] + bo * this->d->m_aoMultTable[alphaMask]) >> 16;
                qint64 &at = this->d->m_alphaDivTable[alphaMask];

                oLine[x] = qRgba(int(rt), int(gt), int(bt), int(at));
            }
    }

    if (dst)
        this->oStream(dst);

    return dst;
}

CinemaPrivate::CinemaPrivate(Cinema *self):
    self(self)
{

}

#include "moc_cinema.cpp"
