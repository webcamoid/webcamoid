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

#include <QSharedPointer>
#include <QMutex>
#include <QQmlContext>
#include <akcaps.h>

#include "multisrcelement.h"
#include "multisrcelementsettings.h"
#include "mediasource.h"

template<typename T>
inline QSharedPointer<T> ptr_cast(QObject *obj=nullptr)
{
    return QSharedPointer<T>(static_cast<T *>(obj));
}

using MediaSourcePtr = QSharedPointer<MediaSource>;

class MultiSrcElementPrivate
{
    public:
        MultiSrcElement *self;
        MultiSrcElementSettings m_settings;
        MediaSourcePtr m_mediaSource;
        QMutex m_mutexLib;

        explicit MultiSrcElementPrivate(MultiSrcElement *self);
        void codecLibUpdated(const QString &codecLib);
};

MultiSrcElement::MultiSrcElement():
    AkMultimediaSourceElement()
{
    this->d = new MultiSrcElementPrivate(this);
    QObject::connect(&this->d->m_settings,
                     &MultiSrcElementSettings::codecLibChanged,
                     [this] (const QString &codecLib) {
                        this->d->codecLibUpdated(codecLib);
                     });

    this->d->codecLibUpdated(this->d->m_settings.codecLib());
}

MultiSrcElement::~MultiSrcElement()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QStringList MultiSrcElement::medias()
{
    if (!this->d->m_mediaSource)
        return {};

    return this->d->m_mediaSource->medias();
}

QString MultiSrcElement::media() const
{
    if (!this->d->m_mediaSource)
        return {};

    return this->d->m_mediaSource->media();
}

QList<int> MultiSrcElement::streams()
{
    if (!this->d->m_mediaSource)
        return {};

    return this->d->m_mediaSource->streams();
}

bool MultiSrcElement::loop() const
{
    if (!this->d->m_mediaSource)
        return false;

    return this->d->m_mediaSource->loop();
}

bool MultiSrcElement::sync() const
{
    if (!this->d->m_mediaSource)
        return false;

    return this->d->m_mediaSource->sync();
}

QList<int> MultiSrcElement::listTracks(const QString &type)
{
    if (!this->d->m_mediaSource)
        return {};

    return this->d->m_mediaSource->listTracks(type);
}

QString MultiSrcElement::streamLanguage(int stream)
{
    if (!this->d->m_mediaSource)
        return {};

    return this->d->m_mediaSource->streamLanguage(stream);
}

int MultiSrcElement::defaultStream(const QString &mimeType)
{
    if (!this->d->m_mediaSource)
        return -1;

    return this->d->m_mediaSource->defaultStream(mimeType);
}

QString MultiSrcElement::description(const QString &media)
{
    if (!this->d->m_mediaSource)
        return {};

    return this->d->m_mediaSource->description(media);
}

AkCaps MultiSrcElement::caps(int stream)
{
    if (!this->d->m_mediaSource)
        return {};

    return this->d->m_mediaSource->caps(stream);
}

qint64 MultiSrcElement::durationMSecs()
{
    if (!this->d->m_mediaSource)
        return {};

    return this->d->m_mediaSource->durationMSecs();
}

qint64 MultiSrcElement::currentTimeMSecs()
{
    if (!this->d->m_mediaSource)
        return {};

    return this->d->m_mediaSource->currentTimeMSecs();
}

qint64 MultiSrcElement::maxPacketQueueSize() const
{
    if (!this->d->m_mediaSource)
        return 0;

    return this->d->m_mediaSource->maxPacketQueueSize();
}

bool MultiSrcElement::showLog() const
{
    if (!this->d->m_mediaSource)
        return false;

    return this->d->m_mediaSource->showLog();
}

AkElement::ElementState MultiSrcElement::state() const
{
    if (!this->d->m_mediaSource)
        return ElementStateNull;

    return this->d->m_mediaSource->state();
}

QString MultiSrcElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/MultiSrc/share/qml/main.qml");
}

void MultiSrcElement::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("MultiSrc", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void MultiSrcElement::seek(qint64 seekTo, SeekPosition position)
{
    if (this->d->m_mediaSource)
        this->d->m_mediaSource->seek(seekTo, position);
}

void MultiSrcElement::setMedia(const QString &media)
{
    if (this->d->m_mediaSource)
        this->d->m_mediaSource->setMedia(media);
}

void MultiSrcElement::setStreams(const QList<int> &streams)
{
    if (this->d->m_mediaSource)
        this->d->m_mediaSource->setStreams(streams);
}

void MultiSrcElement::setLoop(bool loop)
{
    if (this->d->m_mediaSource)
        this->d->m_mediaSource->setLoop(loop);
}

void MultiSrcElement::setSync(bool sync)
{
    if (this->d->m_mediaSource)
        this->d->m_mediaSource->setSync(sync);
}

void MultiSrcElement::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    if (this->d->m_mediaSource)
        this->d->m_mediaSource->setMaxPacketQueueSize(maxPacketQueueSize);
}

void MultiSrcElement::setShowLog(bool showLog)
{
    if (this->d->m_mediaSource)
        this->d->m_mediaSource->setShowLog(showLog);
}

void MultiSrcElement::resetMedia()
{
    if (this->d->m_mediaSource)
        this->d->m_mediaSource->resetMedia();
}

void MultiSrcElement::resetStreams()
{
    if (this->d->m_mediaSource)
        this->d->m_mediaSource->resetStreams();
}

void MultiSrcElement::resetLoop()
{
    if (this->d->m_mediaSource)
        this->d->m_mediaSource->resetLoop();
}

void MultiSrcElement::resetSync()
{
    if (this->d->m_mediaSource)
        this->d->m_mediaSource->resetSync();
}

void MultiSrcElement::resetMaxPacketQueueSize()
{
    if (this->d->m_mediaSource)
        this->d->m_mediaSource->resetMaxPacketQueueSize();
}

void MultiSrcElement::resetShowLog()
{
    if (this->d->m_mediaSource)
        this->d->m_mediaSource->resetShowLog();
}

bool MultiSrcElement::setState(AkElement::ElementState state)
{
    if (!this->d->m_mediaSource)
        return false;

    return this->d->m_mediaSource->setState(state);
}

MultiSrcElementPrivate::MultiSrcElementPrivate(MultiSrcElement *self):
    self(self)
{

}

void MultiSrcElementPrivate::codecLibUpdated(const QString &codecLib)
{
    auto state = self->state();
    self->setState(AkElement::ElementStateNull);

    QString media;
    bool loop = false;
    bool showLog = false;

    if (this->m_mediaSource) {
        media = this->m_mediaSource->media();
        loop = this->m_mediaSource->loop();
        showLog = this->m_mediaSource->showLog();
    }

    this->m_mutexLib.lock();

    this->m_mediaSource =
            ptr_cast<MediaSource>(MultiSrcElement::loadSubModule("MultiSrc",
                                                                 codecLib));

    if (!this->m_mediaSource) {
        this->m_mutexLib.unlock();

        return;
    }

    QObject::connect(this->m_mediaSource.data(),
                     SIGNAL(stateChanged(AkElement::ElementState)),
                     self,
                     SIGNAL(stateChanged(AkElement::ElementState)));
    QObject::connect(this->m_mediaSource.data(),
                     SIGNAL(oStream(const AkPacket &)),
                     self,
                     SIGNAL(oStream(const AkPacket &)),
                     Qt::DirectConnection);
    QObject::connect(this->m_mediaSource.data(),
                     SIGNAL(error(const QString &)),
                     self,
                     SIGNAL(error(const QString &)));
    QObject::connect(this->m_mediaSource.data(),
                     SIGNAL(durationMSecsChanged(qint64)),
                     self,
                     SIGNAL(durationMSecsChanged(qint64)));
    QObject::connect(this->m_mediaSource.data(),
                     SIGNAL(currentTimeMSecsChanged(qint64)),
                     self,
                     SIGNAL(currentTimeMSecsChanged(qint64)));
    QObject::connect(this->m_mediaSource.data(),
                     SIGNAL(maxPacketQueueSizeChanged(qint64)),
                     self,
                     SIGNAL(maxPacketQueueSizeChanged(qint64)));
    QObject::connect(this->m_mediaSource.data(),
                     SIGNAL(showLogChanged(bool)),
                     self,
                     SIGNAL(showLogChanged(bool)));
    QObject::connect(this->m_mediaSource.data(),
                     SIGNAL(loopChanged(bool)),
                     self,
                     SIGNAL(loopChanged(bool)));
    QObject::connect(this->m_mediaSource.data(),
                     SIGNAL(syncChanged(bool)),
                     self,
                     SIGNAL(syncChanged(bool)));
    QObject::connect(this->m_mediaSource.data(),
                     SIGNAL(mediasChanged(const QStringList &)),
                     self,
                     SIGNAL(mediasChanged(const QStringList &)));
    QObject::connect(this->m_mediaSource.data(),
                     SIGNAL(mediaChanged(const QString &)),
                     self,
                     SIGNAL(mediaChanged(const QString &)));
    QObject::connect(this->m_mediaSource.data(),
                     SIGNAL(streamsChanged(const QList<int> &)),
                     self,
                     SIGNAL(streamsChanged(const QList<int> &)));

    this->m_mutexLib.unlock();

    this->m_mediaSource->setMedia(media);
    this->m_mediaSource->setLoop(loop);
    this->m_mediaSource->setShowLog(showLog);

    emit self->streamsChanged(self->streams());
    emit self->maxPacketQueueSizeChanged(self->maxPacketQueueSize());

    self->setState(state);
}

#include "moc_multisrcelement.cpp"
