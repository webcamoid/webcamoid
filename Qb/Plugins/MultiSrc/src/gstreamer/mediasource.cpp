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

#include "mediasource.h"

MediaSource::MediaSource(QObject *parent): QObject(parent)
{
}

MediaSource::~MediaSource()
{
}

QStringList MediaSource::medias() const
{
}

QString MediaSource::media() const
{
}

QList<int> MediaSource::streams() const
{
}

bool MediaSource::loop() const
{
}

int MediaSource::defaultStream(const QString &mimeType)
{
}

QString MediaSource::description(const QString &media) const
{
}

QbCaps MediaSource::caps(int stream)
{
}

qint64 MediaSource::maxPacketQueueSize() const
{
}

bool MediaSource::showLog() const
{
}

void MediaSource::setMedia(const QString &media)
{
}

void MediaSource::setStreams(const QList<int> &streams)
{
}

void MediaSource::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
}

void MediaSource::setShowLog(bool showLog)
{
}

void MediaSource::setLoop(bool loop)
{
}

void MediaSource::resetMedia()
{
}

void MediaSource::resetStreams()
{
}

void MediaSource::resetMaxPacketQueueSize()
{
}

void MediaSource::resetShowLog()
{
}

void MediaSource::resetLoop()
{
}

bool MediaSource::init()
{
}

void MediaSource::uninit()
{
}
