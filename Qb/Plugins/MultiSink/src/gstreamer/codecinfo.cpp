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

#include "codecinfo.h"

CodecInfo::CodecInfo(QObject *parent):
    QObject(parent)
{

}

CodecInfo::CodecInfo(const CodecInfo &other):
    QObject()
{
    this->m_name = other.m_name;
    this->m_longName = other.m_longName;
    this->m_supportedSamplerates = other.m_supportedSamplerates;
    this->m_sampleFormats = other.m_sampleFormats;
    this->m_channelLayouts = other.m_channelLayouts;
    this->m_supportedFramerates = other.m_supportedFramerates;
    this->m_pixelFormats = other.m_pixelFormats;
}

CodecInfo &CodecInfo::operator =(const CodecInfo &other)
{
    if (this != &other) {
        this->m_name = other.m_name;
        this->m_longName = other.m_longName;
        this->m_supportedSamplerates = other.m_supportedSamplerates;
        this->m_sampleFormats = other.m_sampleFormats;
        this->m_channelLayouts = other.m_channelLayouts;
        this->m_supportedFramerates = other.m_supportedFramerates;
        this->m_pixelFormats = other.m_pixelFormats;
    }

    return *this;
}

QString CodecInfo::name() const
{
    return this->m_name;
}

QString &CodecInfo::name()
{
    return this->m_name;
}

QString CodecInfo::longName() const
{
    return this->m_longName;
}

QString &CodecInfo::longName()
{
    return this->m_longName;
}

IntList CodecInfo::supportedSamplerates() const
{
    return this->m_supportedSamplerates;
}

IntList &CodecInfo::supportedSamplerates()
{
    return this->m_supportedSamplerates;
}

QStringList CodecInfo::sampleFormats() const
{
    return this->m_sampleFormats;
}

QStringList &CodecInfo::sampleFormats()
{
    return this->m_sampleFormats;
}

QStringList CodecInfo::channelLayouts() const
{
    return this->m_channelLayouts;
}

QStringList &CodecInfo::channelLayouts()
{
    return this->m_channelLayouts;
}

FracList CodecInfo::supportedFramerates() const
{
    return this->m_supportedFramerates;
}

FracList &CodecInfo::supportedFramerates()
{
    return this->m_supportedFramerates;
}

QStringList CodecInfo::pixelFormats() const
{
    return this->m_pixelFormats;
}

QStringList &CodecInfo::pixelFormats()
{
    return this->m_pixelFormats;
}

void CodecInfo::setName(const QString &name)
{
    if (this->m_name == name)
        return;

    this->m_name = name;
    emit this->nameChanged(name);
}

void CodecInfo::setLongName(const QString &longName)
{
    if (this->m_longName == longName)
        return;

    this->m_longName = longName;
    emit this->longNameChanged(longName);
}

void CodecInfo::setSupportedSamplerates(const IntList &supportedSamplerates)
{
    if (this->m_supportedSamplerates == supportedSamplerates)
        return;

    this->m_supportedSamplerates = supportedSamplerates;
    emit this->supportedSampleratesChanged(supportedSamplerates);
}

void CodecInfo::setSampleFormats(const QStringList &sampleFormats)
{
    if (this->m_sampleFormats == sampleFormats)
        return;

    this->m_sampleFormats = sampleFormats;
    emit this->sampleFormatsChanged(sampleFormats);
}

void CodecInfo::setChannelLayouts(const QStringList &channelLayouts)
{
    if (this->m_channelLayouts == channelLayouts)
        return;

    this->m_channelLayouts = channelLayouts;
    emit this->channelLayoutsChanged(channelLayouts);
}

void CodecInfo::setSupportedFramerates(const FracList &supportedFramerates)
{
    if (this->m_supportedFramerates == supportedFramerates)
        return;

    this->m_supportedFramerates = supportedFramerates;
    emit this->supportedFrameratesChanged(supportedFramerates);
}

void CodecInfo::setPixelFormats(const QStringList &pixelFormats)
{
    if (this->m_pixelFormats == pixelFormats)
        return;

    this->m_pixelFormats = pixelFormats;
    emit this->pixelFormatsChanged(pixelFormats);
}

void CodecInfo::resetName()
{
    this->setName(QString());
}

void CodecInfo::resetLongName()
{
    this->setLongName(QString());
}

void CodecInfo::resetSupportedSamplerates()
{
    this->setSupportedSamplerates(IntList());
}

void CodecInfo::resetSampleFormats()
{
    this->setSampleFormats(QStringList());
}

void CodecInfo::resetChannelLayouts()
{
    this->setChannelLayouts(QStringList());
}

void CodecInfo::resetSupportedFramerates()
{
    this->setSupportedFramerates(FracList());
}

void CodecInfo::resetPixelFormats()
{
    this->setPixelFormats(QStringList());
}
