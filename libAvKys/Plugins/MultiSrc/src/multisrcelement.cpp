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
#include <QReadWriteLock>
#include <akplugininfo.h>
#include <akpluginmanager.h>

#include "multisrcelement.h"
#include "mediasource.h"

using MediaSourcePtr = QSharedPointer<MediaSource>;

class MultiSrcElementPrivate
{
    public:
        MultiSrcElement *self;
        MediaSourcePtr m_mediaSource;
        QString m_mediaSourceImpl;
        QReadWriteLock m_mutex;

        explicit MultiSrcElementPrivate(MultiSrcElement *self);
        void linksChanged(const AkPluginLinks &links);
};

MultiSrcElement::MultiSrcElement():
    AkMultimediaSourceElement()
{
    this->d = new MultiSrcElementPrivate(this);
    QObject::connect(akPluginManager,
                     &AkPluginManager::linksChanged,
                     this,
                     [this] (const AkPluginLinks &links) {
                        this->d->linksChanged(links);
                     });

    if (this->d->m_mediaSource) {
        QObject::connect(this->d->m_mediaSource.data(),
                         &MediaSource::stateChanged,
                         this,
                         &MultiSrcElement::stateChanged);
        QObject::connect(this->d->m_mediaSource.data(),
                         &MediaSource::oStream,
                         this,
                         &MultiSrcElement::oStream,
                         Qt::DirectConnection);
        QObject::connect(this->d->m_mediaSource.data(),
                         &MediaSource::error,
                         this,
                         &MultiSrcElement::error);
        QObject::connect(this->d->m_mediaSource.data(),
                         &MediaSource::durationMSecsChanged,
                         this,
                         &MultiSrcElement::durationMSecsChanged);
        QObject::connect(this->d->m_mediaSource.data(),
                         &MediaSource::currentTimeMSecsChanged,
                         this,
                         &MultiSrcElement::currentTimeMSecsChanged);
        QObject::connect(this->d->m_mediaSource.data(),
                         &MediaSource::maxPacketQueueSizeChanged,
                         this,
                         &MultiSrcElement::maxPacketQueueSizeChanged);
        QObject::connect(this->d->m_mediaSource.data(),
                         &MediaSource::showLogChanged,
                         this,
                         &MultiSrcElement::showLogChanged);
        QObject::connect(this->d->m_mediaSource.data(),
                         &MediaSource::loopChanged,
                         this,
                         &MultiSrcElement::loopChanged);
        QObject::connect(this->d->m_mediaSource.data(),
                         &MediaSource::syncChanged,
                         this,
                         &MultiSrcElement::syncChanged);
        QObject::connect(this->d->m_mediaSource.data(),
                         &MediaSource::mediasChanged,
                         this,
                         &MultiSrcElement::mediasChanged);
        QObject::connect(this->d->m_mediaSource.data(),
                         &MediaSource::mediaChanged,
                         this,
                         &MultiSrcElement::mediaChanged);
        QObject::connect(this->d->m_mediaSource.data(),
                         &MediaSource::mediaLoaded,
                         this,
                         &MultiSrcElement::mediaLoaded);
        QObject::connect(this->d->m_mediaSource.data(),
                         &MediaSource::streamsChanged,
                         this,
                         &MultiSrcElement::streamsChanged);
    }
}

MultiSrcElement::~MultiSrcElement()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QStringList MultiSrcElement::medias()
{
    this->d->m_mutex.lockForRead();
    QStringList medias;

    if (this->d->m_mediaSource)
        medias = this->d->m_mediaSource->medias();

    this->d->m_mutex.unlock();

    return medias;
}

QString MultiSrcElement::media() const
{
    this->d->m_mutex.lockForRead();
    QString media;

    if (this->d->m_mediaSource)
        media = this->d->m_mediaSource->media();

    this->d->m_mutex.unlock();

    return media;
}

QList<int> MultiSrcElement::streams()
{
    this->d->m_mutex.lockForRead();
    QList<int> streams;

    if (this->d->m_mediaSource)
        streams = this->d->m_mediaSource->streams();

    this->d->m_mutex.unlock();

    return streams;
}

bool MultiSrcElement::loop() const
{
    this->d->m_mutex.lockForRead();
    bool loop = false;

    if (this->d->m_mediaSource)
        loop = this->d->m_mediaSource->loop();

    this->d->m_mutex.unlock();

    return loop;
}

bool MultiSrcElement::sync() const
{
    this->d->m_mutex.lockForRead();
    bool sync = false;

    if (this->d->m_mediaSource)
        sync = this->d->m_mediaSource->sync();

    this->d->m_mutex.unlock();

    return sync;
}

QList<int> MultiSrcElement::listTracks(AkCaps::CapsType type)
{
    this->d->m_mutex.lockForRead();
    QList<int> tracks;

    if (this->d->m_mediaSource)
        tracks = this->d->m_mediaSource->listTracks(type);

    this->d->m_mutex.unlock();

    return tracks;
}

QString MultiSrcElement::streamLanguage(int stream)
{
    this->d->m_mutex.lockForRead();
    QString language;

    if (this->d->m_mediaSource)
        language = this->d->m_mediaSource->streamLanguage(stream);

    this->d->m_mutex.unlock();

    return language;
}

int MultiSrcElement::defaultStream(AkCaps::CapsType type)
{
    this->d->m_mutex.lockForRead();
    int stream = 0;

    if (this->d->m_mediaSource)
        stream = this->d->m_mediaSource->defaultStream(type);

    this->d->m_mutex.unlock();

    return stream;
}

QString MultiSrcElement::description(const QString &media)
{
    this->d->m_mutex.lockForRead();
    QString description;

    if (this->d->m_mediaSource)
        description = this->d->m_mediaSource->description(media);

    this->d->m_mutex.unlock();

    return description;
}

AkCaps MultiSrcElement::caps(int stream)
{
    this->d->m_mutex.lockForRead();
    AkCaps caps;

    if (this->d->m_mediaSource)
        caps = this->d->m_mediaSource->caps(stream);

    this->d->m_mutex.unlock();

    return caps;
}

qint64 MultiSrcElement::durationMSecs()
{
    this->d->m_mutex.lockForRead();
    qint64 duration = 0;

    if (this->d->m_mediaSource)
        duration = this->d->m_mediaSource->durationMSecs();

    this->d->m_mutex.unlock();

    return duration;
}

qint64 MultiSrcElement::currentTimeMSecs()
{
    this->d->m_mutex.lockForRead();
    qint64 curTime = 0;

    if (this->d->m_mediaSource)
        curTime = this->d->m_mediaSource->currentTimeMSecs();

    this->d->m_mutex.unlock();

    return curTime;
}

qint64 MultiSrcElement::maxPacketQueueSize() const
{
    this->d->m_mutex.lockForRead();
    qint64 queueSize = 0;

    if (this->d->m_mediaSource)
        queueSize = this->d->m_mediaSource->maxPacketQueueSize();

    this->d->m_mutex.unlock();

    return queueSize;
}

bool MultiSrcElement::showLog() const
{
    this->d->m_mutex.lockForRead();
    bool showLog = false;

    if (this->d->m_mediaSource)
        showLog = this->d->m_mediaSource->showLog();

    this->d->m_mutex.unlock();

    return showLog;
}

AkElement::ElementState MultiSrcElement::state() const
{
    this->d->m_mutex.lockForRead();
    ElementState state = ElementStateNull;

    if (this->d->m_mediaSource)
        state = this->d->m_mediaSource->state();

    this->d->m_mutex.unlock();

    return state;
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
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaSource)
        this->d->m_mediaSource->seek(seekTo, MediaSource::SeekPosition(position));

    this->d->m_mutex.unlock();
}

void MultiSrcElement::setMedia(const QString &media)
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaSource)
        this->d->m_mediaSource->setMedia(media);

    this->d->m_mutex.unlock();
}

void MultiSrcElement::setStreams(const QList<int> &streams)
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaSource)
        this->d->m_mediaSource->setStreams(streams);

    this->d->m_mutex.unlock();
}

void MultiSrcElement::setLoop(bool loop)
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaSource)
        this->d->m_mediaSource->setLoop(loop);

    this->d->m_mutex.unlock();
}

void MultiSrcElement::setSync(bool sync)
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaSource)
        this->d->m_mediaSource->setSync(sync);

    this->d->m_mutex.unlock();
}

void MultiSrcElement::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaSource)
        this->d->m_mediaSource->setMaxPacketQueueSize(maxPacketQueueSize);

    this->d->m_mutex.unlock();
}

void MultiSrcElement::setShowLog(bool showLog)
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaSource)
        this->d->m_mediaSource->setShowLog(showLog);

    this->d->m_mutex.unlock();
}

void MultiSrcElement::resetMedia()
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaSource)
        this->d->m_mediaSource->resetMedia();

    this->d->m_mutex.unlock();
}

void MultiSrcElement::resetStreams()
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaSource)
        this->d->m_mediaSource->resetStreams();

    this->d->m_mutex.unlock();
}

void MultiSrcElement::resetLoop()
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaSource)
        this->d->m_mediaSource->resetLoop();

    this->d->m_mutex.unlock();
}

void MultiSrcElement::resetSync()
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaSource)
        this->d->m_mediaSource->resetSync();

    this->d->m_mutex.unlock();
}

void MultiSrcElement::resetMaxPacketQueueSize()
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaSource)
        this->d->m_mediaSource->resetMaxPacketQueueSize();

    this->d->m_mutex.unlock();
}

void MultiSrcElement::resetShowLog()
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaSource)
        this->d->m_mediaSource->resetShowLog();

    this->d->m_mutex.unlock();
}

bool MultiSrcElement::setState(ElementState state)
{
    this->d->m_mutex.lockForRead();
    bool result = false;

    if (this->d->m_mediaSource)
        result = this->d->m_mediaSource->setState(state);

    this->d->m_mutex.unlock();

    return result;
}

MultiSrcElementPrivate::MultiSrcElementPrivate(MultiSrcElement *self):
    self(self)
{
    this->m_mediaSource =
            akPluginManager->create<MediaSource>("MultimediaSource/MultiSrc/Impl/*");
    this->m_mediaSourceImpl =
            akPluginManager->defaultPlugin("MultimediaSource/MultiSrc/Impl/*",
                                           {"MultiSrcImpl"}).id();
}

void MultiSrcElementPrivate::linksChanged(const AkPluginLinks &links)
{
    if (!links.contains("MultimediaSource/MultiSrc/Impl/*")
        || links["MultimediaSource/MultiSrc/Impl/*"] == this->m_mediaSourceImpl)
        return;

    auto state = self->state();
    self->setState(AkElement::ElementStateNull);
    this->m_mutex.lockForWrite();

    QString media;
    bool loop = false;
    bool showLog = false;

    if (this->m_mediaSource) {
        media = this->m_mediaSource->media();
        loop = this->m_mediaSource->loop();
        showLog = this->m_mediaSource->showLog();
    }

    this->m_mediaSource =
            akPluginManager->create<MediaSource>("MultimediaSource/MultiSrc/Impl/*");
    this->m_mutex.unlock();
    this->m_mediaSourceImpl = links["MultimediaSource/MultiSrc/Impl/*"];

    if (!this->m_mediaSource)
        return;

    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::stateChanged,
                     self,
                     &MultiSrcElement::stateChanged);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::oStream,
                     self,
                     &MultiSrcElement::oStream,
                     Qt::DirectConnection);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::error,
                     self,
                     &MultiSrcElement::error);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::durationMSecsChanged,
                     self,
                     &MultiSrcElement::durationMSecsChanged);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::currentTimeMSecsChanged,
                     self,
                     &MultiSrcElement::currentTimeMSecsChanged);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::maxPacketQueueSizeChanged,
                     self,
                     &MultiSrcElement::maxPacketQueueSizeChanged);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::showLogChanged,
                     self,
                     &MultiSrcElement::showLogChanged);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::loopChanged,
                     self,
                     &MultiSrcElement::loopChanged);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::syncChanged,
                     self,
                     &MultiSrcElement::syncChanged);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::mediasChanged,
                     self,
                     &MultiSrcElement::mediasChanged);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::mediaChanged,
                     self,
                     &MultiSrcElement::mediaChanged);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::mediaLoaded,
                     self,
                     &MultiSrcElement::mediaLoaded);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::streamsChanged,
                     self,
                     &MultiSrcElement::streamsChanged);

    this->m_mediaSource->setMedia(media);
    this->m_mediaSource->setLoop(loop);
    this->m_mediaSource->setShowLog(showLog);

    emit self->streamsChanged(self->streams());
    emit self->maxPacketQueueSizeChanged(self->maxPacketQueueSize());

    self->setState(state);
}

#include "moc_multisrcelement.cpp"
