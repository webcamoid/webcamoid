/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include "multisrcelement.h"

Q_GLOBAL_STATIC_WITH_ARGS(QStringList, preferredFramework, ({"ffmpeg", "gstreamer"}))

template<typename T>
inline QSharedPointer<T> ptr_init(QObject *obj=nullptr)
{
    if (!obj)
        return QSharedPointer<T>(new T());

    return QSharedPointer<T>(static_cast<T *>(obj));
}

MultiSrcElement::MultiSrcElement():
    AkMultimediaSourceElement(),
    m_mediaSource(ptr_init<MediaSource>())
{
    QObject::connect(this,
                     &MultiSrcElement::codecLibChanged,
                     this,
                     &MultiSrcElement::codecLibUpdated);

    this->resetCodecLib();
}

MultiSrcElement::~MultiSrcElement()
{
    this->setState(AkElement::ElementStateNull);
}

QObject *MultiSrcElement::controlInterface(QQmlEngine *engine,
                                           const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/MultiSrc/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("MultiSrc", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);

    if (!item) {
        delete context;

        return NULL;
    }

    context->setParent(item);

    return item;
}

QStringList MultiSrcElement::medias() const
{
    return this->m_mediaSource->medias();
}

QString MultiSrcElement::media() const
{
    return this->m_mediaSource->media();
}

QList<int> MultiSrcElement::streams() const
{
    return this->m_mediaSource->streams();
}

bool MultiSrcElement::loop() const
{
    return this->m_mediaSource->loop();
}

QList<int> MultiSrcElement::listTracks(const QString &type)
{
    return this->m_mediaSource->listTracks(type);
}

QString MultiSrcElement::streamLanguage(int stream)
{
    return this->m_mediaSource->streamLanguage(stream);
}

int MultiSrcElement::defaultStream(const QString &mimeType)
{
    return this->m_mediaSource->defaultStream(mimeType);
}

QString MultiSrcElement::description(const QString &media) const
{
    return this->m_mediaSource->description(media);
}

AkCaps MultiSrcElement::caps(int stream)
{
    return this->m_mediaSource->caps(stream);
}

qint64 MultiSrcElement::maxPacketQueueSize() const
{
    return this->m_mediaSource->maxPacketQueueSize();
}

bool MultiSrcElement::showLog() const
{
    return this->m_mediaSource->showLog();
}

QString MultiSrcElement::codecLib() const
{
    return this->m_codecLib;
}

void MultiSrcElement::setMedia(const QString &media)
{
    this->m_mediaSource->setMedia(media);
}

void MultiSrcElement::setStreams(const QList<int> &streams)
{
    this->m_mediaSource->setStreams(streams);
}

void MultiSrcElement::setLoop(bool loop)
{
    this->m_mediaSource->setLoop(loop);
}

void MultiSrcElement::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    this->m_mediaSource->setMaxPacketQueueSize(maxPacketQueueSize);
}

void MultiSrcElement::setShowLog(bool showLog)
{
    this->m_mediaSource->setShowLog(showLog);
}

void MultiSrcElement::setCodecLib(const QString &codecLib)
{
    if (this->m_codecLib == codecLib)
        return;

    this->m_codecLib = codecLib;
    emit this->codecLibChanged(codecLib);
}

void MultiSrcElement::resetMedia()
{
    this->m_mediaSource->resetMedia();
}

void MultiSrcElement::resetStreams()
{
    this->m_mediaSource->resetStreams();
}

void MultiSrcElement::resetLoop()
{
    this->m_mediaSource->resetLoop();
}

void MultiSrcElement::resetMaxPacketQueueSize()
{
    this->m_mediaSource->resetMaxPacketQueueSize();
}

void MultiSrcElement::resetShowLog()
{
    this->m_mediaSource->resetShowLog();
}

void MultiSrcElement::resetCodecLib()
{
    auto subModules = this->listSubModules("MultiSrc");

    for (const QString &framework: *preferredFramework)
        if (subModules.contains(framework)) {
            this->setCodecLib(framework);

            return;
        }

    if (this->m_codecLib.isEmpty() && !subModules.isEmpty())
        this->setCodecLib(subModules.first());
    else
        this->setCodecLib("");
}

bool MultiSrcElement::setState(AkElement::ElementState state)
{
    if (!this->m_mediaSource->setState(state))
        return false;

    return AkElement::setState(state);
}

void MultiSrcElement::codecLibUpdated(const QString &codecLib)
{
    auto state = this->state();
    this->setState(AkElement::ElementStateNull);

    this->m_mutexLib.lock();

    this->m_mediaSource =
            ptr_init<MediaSource>(this->loadSubModule("MultiSrc", codecLib));

    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::oStream,
                     this,
                     &MultiSrcElement::oStream,
                     Qt::DirectConnection);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::error,
                     this,
                     &MultiSrcElement::error);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::maxPacketQueueSizeChanged,
                     this,
                     &MultiSrcElement::maxPacketQueueSizeChanged);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::showLogChanged,
                     this,
                     &MultiSrcElement::showLogChanged);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::loopChanged,
                     this,
                     &MultiSrcElement::loopChanged);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::mediasChanged,
                     this,
                     &MultiSrcElement::mediasChanged);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::mediaChanged,
                     this,
                     &MultiSrcElement::mediaChanged);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::streamsChanged,
                     this,
                     &MultiSrcElement::streamsChanged);

    this->m_mutexLib.unlock();

    emit this->mediasChanged(this->medias());
    emit this->mediaChanged(this->media());
    emit this->streamsChanged(this->streams());
    emit this->loopChanged(this->loop());
    emit this->maxPacketQueueSizeChanged(this->maxPacketQueueSize());
    emit this->showLogChanged(this->showLog());

    this->setState(state);
}
