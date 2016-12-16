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

#include <QDir>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "audiodevoss.h"

#define BUFFER_SIZE 1024 // In samples

typedef QMap<AkAudioCaps::SampleFormat, int> SampleFormatMap;

inline SampleFormatMap initSampleFormatMap()
{
    SampleFormatMap sampleFormat = {
        {AkAudioCaps::SampleFormat_s8   , AFMT_S8},
        {AkAudioCaps::SampleFormat_u8   , AFMT_U8},
        {AkAudioCaps::SampleFormat_s16  , AFMT_S16_NE},
        {AkAudioCaps::SampleFormat_s16le, AFMT_S16_LE},
        {AkAudioCaps::SampleFormat_s16be, AFMT_S16_BE},
        {AkAudioCaps::SampleFormat_u16le, AFMT_U16_LE},
        {AkAudioCaps::SampleFormat_u16be, AFMT_U16_BE},
    };

    return sampleFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatMap, sampleFormats, (initSampleFormatMap()))

AudioDevOSS::AudioDevOSS(QObject *parent):
    AudioDev(parent)
{
    this->m_fsWatcher = new QFileSystemWatcher({"/dev"}, this);

    QObject::connect(this->m_fsWatcher,
                     &QFileSystemWatcher::directoryChanged,
                     this,
                     &AudioDevOSS::updateDevices);

    this->updateDevices();
}

AudioDevOSS::~AudioDevOSS()
{
    this->uninit();

    if (this->m_fsWatcher)
        delete this->m_fsWatcher;
}

QString AudioDevOSS::error() const
{
    return this->m_error;
}

QString AudioDevOSS::defaultInput()
{
    return this->m_defaultSource;
}

QString AudioDevOSS::defaultOutput()
{
    return this->m_defaultSink;
}

QStringList AudioDevOSS::inputs()
{
    return this->m_sources;
}

QStringList AudioDevOSS::outputs()
{
    return this->m_sinks;
}

QString AudioDevOSS::description(const QString &device)
{
    return this->m_pinDescriptionMap.value(device);
}

AkAudioCaps AudioDevOSS::preferredFormat(const QString &device)
{
    return this->m_pinCapsMap.value(device);
}

bool AudioDevOSS::init(const QString &device, const AkAudioCaps &caps)
{
    QMutexLocker mutexLockeer(&this->m_mutex);

    this->m_deviceFile.setFileName(QString(device)
                                   .remove(QRegExp(":Input$|:Output$")));

    if (!this->m_deviceFile.open(device.endsWith(":Input")?
                                 QIODevice::ReadOnly: QIODevice::WriteOnly))
        return false;

    int format;
    format = sampleFormats->value(caps.format(), AFMT_QUERY);

    if (ioctl(this->m_deviceFile.handle(), SNDCTL_DSP_SETFMT, &format) < 0)
        goto init_fail;

    int stereo;
    stereo = caps.channels() > 1? 1: 0;

    if (ioctl(this->m_deviceFile.handle(), SNDCTL_DSP_STEREO, &stereo) < 0)
        goto init_fail;

    int sampleRate;
    sampleRate = caps.rate();

    if (ioctl(this->m_deviceFile.handle(), SNDCTL_DSP_SPEED, &sampleRate) < 0)
        goto init_fail;

    if (device.endsWith(":Output")) {
        int fragment = this->m_fragmentSizeMap.value(device);
        ioctl(this->m_deviceFile.handle(), SNDCTL_DSP_SETFRAGMENT, &fragment);
    }

    this->m_curCaps = caps;

    return true;

init_fail:
    this->m_deviceFile.close();

    return false;
}

QByteArray AudioDevOSS::read(int samples)
{
    QMutexLocker mutexLockeer(&this->m_mutex);

    if (!this->m_deviceFile.isOpen())
        return QByteArray();

    QByteArray buffer;
    int bufferSize = samples
                     * this->m_curCaps.channels()
                     * AkAudioCaps::bitsPerSample(this->m_curCaps.format())
                     / 8;

    while (bufferSize > 0) {
        auto data = this->m_deviceFile.read(bufferSize);

        if (data.size() > 0) {
            buffer += data;
            bufferSize -= data.size();
        }
    }

    return buffer;
}

bool AudioDevOSS::write(const AkAudioPacket &packet)
{
    QMutexLocker mutexLockeer(&this->m_mutex);

    if (!this->m_deviceFile.isOpen())
        return false;

    return this->m_deviceFile.write(packet.buffer()) > 0;
}

bool AudioDevOSS::uninit()
{
    QMutexLocker mutexLockeer(&this->m_mutex);

    this->m_deviceFile.close();
    this->m_curCaps = AkAudioCaps();

    return true;
}

AkAudioCaps AudioDevOSS::deviceCaps(const QString &device, int *fragmentSize) const
{
    QFile pcmFile(QString(device)
                    .remove(QRegExp(":Input$|:Output$")));

    if (!pcmFile.open(device.endsWith(":Input")?
                      QIODevice::ReadOnly: QIODevice::WriteOnly))
        return AkAudioCaps();

    int formats = AFMT_QUERY;

    if (ioctl(pcmFile.handle(), SNDCTL_DSP_GETFMTS, &formats) < 0)
        goto deviceCaps_fail;

    static const QVector<int> preferredFormats = {
        AFMT_S16_NE,
        AFMT_S16_LE,
        AFMT_S16_BE,
        AFMT_U16_LE,
        AFMT_U16_BE,
        AFMT_S8,
        AFMT_U8
    };

    int format;
    format = AFMT_QUERY;

    for (const auto &fmt: preferredFormats)
        if (formats & fmt) {
            format = fmt;

            break;
        }

    if (format == AFMT_QUERY)
        goto deviceCaps_fail;

    if (ioctl(pcmFile.handle(), SNDCTL_DSP_SETFMT, &format) < 0)
        goto deviceCaps_fail;

    int stereo;
    stereo = 1;

    if (ioctl(pcmFile.handle(), SNDCTL_DSP_STEREO, &stereo) < 0)
        goto deviceCaps_fail;

    static const QVector<int> preferredSampleRates{
        48000,
        44100,
        22050,
        11025,
        8000
    };

    int sampleRate;
    sampleRate = 0;

    for (int rate: preferredSampleRates)
        if (ioctl(pcmFile.handle(), SNDCTL_DSP_SPEED, &rate) >= 0){
            sampleRate = rate;

            break;
        }

    if (sampleRate < 1)
        goto deviceCaps_fail;

    int channels;
    channels = stereo? 2: 1;
    AkAudioCaps::SampleFormat sampleFormat;
    sampleFormat = sampleFormats->key(format,
                                      AkAudioCaps::SampleFormat_none);

    if (fragmentSize
        && device.endsWith(":Output")) {
        // Set the buffer to a maximum of 1024 samples.
        int bufferSize;
        bufferSize = BUFFER_SIZE
                     * channels
                     * AkAudioCaps::bitsPerSample(sampleFormat)
                     / 8;

        // Let's try setting the fragmet to just 2 pieces, and the half of the
        // buffer size, for low latency.
        int fragment;
        fragment = (2 << 16) | (bufferSize / 2);
        ioctl(pcmFile.handle(), SNDCTL_DSP_SETFRAGMENT, &fragment);

        // Let's see what OSS did actually set,
        audio_buf_info info;
        ioctl(pcmFile.handle(), SNDCTL_DSP_GETOSPACE, &info);

        *fragmentSize = info.fragsize > 0?
                            ((bufferSize / info.fragsize) << 16) | info.fragsize:
                            0;
    }

    pcmFile.close();

    {
        AkAudioCaps audioCaps;
        audioCaps.isValid() = true;
        audioCaps.format() = sampleFormat;
        audioCaps.bps() = AkAudioCaps::bitsPerSample(audioCaps.format());
        audioCaps.channels() = channels;
        audioCaps.rate() = int(sampleRate);
        audioCaps.layout() = AkAudioCaps::defaultChannelLayout(audioCaps.channels());
        audioCaps.align() = false;

        return audioCaps;
    }

deviceCaps_fail:
    pcmFile.close();

    return AkAudioCaps();
}

void AudioDevOSS::updateDevices()
{
    QStringList inputs;
    QStringList outputs;
    QMap<QString, AkAudioCaps> pinCapsMap;
    QMap<QString, QString> pinDescriptionMap;
    QMap<QString, int> fragmentSizeMap;

    QDir devicesDir("/dev");

    QStringList devices = devicesDir.entryList(QStringList() << "mixer*",
                                               QDir::System
                                               | QDir::Readable
                                               | QDir::Writable
                                               | QDir::NoSymLinks
                                               | QDir::NoDotAndDotDot
                                               | QDir::CaseSensitive,
                                               QDir::Name);

    for (const auto &devicePath: devices) {
        auto mixerDevice = devicesDir.absoluteFilePath(devicePath);
        auto dspDevice = QString(mixerDevice).replace("mixer", "dsp");

        if (!QFile::exists(mixerDevice)
            || !QFile::exists(dspDevice))
            continue;

        QString description;
        QFile mixerFile(mixerDevice);

        if (!mixerFile.open(QIODevice::ReadWrite))
            continue;

        mixer_info mixerInfo;

        if (ioctl(mixerFile.handle(), SOUND_MIXER_INFO, &mixerInfo) < 0) {
            mixerFile.close();

            continue;
        }

        mixerFile.close();
        description = QString("%1, %2").arg(mixerInfo.id).arg(mixerInfo.name);

        auto input = dspDevice + ":Input";
        int fragmentSize = 0;
        auto caps = this->deviceCaps(input, &fragmentSize);

        if (!caps) {
            fragmentSize = this->m_fragmentSizeMap.value(input);
            caps = this->m_pinCapsMap.value(input);
        }

        if (caps) {
            inputs << input;
            pinDescriptionMap[input] = description;
            fragmentSizeMap[input] = fragmentSize;
            pinCapsMap[input] = caps;
        }

        auto output = dspDevice + ":Output";
        caps = this->deviceCaps(output, &fragmentSize);

        if (!caps) {
            fragmentSize = this->m_fragmentSizeMap.value(output);
            caps = this->m_pinCapsMap.value(output);
        }

        if (caps) {
            outputs << output;
            pinDescriptionMap[output] = description;
            fragmentSizeMap[output] = fragmentSize;
            pinCapsMap[output] = caps;
        }
    }

    if (this->m_pinCapsMap != pinCapsMap)
        this->m_pinCapsMap = pinCapsMap;

    if (this->m_pinDescriptionMap != pinDescriptionMap)
        this->m_pinDescriptionMap = pinDescriptionMap;

    if (this->m_fragmentSizeMap != fragmentSizeMap)
        this->m_fragmentSizeMap = fragmentSizeMap;

    if (this->m_sources != inputs) {
        this->m_sources = inputs;
        emit this->inputsChanged(inputs);
    }

    if (this->m_sinks != outputs) {
        this->m_sinks = outputs;
        emit this->outputsChanged(outputs);
    }

    QString defaultOutput = outputs.isEmpty()? "": outputs.first();
    QString defaultInput = inputs.isEmpty()? "": inputs.first();

    if (this->m_defaultSource != defaultInput) {
        this->m_defaultSource = defaultInput;
        emit this->defaultInputChanged(defaultInput);
    }

    if (this->m_defaultSink != defaultOutput) {
        this->m_defaultSink = defaultOutput;
        emit this->defaultOutputChanged(defaultOutput);
    }
}
