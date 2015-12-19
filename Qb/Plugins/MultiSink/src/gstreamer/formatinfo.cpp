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

#include "formatinfo.h"

FormatInfo::FormatInfo(QObject *parent):
    QObject(parent)
{

}

FormatInfo::FormatInfo(const FormatInfo &other):
    QObject()
{
    this->m_name = other.m_name;
    this->m_longName = other.m_longName;
    this->m_extensions = other.m_extensions;
    this->m_defaultAudioCodec = other.m_defaultAudioCodec;
    this->m_audioCodec = other.m_audioCodec;
    this->m_defaultVideoCodec = other.m_defaultVideoCodec;
    this->m_videoCodec = other.m_videoCodec;
}

FormatInfo &FormatInfo::operator =(const FormatInfo &other)
{
    if (this != &other) {
        this->m_name = other.m_name;
        this->m_longName = other.m_longName;
        this->m_extensions = other.m_extensions;
        this->m_defaultAudioCodec = other.m_defaultAudioCodec;
        this->m_audioCodec = other.m_audioCodec;
        this->m_defaultVideoCodec = other.m_defaultVideoCodec;
        this->m_videoCodec = other.m_videoCodec;
    }

    return *this;
}

QString FormatInfo::name() const
{
    return this->m_name;
}

QString &FormatInfo::name()
{
    return this->m_name;
}

QString FormatInfo::longName() const
{
    return this->m_longName;
}

QString &FormatInfo::longName()
{
    return this->m_longName;
}

QStringList FormatInfo::extensions() const
{
    return this->m_extensions;
}

QStringList &FormatInfo::extensions()
{
    return this->m_extensions;
}

QString FormatInfo::defaultAudioCodec() const
{
    return this->m_defaultAudioCodec;
}

QString &FormatInfo::defaultAudioCodec()
{
    return this->m_defaultAudioCodec;
}

QStringList FormatInfo::audioCodec() const
{
    return this->m_audioCodec;
}

QStringList &FormatInfo::audioCodec()
{
    return this->m_audioCodec;
}

QString FormatInfo::defaultVideoCodec() const
{
    return this->m_defaultVideoCodec;
}

QString &FormatInfo::defaultVideoCodec()
{
    return this->m_defaultVideoCodec;
}

QStringList FormatInfo::videoCodec() const
{
    return this->m_videoCodec;
}

QStringList &FormatInfo::videoCodec()
{
    return this->m_videoCodec;
}

void FormatInfo::setName(const QString &name)
{
    if (this->m_name == name)
        return;

    this->m_name = name;
    emit this->nameChanged(name);
}

void FormatInfo::setLongName(const QString &longName)
{
    if (this->m_longName == longName)
        return;

    this->m_longName = longName;
    emit this->longNameChanged(longName);
}

void FormatInfo::setExtensions(const QStringList &extensions)
{
    if (this->m_extensions == extensions)
        return;

    this->m_extensions = extensions;
    emit this->extensionsChanged(extensions);
}

void FormatInfo::setDefaultAudioCodec(const QString &defaultAudioCodec)
{
    if (this->m_defaultAudioCodec == defaultAudioCodec)
        return;

    this->m_defaultAudioCodec = defaultAudioCodec;
    emit this->defaultAudioCodecChanged(defaultAudioCodec);
}

void FormatInfo::setAudioCodec(const QStringList &audioCodec)
{
    if (this->m_audioCodec == audioCodec)
        return;

    this->m_audioCodec = audioCodec;
    emit this->audioCodecChanged(audioCodec);
}

void FormatInfo::setDefaultVideoCodec(const QString &defaultVideoCodec)
{
    if (this->m_defaultVideoCodec == defaultVideoCodec)
        return;

    this->m_defaultVideoCodec = defaultVideoCodec;
    emit this->defaultVideoCodecChanged(defaultVideoCodec);
}

void FormatInfo::setVideoCodec(const QStringList &videoCodec)
{
    if (this->m_videoCodec == videoCodec)
        return;

    this->m_videoCodec = videoCodec;
    emit this->videoCodecChanged(videoCodec);
}

void FormatInfo::resetName()
{
    this->setName(QString());
}

void FormatInfo::resetLongName()
{
    this->setLongName(QString());
}

void FormatInfo::resetExtensions()
{
    this->setExtensions(QStringList());
}

void FormatInfo::resetDefaultAudioCodec()
{
    this->setDefaultAudioCodec(QString());
}

void FormatInfo::resetAudioCodec()
{
    this->setAudioCodec(QStringList());
}

void FormatInfo::resetDefaultVideoCodec()
{
    this->setDefaultVideoCodec(QString());
}

void FormatInfo::resetVideoCodec()
{
    this->setVideoCodec(QStringList());
}
