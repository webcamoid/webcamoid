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

#include <QCoreApplication>
#include <QMap>

#include "audiodev.h"

typedef QMap<AkAudioCaps::SampleFormat, pa_sample_format_t> SampleFormatMap;

inline SampleFormatMap initSampleFormatMap()
{
    SampleFormatMap sampleFormat;

    return sampleFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatMap, sampleFormats, (initSampleFormatMap()))

AudioDev::AudioDev(QObject *parent):
    QObject(parent)
{
    this->m_defaultChannels = 0;
    this->m_defaultRate = 0;
    this->m_curBps = 0;
    this->m_curChannels = 0;
}

AudioDev::~AudioDev()
{
    this->uninit();
}

QString AudioDev::error() const
{
    return this->m_error;
}

// Get native format for the default audio device.
bool AudioDev::preferredFormat(DeviceMode mode,
                               AkAudioCaps::SampleFormat *sampleFormat,
                               int *channels,
                               int *sampleRate)
{
    return false;
}

bool AudioDev::init(DeviceMode mode,
                    AkAudioCaps::SampleFormat sampleFormat,
                    int channels,
                    int sampleRate)
{
    return false;
}

QByteArray AudioDev::read(int samples)
{
    return QByteArray();
}

bool AudioDev::write(const QByteArray &frame)
{
    return false;
}

bool AudioDev::uninit()
{
    return true;
}
