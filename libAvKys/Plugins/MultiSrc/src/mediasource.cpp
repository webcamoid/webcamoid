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

#include "mediasource.h"

MediaSource::MediaSource(QObject *parent):
    QObject(parent)
{
}

QStringList MediaSource::medias() const
{
    return QStringList();
}

QString MediaSource::media() const
{
    return QString();
}

QList<int> MediaSource::streams() const
{
    return QList<int>();
}

QList<int> MediaSource::listTracks(AkCaps::CapsType type)
{
    Q_UNUSED(type)

    return QList<int>();
}

QString MediaSource::streamLanguage(int stream)
{
    Q_UNUSED(stream)

    return QString();
}

bool MediaSource::loop() const
{
    return false;
}

bool MediaSource::sync() const
{
    return true;
}

int MediaSource::defaultStream(AkCaps::CapsType type)
{
    Q_UNUSED(type)

    return -1;
}

QString MediaSource::description(const QString &media) const
{
    Q_UNUSED(media)

    return QString();
}

AkCaps MediaSource::caps(int stream)
{
    Q_UNUSED(stream)

    return AkCaps();
}

qint64 MediaSource::durationMSecs()
{
    return 0;
}

qint64 MediaSource::currentTimeMSecs()
{
    return 0;
}

qint64 MediaSource::maxPacketQueueSize() const
{
    return 0;
}

bool MediaSource::showLog() const
{
    return false;
}

AkElement::ElementState MediaSource::state() const
{
    return AkElement::ElementStateNull;
}

void MediaSource::seek(qint64 seekTo, SeekPosition position)
{
    Q_UNUSED(seekTo)
    Q_UNUSED(position)
}

void MediaSource::setMedia(const QString &media)
{
    Q_UNUSED(media)
}

void MediaSource::setStreams(const QList<int> &streams)
{
    Q_UNUSED(streams)
}

void MediaSource::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    Q_UNUSED(maxPacketQueueSize)
}

void MediaSource::setShowLog(bool showLog)
{
    Q_UNUSED(showLog)
}

void MediaSource::setLoop(bool loop)
{
    Q_UNUSED(loop)
}

void MediaSource::setSync(bool sync)
{
    Q_UNUSED(sync)
}

bool MediaSource::setState(AkElement::ElementState state)
{
    Q_UNUSED(state)

    return false;
}

void MediaSource::resetMedia()
{
    this->setMedia({});
}

void MediaSource::resetStreams()
{
    this->setStreams({});
}

void MediaSource::resetMaxPacketQueueSize()
{
    this->setMaxPacketQueueSize(0);
}

void MediaSource::resetShowLog()
{
    this->setShowLog(false);
}

void MediaSource::resetLoop()
{
    this->setLoop(false);
}

void MediaSource::resetSync()
{
    this->setSync(true);
}

void MediaSource::resetState()
{
    this->setState(AkElement::ElementStateNull);
}

#include "moc_mediasource.cpp"
