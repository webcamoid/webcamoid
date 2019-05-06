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

#include <sys/ioctl.h>
#include <fcntl.h>
#include <QMap>
#include <QVector>
#include <QDir>
#include <QMutex>
#include <QFileSystemWatcher>
#include <akaudiopacket.h>

#ifdef HAVE_OSS_LINUX
#include <linux/soundcard.h>
#else
#include <sys/soundcard.h>
#endif

#include "audiodevoss.h"

#define BUFFER_SIZE 1024 // In samples

using SampleFormatMap = QMap<AkAudioCaps::SampleFormat, int>;

inline SampleFormatMap initSampleFormatMap()
{
    SampleFormatMap sampleFormat = {
        {AkAudioCaps::SampleFormat_s8   , AFMT_S8    },
        {AkAudioCaps::SampleFormat_u8   , AFMT_U8    },
        {AkAudioCaps::SampleFormat_s16  , AFMT_S16_NE},
        {AkAudioCaps::SampleFormat_s16le, AFMT_S16_LE},
        {AkAudioCaps::SampleFormat_s16be, AFMT_S16_BE},
        {AkAudioCaps::SampleFormat_u16le, AFMT_U16_LE},
        {AkAudioCaps::SampleFormat_u16be, AFMT_U16_BE},
    };

    return sampleFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatMap, sampleFormats, (initSampleFormatMap()))

class AudioDevOSSPrivate
{
    public:
        AudioDevOSS *self;
        QString m_error;
        QString m_defaultSink;
        QString m_defaultSource;
        QStringList m_sinks;
        QStringList m_sources;
        QMap<QString, QString> m_pinDescriptionMap;
        QMap<QString, QList<AkAudioCaps::SampleFormat>> m_supportedFormats;
        QMap<QString, QList<int>> m_supportedChannels;
        QMap<QString, QList<int>> m_supportedSampleRates;
        AkAudioCaps m_curCaps;
        QFile m_deviceFile;
        QFileSystemWatcher *m_fsWatcher {nullptr};
        QMutex m_mutex;

        explicit AudioDevOSSPrivate(AudioDevOSS *self);
        int fragmentSize(const QString &device, const AkAudioCaps &caps);
        void fillDeviceInfo(const QString &device,
                            QList<AkAudioCaps::SampleFormat> *supportedFormats,
                            QList<int> *supportedChannels,
                            QList<int> *supportedSampleRates) const;
};

AudioDevOSS::AudioDevOSS(QObject *parent):
    AudioDev(parent)
{
    this->d = new AudioDevOSSPrivate(this);
    this->d->m_fsWatcher = new QFileSystemWatcher({"/dev"}, this);

    QObject::connect(this->d->m_fsWatcher,
                     &QFileSystemWatcher::directoryChanged,
                     this,
                     &AudioDevOSS::updateDevices);

    this->updateDevices();
}

AudioDevOSS::~AudioDevOSS()
{
    this->uninit();

    if (this->d->m_fsWatcher)
        delete this->d->m_fsWatcher;

    delete this->d;
}

QString AudioDevOSS::error() const
{
    return this->d->m_error;
}

QString AudioDevOSS::defaultInput()
{
    return this->d->m_defaultSource;
}

QString AudioDevOSS::defaultOutput()
{
    return this->d->m_defaultSink;
}

QStringList AudioDevOSS::inputs()
{
    return this->d->m_sources;
}

QStringList AudioDevOSS::outputs()
{
    return this->d->m_sinks;
}

QString AudioDevOSS::description(const QString &device)
{
    return this->d->m_pinDescriptionMap.value(device);
}

AkAudioCaps AudioDevOSS::preferredFormat(const QString &device)
{
    return this->d->m_sinks.contains(device)?
                AkAudioCaps(AkAudioCaps::SampleFormat_s16,
                            AkAudioCaps::Layout_stereo,
                            44100):
                AkAudioCaps(AkAudioCaps::SampleFormat_u8,
                            AkAudioCaps::Layout_mono,
                            8000);
}

QList<AkAudioCaps::SampleFormat> AudioDevOSS::supportedFormats(const QString &device)
{
    return this->d->m_supportedFormats.value(device);
}

QList<int> AudioDevOSS::supportedChannels(const QString &device)
{
    return this->d->m_supportedChannels.value(device);
}

QList<int> AudioDevOSS::supportedSampleRates(const QString &device)
{
    return this->d->m_supportedSampleRates.value(device);
}

bool AudioDevOSS::init(const QString &device, const AkAudioCaps &caps)
{
    QMutexLocker mutexLockeer(&this->d->m_mutex);

    int fragmentSize = this->d->fragmentSize(device, caps);

    if (fragmentSize < 1)
        return false;

    this->d->m_deviceFile.setFileName(QString(device)
                                      .remove(QRegExp(":Input$|:Output$")));

    if (!this->d->m_deviceFile.open(device.endsWith(":Input")?
                                    QIODevice::ReadOnly: QIODevice::WriteOnly))
        return false;

    int format;
    format = sampleFormats->value(caps.format(), AFMT_QUERY);

    if (ioctl(this->d->m_deviceFile.handle(), SNDCTL_DSP_SETFMT, &format) < 0)
        goto init_fail;

    int stereo;
    stereo = caps.channels() > 1? 1: 0;

    if (ioctl(this->d->m_deviceFile.handle(), SNDCTL_DSP_STEREO, &stereo) < 0)
        goto init_fail;

    int sampleRate;
    sampleRate = caps.rate();

    if (ioctl(this->d->m_deviceFile.handle(), SNDCTL_DSP_SPEED, &sampleRate) < 0)
        goto init_fail;

    if (device.endsWith(":Output"))
        ioctl(this->d->m_deviceFile.handle(), SNDCTL_DSP_SETFRAGMENT, &fragmentSize);

    this->d->m_curCaps = caps;

    return true;

init_fail:
    this->d->m_deviceFile.close();

    return false;
}

QByteArray AudioDevOSS::read(int samples)
{
    if (samples < 1)
        return {};

    QMutexLocker mutexLockeer(&this->d->m_mutex);

    if (!this->d->m_deviceFile.isOpen())
        return QByteArray();

    QByteArray buffer;
    int bufferSize = samples
                     * this->d->m_curCaps.channels()
                     * AkAudioCaps::bitsPerSample(this->d->m_curCaps.format())
                     / 8;

    while (bufferSize > 0) {
        auto data = this->d->m_deviceFile.read(bufferSize);

        if (data.size() > 0) {
            buffer += data;
            bufferSize -= data.size();
        }
    }

    return buffer;
}

bool AudioDevOSS::write(const AkAudioPacket &packet)
{
    QMutexLocker mutexLockeer(&this->d->m_mutex);

    if (!this->d->m_deviceFile.isOpen())
        return false;

    return this->d->m_deviceFile.write(packet.buffer()) > 0;
}

bool AudioDevOSS::uninit()
{
    QMutexLocker mutexLockeer(&this->d->m_mutex);

    this->d->m_deviceFile.close();
    this->d->m_curCaps = AkAudioCaps();

    return true;
}

AudioDevOSSPrivate::AudioDevOSSPrivate(AudioDevOSS *self):
    self(self)
{
}

int AudioDevOSSPrivate::fragmentSize(const QString &device,
                                     const AkAudioCaps &caps)
{
    if (!device.endsWith(":Output"))
        return 0;

    QFile deviceFile;

    deviceFile.setFileName(QString(device).remove(":Output"));

    if (!deviceFile.open(QIODevice::WriteOnly))
        return 0;

    int format = sampleFormats->value(caps.format(), AFMT_QUERY);

    if (ioctl(deviceFile.handle(), SNDCTL_DSP_SETFMT, &format) < 0) {
        deviceFile.close();

        return 0;
    }

    int stereo;
    stereo = caps.channels() > 1? 1: 0;

    if (ioctl(deviceFile.handle(), SNDCTL_DSP_STEREO, &stereo) < 0) {
        deviceFile.close();

        return 0;
    }

    int sampleRate;
    sampleRate = caps.rate();

    if (ioctl(deviceFile.handle(), SNDCTL_DSP_SPEED, &sampleRate) < 0) {
        deviceFile.close();

        return 0;
    }

    // Set the buffer to a maximum of 1024 samples.
    int bufferSize =
            BUFFER_SIZE
            * caps.channels()
            * AkAudioCaps::bitsPerSample(caps.format())
            / 8;

    // Let's try setting the fragmet to just 2 pieces, and the half of the
    // buffer size, for low latency.
    int fragmentSize = (2 << 16) | (bufferSize / 2);
    ioctl(deviceFile.handle(), SNDCTL_DSP_SETFRAGMENT, &fragmentSize);

    // Let's see what OSS did actually set,
    audio_buf_info info;
    ioctl(deviceFile.handle(), SNDCTL_DSP_GETOSPACE, &info);

    fragmentSize = info.fragsize > 0?
                       ((bufferSize / info.fragsize) << 16) | info.fragsize:
                       0;
    deviceFile.close();

    return fragmentSize;
}

void AudioDevOSSPrivate::fillDeviceInfo(const QString &device,
                                        QList<AkAudioCaps::SampleFormat> *supportedFormats,
                                        QList<int> *supportedChannels,
                                        QList<int> *supportedSampleRates) const
{
    QFile pcmFile(QString(device)
                    .remove(QRegExp(":Input$|:Output$")));

    if (!pcmFile.open(device.endsWith(":Input")?
                      QIODevice::ReadOnly: QIODevice::WriteOnly))
        return;

    int formats = AFMT_QUERY;

    if (ioctl(pcmFile.handle(), SNDCTL_DSP_GETFMTS, &formats) < 0)
        goto deviceCaps_fail;

    static const QVector<int> preferredFormats = {
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
            if (format == AFMT_QUERY)
                format = fmt;

            supportedFormats->append(sampleFormats->key(fmt));
        }

    if (format == AFMT_QUERY)
        goto deviceCaps_fail;

    if (ioctl(pcmFile.handle(), SNDCTL_DSP_SETFMT, &format) < 0)
        goto deviceCaps_fail;

    for (int channels = 0; channels < 2; channels++)
        if (ioctl(pcmFile.handle(), SNDCTL_DSP_STEREO, &channels) >= 0)
            supportedChannels->append(channels + 1);

    for (auto &rate: self->commonSampleRates())
        if (ioctl(pcmFile.handle(), SNDCTL_DSP_SPEED, &rate) >= 0)
            supportedSampleRates->append(rate);

deviceCaps_fail:
    pcmFile.close();
}

void AudioDevOSS::updateDevices()
{
    decltype(this->d->m_sources) inputs;
    decltype(this->d->m_sinks) outputs;
    decltype(this->d->m_pinDescriptionMap) pinDescriptionMap;
    decltype(this->d->m_supportedFormats) supportedFormats;
    decltype(this->d->m_supportedChannels) supportedChannels;
    decltype(this->d->m_supportedSampleRates) supportedSampleRates;

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
        description = QString("%1, %2").arg(mixerInfo.id, mixerInfo.name);

        QList<AkAudioCaps::SampleFormat> _supportedFormats;
        QList<int> _supportedChannels;
        QList<int> _supportedSampleRates;

        auto input = dspDevice + ":Input";
        this->d->fillDeviceInfo(input,
                                &_supportedFormats,
                                &_supportedChannels,
                                &_supportedSampleRates);

        if (_supportedFormats.isEmpty())
            _supportedFormats = this->d->m_supportedFormats.value(input);

        if (_supportedChannels.isEmpty())
            _supportedChannels = this->d->m_supportedChannels.value(input);

        if (_supportedSampleRates.isEmpty())
            _supportedSampleRates = this->d->m_supportedSampleRates.value(input);

        if (!_supportedFormats.isEmpty()
            && !_supportedChannels.isEmpty()
            && !_supportedSampleRates.isEmpty()) {
            inputs << input;
            pinDescriptionMap[input] = description;
            supportedFormats[input] = _supportedFormats;
            supportedChannels[input] = _supportedChannels;
            supportedSampleRates[input] = _supportedSampleRates;
        }

        _supportedFormats.clear();
        _supportedChannels.clear();
        _supportedSampleRates.clear();

        auto output = dspDevice + ":Output";
        this->d->fillDeviceInfo(output,
                                &_supportedFormats,
                                &_supportedChannels,
                                &_supportedSampleRates);

        if (_supportedFormats.isEmpty())
            _supportedFormats = this->d->m_supportedFormats.value(output);

        if (_supportedChannels.isEmpty())
            _supportedChannels = this->d->m_supportedChannels.value(output);

        if (_supportedSampleRates.isEmpty())
            _supportedSampleRates = this->d->m_supportedSampleRates.value(output);

        if (!_supportedFormats.isEmpty()
            && !_supportedChannels.isEmpty()
            && !_supportedSampleRates.isEmpty()) {
            outputs << output;
            pinDescriptionMap[output] = description;
            supportedFormats[output] = _supportedFormats;
            supportedChannels[output] = _supportedChannels;
            supportedSampleRates[output] = _supportedSampleRates;
        }
    }

    if (this->d->m_supportedFormats != supportedFormats)
        this->d->m_supportedFormats = supportedFormats;

    if (this->d->m_supportedChannels != supportedChannels)
        this->d->m_supportedChannels = supportedChannels;

    if (this->d->m_supportedSampleRates != supportedSampleRates)
        this->d->m_supportedSampleRates = supportedSampleRates;

    if (this->d->m_pinDescriptionMap != pinDescriptionMap)
        this->d->m_pinDescriptionMap = pinDescriptionMap;

    if (this->d->m_sources != inputs) {
        this->d->m_sources = inputs;
        emit this->inputsChanged(inputs);
    }

    if (this->d->m_sinks != outputs) {
        this->d->m_sinks = outputs;
        emit this->outputsChanged(outputs);
    }

    QString defaultOutput = outputs.isEmpty()? "": outputs.first();
    QString defaultInput = inputs.isEmpty()? "": inputs.first();

    if (this->d->m_defaultSource != defaultInput) {
        this->d->m_defaultSource = defaultInput;
        emit this->defaultInputChanged(defaultInput);
    }

    if (this->d->m_defaultSink != defaultOutput) {
        this->d->m_defaultSink = defaultOutput;
        emit this->defaultOutputChanged(defaultOutput);
    }
}

#include "moc_audiodevoss.cpp"
