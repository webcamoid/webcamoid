/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "multisrcelement.h"

MultiSrcElement::MultiSrcElement():
    QbMultimediaSourceElement()
{
    this->m_mediaSource.setLoop(this->loop());

    QObject::connect(this,
                     &MultiSrcElement::loopChanged,
                     &this->m_mediaSource,
                     &MediaSource::setLoop);
    QObject::connect(&this->m_mediaSource,
                     &MediaSource::oStream,
                     this,
                     &MultiSrcElement::oStream,
                     Qt::DirectConnection);
    QObject::connect(&this->m_mediaSource,
                     &MediaSource::error,
                     this,
                     &MultiSrcElement::error);
    QObject::connect(&this->m_mediaSource,
                     &MediaSource::maxPacketQueueSizeChanged,
                     this,
                     &MultiSrcElement::maxPacketQueueSizeChanged);
    QObject::connect(&this->m_mediaSource,
                     &MediaSource::showLogChanged,
                     this,
                     &MultiSrcElement::showLogChanged);
    QObject::connect(&this->m_mediaSource,
                     &MediaSource::loopChanged,
                     this,
                     &MultiSrcElement::loopChanged);
    QObject::connect(&this->m_mediaSource,
                     &MediaSource::mediasChanged,
                     this,
                     &MultiSrcElement::mediasChanged);
    QObject::connect(&this->m_mediaSource,
                     &MediaSource::mediaChanged,
                     this,
                     &MultiSrcElement::mediaChanged);
    QObject::connect(&this->m_mediaSource,
                     &MediaSource::streamsChanged,
                     this,
                     &MultiSrcElement::streamsChanged);
}

MultiSrcElement::~MultiSrcElement()
{
}

QObject *MultiSrcElement::controlInterface(QQmlEngine *engine,
                                           const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/MultiSrc/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("MultiSrc", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

QStringList MultiSrcElement::medias() const
{
    return this->m_mediaSource.medias();
}

QString MultiSrcElement::media() const
{
    return this->m_mediaSource.media();
}

QList<int> MultiSrcElement::streams() const
{
    return this->m_mediaSource.streams();
}

QList<int> MultiSrcElement::listTracks(const QString &type)
{
    return this->m_mediaSource.listTracks(type);
}

QString MultiSrcElement::streamLanguage(int stream)
{
    return this->m_mediaSource.streamLanguage(stream);
}

int MultiSrcElement::defaultStream(const QString &mimeType)
{
    return this->m_mediaSource.defaultStream(mimeType);
}

QString MultiSrcElement::description(const QString &media) const
{
    return this->m_mediaSource.description(media);
}

QbCaps MultiSrcElement::caps(int stream)
{
    return this->m_mediaSource.caps(stream);
}

qint64 MultiSrcElement::maxPacketQueueSize() const
{
    return this->m_mediaSource.maxPacketQueueSize();
}

bool MultiSrcElement::showLog() const
{
    return this->m_mediaSource.showLog();
}

void MultiSrcElement::stateChange(QbElement::ElementState from,
                                  QbElement::ElementState to)
{
    if (from == QbElement::ElementStateNull
        && to == QbElement::ElementStatePaused)
        this->m_mediaSource.init();
    else if (from == QbElement::ElementStatePaused
             && to == QbElement::ElementStateNull)
        this->m_mediaSource.uninit();
}

void MultiSrcElement::setMedia(const QString &media)
{
    this->m_mediaSource.setMedia(media);
}

void MultiSrcElement::setStreams(const QList<int> &streams)
{
    this->m_mediaSource.setStreams(streams);
}

void MultiSrcElement::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    this->m_mediaSource.setMaxPacketQueueSize(maxPacketQueueSize);
}

void MultiSrcElement::setShowLog(bool showLog)
{
    this->m_mediaSource.setShowLog(showLog);
}

void MultiSrcElement::resetMedia()
{
    this->m_mediaSource.resetMedia();
}

void MultiSrcElement::resetStreams()
{
    this->m_mediaSource.resetStreams();
}

void MultiSrcElement::resetMaxPacketQueueSize()
{
    this->m_mediaSource.resetMaxPacketQueueSize();
}

void MultiSrcElement::resetShowLog()
{
    this->m_mediaSource.resetShowLog();
}
