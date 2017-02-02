/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include "mediawriter.h"

MediaWriter::MediaWriter(QObject *parent):
    QObject(parent)
{
}

MediaWriter::~MediaWriter()
{
}

QString MediaWriter::location() const
{
    return QString();
}

QString MediaWriter::outputFormat() const
{
    return QString();
}

QVariantMap MediaWriter::formatOptions() const
{
    return QVariantMap();
}

QVariantList MediaWriter::streams() const
{
    return QVariantList();
}

qint64 MediaWriter::maxPacketQueueSize() const
{
    return 0;
}

QStringList MediaWriter::supportedFormats()
{
    return QStringList();
}

QStringList MediaWriter::fileExtensions(const QString &format)
{
    Q_UNUSED(format)

    return QStringList();
}

QString MediaWriter::formatDescription(const QString &format)
{
    Q_UNUSED(format)

    return QString();
}

QVariantList MediaWriter::formatOptions(const QString &format)
{
    Q_UNUSED(format)

    return QVariantList();
}

QStringList MediaWriter::supportedCodecs(const QString &format)
{
    Q_UNUSED(format)

    return QStringList();
}

QStringList MediaWriter::supportedCodecs(const QString &format,
                                       const QString &type)
{
    Q_UNUSED(format)
    Q_UNUSED(type)

    return QStringList();
}

QString MediaWriter::defaultCodec(const QString &format, const QString &type)
{
    Q_UNUSED(format)
    Q_UNUSED(type)

    return QString();
}

QString MediaWriter::codecDescription(const QString &codec)
{
    Q_UNUSED(codec)

    return QString();
}

QString MediaWriter::codecType(const QString &codec)
{
    Q_UNUSED(codec)

    return QString();
}

QVariantMap MediaWriter::defaultCodecParams(const QString &codec)
{
    Q_UNUSED(codec)

    return QVariantMap();
}

QVariantList MediaWriter::codecOptions(const QString &codec)
{
    Q_UNUSED(codec)

    return QVariantList();
}

QVariantMap MediaWriter::addStream(int streamIndex, const AkCaps &streamCaps)
{
    Q_UNUSED(streamIndex)
    Q_UNUSED(streamCaps)

    return QVariantMap();
}

QVariantMap MediaWriter::addStream(int streamIndex,
                                   const AkCaps &streamCaps,
                                   const QVariantMap &codecParams)
{
    Q_UNUSED(streamIndex)
    Q_UNUSED(streamCaps)
    Q_UNUSED(codecParams)

    return QVariantMap();
}

QVariantMap MediaWriter::updateStream(int index)
{
    Q_UNUSED(index)

    return QVariantMap();
}

QVariantMap MediaWriter::updateStream(int index, const QVariantMap &codecParams)
{
    Q_UNUSED(index)
    Q_UNUSED(codecParams)

    return QVariantMap();
}

void MediaWriter::setLocation(const QString &location)
{
    Q_UNUSED(location);
}

void MediaWriter::setOutputFormat(const QString &outputFormat)
{
    Q_UNUSED(outputFormat);
}

void MediaWriter::setFormatOptions(const QVariantMap &formatOptions)
{
    Q_UNUSED(formatOptions);
}

void MediaWriter::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    Q_UNUSED(maxPacketQueueSize);
}

void MediaWriter::resetLocation()
{
}

void MediaWriter::resetOutputFormat()
{
}

void MediaWriter::resetFormatOptions()
{
}

void MediaWriter::resetMaxPacketQueueSize()
{
}

void MediaWriter::enqueuePacket(const AkPacket &packet)
{
    Q_UNUSED(packet)
}

void MediaWriter::clearStreams()
{
}

bool MediaWriter::init()
{
    return false;
}

void MediaWriter::uninit()
{
}
