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

#include <QVector>
#include <akaudiopacket.h>

#include "audiodev.h"

#define MAX_SAMPLE_RATE 512e3

class AudioDevPrivate
{
    public:
        QVector<int> m_commonSampleRates;
        int m_latency {25};
};

AudioDev::AudioDev(QObject *parent):
    QObject(parent)
{
    this->d = new AudioDevPrivate;

    // Multiples of 8k sample rates
    for (int rate = 4000; rate < MAX_SAMPLE_RATE; rate *= 2)
        this->d->m_commonSampleRates << rate;

    // Multiples of 48k sample rates
    for (int rate = 6000; rate < MAX_SAMPLE_RATE; rate *= 2)
        this->d->m_commonSampleRates << rate;

    // Multiples of 44.1k sample rates
    for (int rate = 11025; rate < MAX_SAMPLE_RATE; rate *= 2)
        this->d->m_commonSampleRates << rate;

    std::sort(this->d->m_commonSampleRates.begin(),
              this->d->m_commonSampleRates.end());
}

AudioDev::~AudioDev()
{
    delete this->d;
}

int AudioDev::latency() const
{
    return this->d->m_latency;
}

const QVector<int> &AudioDev::commonSampleRates() const
{
    return this->d->m_commonSampleRates;
}

QString AudioDev::error() const
{
    return {};
}

QString AudioDev::defaultInput()
{
    return {};
}

QString AudioDev::defaultOutput()
{
    return {};
}

QStringList AudioDev::inputs()
{
    return {};
}

QStringList AudioDev::outputs()
{
    return {};
}

QString AudioDev::description(const QString &device)
{
    Q_UNUSED(device)

    return {};
}

AkAudioCaps AudioDev::preferredFormat(const QString &device)
{
    Q_UNUSED(device)

    return {};
}

QList<AkAudioCaps::SampleFormat> AudioDev::supportedFormats(const QString &device)
{
    Q_UNUSED(device)

    return {};
}

QList<AkAudioCaps::ChannelLayout> AudioDev::supportedChannelLayouts(const QString &device)
{
    Q_UNUSED(device)

    return {AkAudioCaps::Layout_mono, AkAudioCaps::Layout_stereo};
}

QList<int> AudioDev::supportedSampleRates(const QString &device)
{
    Q_UNUSED(device)

    return {};
}

bool AudioDev::init(const QString &device, const AkAudioCaps &caps)
{
    Q_UNUSED(device)
    Q_UNUSED(caps)

    return false;
}

QByteArray AudioDev::read()
{
    return {};
}

bool AudioDev::write(const AkAudioPacket &packet)
{
    Q_UNUSED(packet)

    return false;
}

bool AudioDev::uninit()
{
    return true;
}

void AudioDev::setLatency(int latency)
{
    if (this->d->m_latency == latency)
        return;

    this->d->m_latency = latency;
    Q_EMIT this->latencyChanged(latency);
}

void AudioDev::resetLatency()
{
    this->setLatency(25);
}

#include "moc_audiodev.cpp"
