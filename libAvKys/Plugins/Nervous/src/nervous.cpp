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

#include <QDateTime>
#include <QRandomGenerator>
#include <QSize>
#include <QVector>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "nervous.h"

class NervousPrivate
{
    public:
        Nervous *self {nullptr};
        QString m_description {QObject::tr("Nervous")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        QVector<AkVideoPacket> m_frames;
        QSize m_frameSize;
        int m_nFrames {32};
        int m_stride {0};
        bool m_simple {false};

        explicit NervousPrivate(Nervous *self);
};

Nervous::Nervous(QObject *parent):
    QObject(parent)
{
    this->d = new NervousPrivate(this);
}

Nervous::~Nervous()
{
    delete this->d;
}

QString Nervous::description() const
{
    return this->d->m_description;
}

AkElementType Nervous::type() const
{
    return this->d->m_type;
}

AkElementCategory Nervous::category() const
{
    return this->d->m_category;
}

void *Nervous::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Nervous::create(const QString &id)
{
    Q_UNUSED(id)

    return new Nervous;
}

int Nervous::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Nervous",
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

int Nervous::nFrames() const
{
    return this->d->m_nFrames;
}

bool Nervous::simple() const
{
    return this->d->m_simple;
}

void Nervous::deleteThis(void *userData) const
{
    delete reinterpret_cast<Nervous *>(userData);
}

QString Nervous::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Nervous/share/qml/main.qml");
}

void Nervous::controlInterfaceConfigure(QQmlContext *context,
                                               const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Nervous", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Nervous::iVideoStream(const AkVideoPacket &packet)
{
    if (!packet)
        return {};

    QSize frameSize(packet.caps().width(), packet.caps().height());

    if (frameSize != this->d->m_frameSize) {
        this->d->m_frames.clear();
        this->d->m_stride = 0;
        this->d->m_frameSize = frameSize;
    }

    this->d->m_frames << packet;
    int diff = this->d->m_frames.size() - this->d->m_nFrames;

    for (int i = 0; i < diff && !this->d->m_frames.isEmpty(); i++)
        this->d->m_frames.removeFirst();

    if (this->d->m_frames.isEmpty()) {
        this->oStream(packet);

        return packet;
    }

    int nFrame = 0;

    if (!this->d->m_simple) {
        static int timer = 0;

        if (timer) {
            nFrame += this->d->m_stride;
            nFrame = qBound(0, nFrame, this->d->m_frames.size() - 1);
            timer--;
        } else {
            nFrame = QRandomGenerator::global()->bounded(this->d->m_frames.size());
            this->d->m_stride = QRandomGenerator::global()->bounded(2, 6);

            if (this->d->m_stride >= 0)
                this->d->m_stride++;

            timer = QRandomGenerator::global()->bounded(2, 8);
        }
    } else if(!this->d->m_frames.isEmpty()) {
        nFrame = QRandomGenerator::global()->bounded(this->d->m_frames.size());
    }

    auto dst = this->d->m_frames[nFrame];
    dst.copyMetadata(packet);

    if (dst)
        this->oStream(dst);

    return dst;
}

void Nervous::setNFrames(int nFrames)
{
    if (this->d->m_nFrames == nFrames)
        return;

    this->d->m_nFrames = nFrames;
    emit this->nFramesChanged(nFrames);
}

void Nervous::setSimple(bool simple)
{
    if (this->d->m_simple == simple)
        return;

    this->d->m_simple = simple;
    emit this->simpleChanged(simple);
}

void Nervous::resetNFrames()
{
    this->setNFrames(32);
}

void Nervous::resetSimple()
{
    this->setSimple(false);
}

NervousPrivate::NervousPrivate(Nervous *self):
    self(self)
{

}

#include "moc_nervous.cpp"
