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

#include <QVariantMap>

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
    return this->m_location;
}

QString MediaWriter::outputFormat() const
{
    return QString();
}

QVariantList MediaWriter::streams() const
{
    return QVariantList();
}

qint64 MediaWriter::maxPacketQueueSize() const
{
    return 0;
}

QStringList MediaWriter::formatsBlackList() const
{
    return this->m_formatsBlackList;
}

QStringList MediaWriter::codecsBlackList() const
{
    return this->m_codecsBlackList;
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

QVariantList MediaWriter::formatOptions()
{
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

QVariantList MediaWriter::codecOptions(int index)
{
    Q_UNUSED(index)

    return QVariantList();
}

void MediaWriter::setLocation(const QString &location)
{
    if (this->m_location == location)
        return;

    this->m_location = location;
    emit this->locationChanged(location);
}

void MediaWriter::setOutputFormat(const QString &outputFormat)
{
    Q_UNUSED(outputFormat);
}

void MediaWriter::setFormatOptions(const QVariantMap &formatOptions)
{
    Q_UNUSED(formatOptions);
}

void MediaWriter::setCodecOptions(int index, const QVariantMap &codecOptions)
{
    Q_UNUSED(index)
    Q_UNUSED(codecOptions)
}

void MediaWriter::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    Q_UNUSED(maxPacketQueueSize);
}

void MediaWriter::setFormatsBlackList(const QStringList &formatsBlackList)
{
    if (this->m_formatsBlackList == formatsBlackList)
        return;

    this->m_formatsBlackList = formatsBlackList;
    emit this->formatsBlackListChanged(formatsBlackList);
}

void MediaWriter::setCodecsBlackList(const QStringList &codecsBlackList)
{
    if (this->m_codecsBlackList == codecsBlackList)
        return;

    this->m_codecsBlackList = codecsBlackList;
    emit this->codecsBlackListChanged(codecsBlackList);
}

void MediaWriter::resetLocation()
{
    this->setLocation("");
}

void MediaWriter::resetOutputFormat()
{
}

void MediaWriter::resetFormatOptions()
{
}

void MediaWriter::resetCodecOptions(int index)
{
    Q_UNUSED(index)
}

void MediaWriter::resetMaxPacketQueueSize()
{
}

void MediaWriter::resetFormatsBlackList()
{
    this->setFormatsBlackList({});
}

void MediaWriter::resetCodecsBlackList()
{
    this->setCodecsBlackList({});
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

#include "moc_mediawriter.cpp"
