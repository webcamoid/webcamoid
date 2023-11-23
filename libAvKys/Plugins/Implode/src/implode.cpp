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

#include "implode.h"

class ImplodePrivate
{
    public:
        Implode *self {nullptr};
        QString m_description {QObject::tr("Implode")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        qreal m_amount {1.0};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        explicit ImplodePrivate(Implode *self);
};

Implode::Implode(QObject *parent):
    QObject(parent)
{
    this->d = new ImplodePrivate(this);
}

Implode::~Implode()
{
    delete this->d;
}

QString Implode::description() const
{
    return this->d->m_description;
}

AkElementType Implode::type() const
{
    return this->d->m_type;
}

AkElementCategory Implode::category() const
{
    return this->d->m_category;
}

void *Implode::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Implode::create(const QString &id)
{
    Q_UNUSED(id)

    return new Implode;
}

int Implode::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Implode",
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

qreal Implode::amount() const
{
    return this->d->m_amount;
}

void Implode::deleteThis(void *userData) const
{
    delete reinterpret_cast<Implode *>(userData);
}

QString Implode::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Implode/share/qml/main.qml");
}

void Implode::controlInterfaceConfigure(QQmlContext *context,
                                               const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Implode", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Implode::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    int xc = src.caps().width() >> 1;
    int yc = src.caps().height() >> 1;
    int radius = qMin(xc, yc);
    auto amount = this->d->m_amount;

    for (int y = 0; y < src.caps().height(); y++) {
        auto iLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));
        int yDiff = y - yc;
        int yDiff2 = yDiff * yDiff;

        for (int x = 0; x < src.caps().width(); x++) {
            int xDiff = x - xc;
            qreal distance = sqrt(xDiff * xDiff + yDiff2);

            if (distance >= radius) {
                oLine[x] = iLine[x];
            } else {
                qreal factor = pow(distance / radius, amount);

                int xp = int(factor * xDiff + xc);
                int yp = int(factor * yDiff + yc);

                xp = qBound(0, xp, dst.caps().width() - 1);
                yp = qBound(0, yp, dst.caps().height() - 1);

                auto line = reinterpret_cast<const QRgb *>(src.constLine(0, yp));
                oLine[x] = line[xp];
            }
        }
    }

    if (dst)
        this->oStream(dst);

    return dst;
}

void Implode::setAmount(qreal amount)
{
    if (qFuzzyCompare(this->d->m_amount, amount))
        return;

    this->d->m_amount = amount;
    emit this->amountChanged(amount);
}

void Implode::resetAmount()
{
    this->setAmount(1.0);
}

ImplodePrivate::ImplodePrivate(Implode *self):
    self(self)
{

}

#include "moc_implode.cpp"
