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
#include "multisrcglobals.h"
#include "mediasource.h"

Q_GLOBAL_STATIC(MultiSrcGlobals, globalMultiSrc)

template<typename T>
inline QSharedPointer<T> ptr_cast(QObject *obj=nullptr)
{
    return QSharedPointer<T>(static_cast<T *>(obj));
}

using MediaSourcePtr = QSharedPointer<MediaSource>;

class MultiSrcElementPrivate
{
    public:
        MediaSourcePtr m_mediaSource;
        QMutex m_mutexLib;
};

MultiSrcElement::MultiSrcElement():
    AkMultimediaSourceElement()
{
    this->d = new MultiSrcElementPrivate;

    QObject::connect(globalMultiSrc,
                     SIGNAL(codecLibChanged(const QString &)),
                     this,
                     SIGNAL(codecLibChanged(const QString &)));
    QObject::connect(globalMultiSrc,
                     SIGNAL(codecLibChanged(const QString &)),
                     this,
                     SLOT(codecLibUpdated(const QString &)));

    this->codecLibUpdated(globalMultiSrc->codecLib());
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
        return AkCaps();

    return this->d->m_mediaSource->caps(stream);
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

QString MultiSrcElement::codecLib() const
{
    return globalMultiSrc->codecLib();
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

void MultiSrcElement::setCodecLib(const QString &codecLib)
{
    globalMultiSrc->setCodecLib(codecLib);
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

void MultiSrcElement::resetCodecLib()
{
    globalMultiSrc->resetCodecLib();
}

bool MultiSrcElement::setState(AkElement::ElementState state)
{
    if (!this->d->m_mediaSource || !this->d->m_mediaSource->setState(state))
        return false;

    return AkElement::setState(state);
}

void MultiSrcElement::codecLibUpdated(const QString &codecLib)
{
    auto state = this->state();
    this->setState(AkElement::ElementStateNull);

    QString media;
    bool loop = false;
    bool showLog = false;

    if (this->d->m_mediaSource) {
        media = this->d->m_mediaSource->media();
        loop = this->d->m_mediaSource->loop();
        showLog = this->d->m_mediaSource->showLog();
    }

    this->d->m_mutexLib.lock();

    this->d->m_mediaSource =
            ptr_cast<MediaSource>(MultiSrcElement::loadSubModule("MultiSrc",
                                                                 codecLib));

    if (!this->d->m_mediaSource) {
        this->d->m_mutexLib.unlock();

        return;
    }

    QObject::connect(this->d->m_mediaSource.data(),
                     SIGNAL(oStream(const AkPacket &)),
                     this,
                     SIGNAL(oStream(const AkPacket &)),
                     Qt::DirectConnection);
    QObject::connect(this->d->m_mediaSource.data(),
                     SIGNAL(error(const QString &)),
                     this,
                     SIGNAL(error(const QString &)));
    QObject::connect(this->d->m_mediaSource.data(),
                     SIGNAL(maxPacketQueueSizeChanged(qint64)),
                     this,
                     SIGNAL(maxPacketQueueSizeChanged(qint64)));
    QObject::connect(this->d->m_mediaSource.data(),
                     SIGNAL(showLogChanged(bool)),
                     this,
                     SIGNAL(showLogChanged(bool)));
    QObject::connect(this->d->m_mediaSource.data(),
                     SIGNAL(loopChanged(bool)),
                     this,
                     SIGNAL(loopChanged(bool)));
    QObject::connect(this->d->m_mediaSource.data(),
                     SIGNAL(mediasChanged(const QStringList &)),
                     this,
                     SIGNAL(mediasChanged(const QStringList &)));
    QObject::connect(this->d->m_mediaSource.data(),
                     SIGNAL(mediaChanged(const QString &)),
                     this,
                     SIGNAL(mediaChanged(const QString &)));
    QObject::connect(this->d->m_mediaSource.data(),
                     SIGNAL(streamsChanged(const QList<int> &)),
                     this,
                     SIGNAL(streamsChanged(const QList<int> &)));

    this->d->m_mutexLib.unlock();

    this->d->m_mediaSource->setMedia(media);
    this->d->m_mediaSource->setLoop(loop);
    this->d->m_mediaSource->setShowLog(showLog);

    emit this->streamsChanged(this->streams());
    emit this->maxPacketQueueSizeChanged(this->maxPacketQueueSize());

    this->setState(state);
}

#include "moc_multisrcelement.cpp"
