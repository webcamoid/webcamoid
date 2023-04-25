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

#include <cstdarg>
#include <QFileSystemWatcher>
#include <QMap>
#include <QMutex>
#include <QTimer>
#include <QVector>
#include <alsa/asoundlib.h>
#include <alsa/error.h>
#include <akaudiopacket.h>

#include "audiodevalsa.h"

using SampleFormatMap = QMap<AkAudioCaps::SampleFormat, snd_pcm_format_t>;

inline const SampleFormatMap &sampleFormats()
{
    static const SampleFormatMap sampleFormat {
        {AkAudioCaps::SampleFormat_s8   , SND_PCM_FORMAT_S8        },
        {AkAudioCaps::SampleFormat_u8   , SND_PCM_FORMAT_U8        },
        {AkAudioCaps::SampleFormat_s16be, SND_PCM_FORMAT_S16_BE    },
        {AkAudioCaps::SampleFormat_s16le, SND_PCM_FORMAT_S16_LE    },
        {AkAudioCaps::SampleFormat_u16be, SND_PCM_FORMAT_U16_BE    },
        {AkAudioCaps::SampleFormat_u16le, SND_PCM_FORMAT_U16_LE    },
        {AkAudioCaps::SampleFormat_s32be, SND_PCM_FORMAT_S32_BE    },
        {AkAudioCaps::SampleFormat_s32le, SND_PCM_FORMAT_S32_LE    },
        {AkAudioCaps::SampleFormat_u32be, SND_PCM_FORMAT_U32_BE    },
        {AkAudioCaps::SampleFormat_u32le, SND_PCM_FORMAT_U32_LE    },
        {AkAudioCaps::SampleFormat_fltbe, SND_PCM_FORMAT_FLOAT_BE  },
        {AkAudioCaps::SampleFormat_fltle, SND_PCM_FORMAT_FLOAT_LE  },
        {AkAudioCaps::SampleFormat_dblbe, SND_PCM_FORMAT_FLOAT64_BE},
        {AkAudioCaps::SampleFormat_dblle, SND_PCM_FORMAT_FLOAT64_LE},
    };

    return sampleFormat;
}

class AudioDevAlsaPrivate
{
    public:
        AudioDevAlsa *self;
        QString m_error;
        QString m_defaultSink;
        QString m_defaultSource;
        QStringList m_sinks;
        QStringList m_sources;
        QMap<QString, QString> m_pinDescriptionMap;
        QMap<QString, QList<AkAudioCaps::SampleFormat>> m_supportedFormats;
        QMap<QString, QList<AkAudioCaps::ChannelLayout>> m_supportedLayouts;
        QMap<QString, QList<int>> m_supportedSampleRates;
        snd_pcm_t *m_pcmHnd {nullptr};
        QFileSystemWatcher *m_fsWatcher {nullptr};
        QTimer m_timer;
        QMutex m_mutex;
        int m_samples {0};

        explicit AudioDevAlsaPrivate(AudioDevAlsa *self);
        QString deviceName(snd_ctl_t *ctlHnd,
                           unsigned int device,
                           snd_pcm_stream_t streamType) const;
        bool fillDeviceInfo(const QString &deviceId,
                            snd_pcm_stream_t streamType,
                            QList<AkAudioCaps::SampleFormat> *supportedFormats,
                            QList<AkAudioCaps::ChannelLayout> *supportedLayouts,
                            QList<int> *supportedSampleRates) const;
        void updateDevices();
};

AudioDevAlsa::AudioDevAlsa(QObject *parent):
    AudioDev(parent)
{
    this->d = new AudioDevAlsaPrivate(this);
    this->d->m_timer.setInterval(3000);

    QObject::connect(&this->d->m_timer,
                     &QTimer::timeout,
                     this,
                     [this] () {
                        this->d->updateDevices();
                     });

#if 1
    this->d->m_fsWatcher = new QFileSystemWatcher({"/dev/snd"}, this);

    QObject::connect(this->d->m_fsWatcher,
                     &QFileSystemWatcher::directoryChanged,
                     this,
                     [this] () {
                        this->d->updateDevices();
                     });

    this->d->updateDevices();
#else
    this->d->updateDevices();
    this->d->m_timer.start();
#endif
}

AudioDevAlsa::~AudioDevAlsa()
{
    this->uninit();

    if (this->d->m_fsWatcher)
        delete this->d->m_fsWatcher;

    delete this->d;
}

QString AudioDevAlsa::error() const
{
    return this->d->m_error;
}

QString AudioDevAlsa::defaultInput()
{
    return this->d->m_defaultSource;
}

QString AudioDevAlsa::defaultOutput()
{
    return this->d->m_defaultSink;
}

QStringList AudioDevAlsa::inputs()
{
    return this->d->m_sources;
}

QStringList AudioDevAlsa::outputs()
{
    return this->d->m_sinks;
}

QString AudioDevAlsa::description(const QString &device)
{
    return this->d->m_pinDescriptionMap.value(device);
}

AkAudioCaps AudioDevAlsa::preferredFormat(const QString &device)
{
    return this->d->m_sinks.contains(device)?
                AkAudioCaps(AkAudioCaps::SampleFormat_s32,
                            AkAudioCaps::Layout_stereo,
                            false,
                            44100):
                AkAudioCaps(AkAudioCaps::SampleFormat_u8,
                            AkAudioCaps::Layout_mono,
                            false,
                            8000);
}

QList<AkAudioCaps::SampleFormat> AudioDevAlsa::supportedFormats(const QString &device)
{
    return this->d->m_supportedFormats.value(device);
}

QList<AkAudioCaps::ChannelLayout> AudioDevAlsa::supportedChannelLayouts(const QString &device)
{
    return this->d->m_supportedLayouts.value(device);
}

QList<int> AudioDevAlsa::supportedSampleRates(const QString &device)
{
    return this->d->m_supportedSampleRates.value(device);
}

bool AudioDevAlsa::init(const QString &device, const AkAudioCaps &caps)
{
    this->d->m_mutex.lock();

    this->d->m_pcmHnd = nullptr;
    int error =
            snd_pcm_open(&this->d->m_pcmHnd,
                         QString(device)
                             .remove(QRegExp(":Input$|:Output$"))
                             .toStdString().c_str(),
                         device.endsWith(":Input")?
                             SND_PCM_STREAM_CAPTURE: SND_PCM_STREAM_PLAYBACK,
                         SND_PCM_NONBLOCK);

    if (error < 0) {
        snd_pcm_close(this->d->m_pcmHnd);
        this->d->m_pcmHnd = nullptr;
        this->d->m_mutex.unlock();

        this->d->m_error = snd_strerror(error);
        emit this->errorChanged(this->d->m_error);

        return false;
    }

    error = snd_pcm_set_params(this->d->m_pcmHnd,
                               sampleFormats().value(caps.format(),
                                                     SND_PCM_FORMAT_UNKNOWN),
                               SND_PCM_ACCESS_RW_INTERLEAVED,
                               uint(caps.channels()),
                               uint(caps.rate()),
                               1,
                               uint(1000 * this->latency()));

    if (error < 0) {
        snd_pcm_close(this->d->m_pcmHnd);
        this->d->m_pcmHnd = nullptr;
        this->d->m_mutex.unlock();

        this->d->m_error = snd_strerror(error);
        emit this->errorChanged(this->d->m_error);

        return false;
    }

    this->d->m_mutex.unlock();
    this->d->m_samples = qMax(this->latency() * caps.rate() / 1000, 1);

    return true;
}

QByteArray AudioDevAlsa::read()
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_pcmHnd)
        return {};

    int samples = this->d->m_samples;
    auto bufferSize = snd_pcm_frames_to_bytes(this->d->m_pcmHnd, samples);
    QByteArray buffer(int(bufferSize), 0);
    auto data = buffer.data();

    while (samples > 0) {
        auto rsamples = snd_pcm_readi(this->d->m_pcmHnd,
                                      data,
                                      snd_pcm_uframes_t(samples));

        if (rsamples >= 0) {
            auto dataRead = snd_pcm_frames_to_bytes(this->d->m_pcmHnd,
                                                    rsamples);
            data += dataRead;
            samples -= rsamples;
        } else {
            if (rsamples == -EAGAIN) {
                snd_pcm_wait(this->d->m_pcmHnd, 1000);

                continue;
            }

            return {};
        }
    }

    return buffer;
}

bool AudioDevAlsa::write(const AkAudioPacket &packet)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_pcmHnd)
        return false;

    auto data = packet.constData();
    int dataSize = packet.size();

    while (dataSize > 0) {
        auto samples = snd_pcm_bytes_to_frames(this->d->m_pcmHnd, dataSize);
        samples = snd_pcm_writei(this->d->m_pcmHnd,
                                 data,
                                 snd_pcm_uframes_t(samples));

        if (samples >= 0) {
            auto dataWritten = snd_pcm_frames_to_bytes(this->d->m_pcmHnd,
                                                       samples);
            data += dataWritten;
            dataSize -= dataWritten;
        } else {
            if (samples == -EAGAIN) {
                snd_pcm_wait(this->d->m_pcmHnd, 1000);

                continue;
            }

            samples = snd_pcm_recover(this->d->m_pcmHnd, int(samples), 0);

            if (samples < 0)
                return false;
        }
    }

    return true;
}

bool AudioDevAlsa::uninit()
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (this->d->m_pcmHnd) {
        snd_pcm_close(this->d->m_pcmHnd);
        this->d->m_pcmHnd = nullptr;
    }

    return true;
}

AudioDevAlsaPrivate::AudioDevAlsaPrivate(AudioDevAlsa *self):
    self(self)
{
}

QString AudioDevAlsaPrivate::deviceName(snd_ctl_t *ctlHnd,
                                        unsigned int device,
                                        snd_pcm_stream_t streamType) const
{
    QString deviceName;
    snd_pcm_info_t *deviceInfo = nullptr;
    snd_pcm_info_malloc(&deviceInfo);
    snd_pcm_info_set_device(deviceInfo, device);
    snd_pcm_info_set_subdevice(deviceInfo, 0);
    snd_pcm_info_set_stream(deviceInfo, streamType);

    if (snd_ctl_pcm_info(ctlHnd, deviceInfo) >= 0)
        deviceName = snd_pcm_info_get_name(deviceInfo);

    snd_pcm_info_free(deviceInfo);

    return deviceName;
}

bool AudioDevAlsaPrivate::fillDeviceInfo(const QString &deviceId,
                                         snd_pcm_stream_t streamType,
                                         QList<AkAudioCaps::SampleFormat> *supportedFormats,
                                         QList<AkAudioCaps::ChannelLayout> *supportedLayouts,
                                         QList<int> *supportedSampleRates) const
{
    snd_pcm_t *pcmHnd = nullptr;
    int error = snd_pcm_open(&pcmHnd,
                             deviceId.toStdString().c_str(),
                             streamType,
                             SND_PCM_NONBLOCK);

    if (error < 0)
        return false;

    uint maxChannels = 0;

    snd_pcm_hw_params_t *hwParams = nullptr;
    snd_pcm_hw_params_malloc(&hwParams);
    snd_pcm_hw_params_any(pcmHnd, hwParams);

    // Get params.

    if (snd_pcm_hw_params_test_access(pcmHnd,
                                      hwParams,
                                      SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
        snd_pcm_hw_params_free(hwParams);

        if (pcmHnd)
            snd_pcm_close(pcmHnd);

        return false;
    }

    static const QVector<snd_pcm_format_t> preferredFormats {
        SND_PCM_FORMAT_FLOAT,
        SND_PCM_FORMAT_S32,
        SND_PCM_FORMAT_U32,
        SND_PCM_FORMAT_S16,
        SND_PCM_FORMAT_U16,
        SND_PCM_FORMAT_S8,
        SND_PCM_FORMAT_U8
    };

    for (auto fmt: preferredFormats)
        if (snd_pcm_hw_params_test_format(pcmHnd, hwParams, fmt) >= 0) {
            auto format = sampleFormats().key(fmt);

            if (!supportedFormats->contains(format))
                supportedFormats->append(format);
        }

    std::sort(supportedFormats->begin(), supportedFormats->end());

    if (snd_pcm_hw_params_get_channels_max(hwParams, &maxChannels) < 0)
        maxChannels = 3;
    else
        maxChannels = qBound<uint>(1, maxChannels, 16) + 1;

    for (uint channels = 1; channels < maxChannels; channels++)
        if (snd_pcm_hw_params_test_channels(pcmHnd, hwParams, channels) >= 0) {
            auto layout = AkAudioCaps::defaultChannelLayout(int(channels));

            if (layout != AkAudioCaps::Layout_none)
                supportedLayouts->append(layout);
        }

    for (auto &rate: self->commonSampleRates())
        if (snd_pcm_hw_params_test_rate(pcmHnd, hwParams, uint(rate), 0) >= 0)
            supportedSampleRates->append(rate);

    snd_pcm_hw_params_free(hwParams);

    if (pcmHnd)
        snd_pcm_close(pcmHnd);

    return true;
}

void AudioDevAlsaPrivate::updateDevices()
{
    decltype(this->m_sources) inputs;
    decltype(this->m_sinks) outputs;
    decltype(this->m_pinDescriptionMap) pinDescriptionMap;
    decltype(this->m_supportedFormats) supportedFormats;
    decltype(this->m_supportedLayouts) supportedChannels;
    decltype(this->m_supportedSampleRates) supportedSampleRates;

    // Add hardware audio devices.

    // Read hardware devices first but put them after virtual devices.
    decltype(this->m_sources) hwInputs;
    decltype(this->m_sinks) hwOutputs;
    decltype(this->m_pinDescriptionMap) hwPinDescriptionMap;
    decltype(this->m_supportedFormats) hwSupportedFormats;
    decltype(this->m_supportedLayouts) hwSupportedChannels;
    decltype(this->m_supportedSampleRates) hwSupportedSampleRates;

    QStringList cardIds;
    QStringList deviceIds;
    int card = -1;

    while (snd_card_next(&card) >= 0 && card >= 0) {
        static const size_t nameSize = 32;
        char name[nameSize];
        snprintf(name, nameSize, "hw:%d", card);
        snd_ctl_t *ctlHnd = nullptr;

        if (snd_ctl_open(&ctlHnd, name, SND_PCM_NONBLOCK) < 0)
            continue;

        QString cardId;
        QString cardName;
        snd_ctl_card_info_t *cardInfo = nullptr;
        snd_ctl_card_info_malloc(&cardInfo);

        if (snd_ctl_card_info(ctlHnd, cardInfo) >= 0) {
            cardId = snd_ctl_card_info_get_id(cardInfo);
            cardName = snd_ctl_card_info_get_name(cardInfo);
        }

        snd_ctl_card_info_free(cardInfo);

        if (cardId.isEmpty() || cardName.isEmpty()) {
            snd_ctl_close(ctlHnd);

            continue;
        }

        auto cid = QString("CARD=%1").arg(cardId);

        if (!cardIds.contains(cid))
            cardIds << cid;

        cid = QString("CARD=%1").arg(card);

        if (!cardIds.contains(cid))
            cardIds << cid;

        int device = -1;

        while (snd_ctl_pcm_next_device(ctlHnd, &device) >= 0 && device >= 0) {
            auto did = QString("CARD=%1,DEV=%2").arg(cardId).arg(device);

            if (!deviceIds.contains(did))
                deviceIds << did;

            auto deviceId =
                    QString("plughw:CARD=%1,DEV=%2").arg(cardId).arg(device);
            QList<AkAudioCaps::SampleFormat> _supportedFormats;
            QList<AkAudioCaps::ChannelLayout> _supportedLayouts;
            QList<int> _supportedSampleRates;

            this->fillDeviceInfo(deviceId,
                                 SND_PCM_STREAM_CAPTURE,
                                 &_supportedFormats,
                                 &_supportedLayouts,
                                 &_supportedSampleRates);
            auto input = deviceId + ":Input";

            if (_supportedFormats.isEmpty())
                _supportedFormats = this->m_supportedFormats.value(input);

            if (_supportedLayouts.isEmpty())
                _supportedLayouts = this->m_supportedLayouts.value(input);

            if (_supportedSampleRates.isEmpty())
                _supportedSampleRates = this->m_supportedSampleRates.value(input);

            if (!_supportedFormats.isEmpty()
                && !_supportedLayouts.isEmpty()
                && !_supportedSampleRates.isEmpty()) {
                hwInputs << input;
                auto deviceName = this->deviceName(ctlHnd,
                                                   device,
                                                   SND_PCM_STREAM_CAPTURE);
                hwPinDescriptionMap[input] =
                        QString("%1 - %2").arg(cardName, deviceName);
                hwSupportedFormats[input] = _supportedFormats;
                hwSupportedChannels[input] = _supportedLayouts;
                hwSupportedSampleRates[input] = _supportedSampleRates;
            }

            _supportedFormats.clear();
            _supportedLayouts.clear();
            _supportedSampleRates.clear();

            this->fillDeviceInfo(deviceId,
                                 SND_PCM_STREAM_PLAYBACK,
                                 &_supportedFormats,
                                 &_supportedLayouts,
                                 &_supportedSampleRates);
            auto output = deviceId + ":Output";

            if (_supportedFormats.isEmpty())
                _supportedFormats = this->m_supportedFormats.value(output);

            if (_supportedLayouts.isEmpty())
                _supportedLayouts = this->m_supportedLayouts.value(output);

            if (_supportedSampleRates.isEmpty())
                _supportedSampleRates = this->m_supportedSampleRates.value(output);

            if (!_supportedFormats.isEmpty()
                && !_supportedLayouts.isEmpty()
                && !_supportedSampleRates.isEmpty()) {
                hwOutputs << output;
                auto deviceName = this->deviceName(ctlHnd,
                                                   device,
                                                   SND_PCM_STREAM_PLAYBACK);
                hwPinDescriptionMap[output] =
                        QString("%1 - %2").arg(cardName, deviceName);
                hwSupportedFormats[output] = _supportedFormats;
                hwSupportedChannels[output] = _supportedLayouts;
                hwSupportedSampleRates[output] = _supportedSampleRates;
            }
        }

        snd_ctl_close(ctlHnd);
    }

    // Add virtual audio devices.

    void **hints = nullptr;

    if (snd_device_name_hint(-1, "pcm", &hints) >= 0) {
        for (auto hint = hints; *hint != nullptr; hint++) {
            QString deviceId = snd_device_name_get_hint(*hint, "NAME");

            if (deviceId.isEmpty() || deviceId == "null")
                continue;

            QString description = snd_device_name_get_hint(*hint, "DESC");
            description.replace('\n', " - ");
            QString io = snd_device_name_get_hint(*hint, "IOID");
            auto interfaceDevice = deviceId.split(":");

            if (interfaceDevice.size() > 1
                && (cardIds.contains(interfaceDevice[1])
                    || deviceIds.contains(interfaceDevice[1]))) {
                continue;
            }

            QList<AkAudioCaps::SampleFormat> _supportedFormats;
            QList<AkAudioCaps::ChannelLayout> _supportedLayouts;
            QList<int> _supportedSampleRates;

            if (io.isEmpty() || io == "Input") {
                this->fillDeviceInfo(deviceId,
                                     SND_PCM_STREAM_CAPTURE,
                                     &_supportedFormats,
                                     &_supportedLayouts,
                                     &_supportedSampleRates);
                auto input = deviceId + ":Input";

                if (_supportedFormats.isEmpty())
                    _supportedFormats = this->m_supportedFormats.value(input);

                if (_supportedLayouts.isEmpty())
                    _supportedLayouts = this->m_supportedLayouts.value(input);

                if (_supportedSampleRates.isEmpty())
                    _supportedSampleRates = this->m_supportedSampleRates.value(input);

                if (!_supportedFormats.isEmpty()
                    && !_supportedLayouts.isEmpty()
                    && !_supportedSampleRates.isEmpty()) {
                    inputs << input;
                    pinDescriptionMap[input] = description;
                    supportedFormats[input] = _supportedFormats;
                    supportedChannels[input] = _supportedLayouts;
                    supportedSampleRates[input] = _supportedSampleRates;
                }
            }

            _supportedFormats.clear();
            _supportedLayouts.clear();
            _supportedSampleRates.clear();

            if (io.isEmpty() || io == "Output") {
                this->fillDeviceInfo(deviceId,
                                     SND_PCM_STREAM_PLAYBACK,
                                     &_supportedFormats,
                                     &_supportedLayouts,
                                     &_supportedSampleRates);
                auto output = deviceId + ":Output";

                if (_supportedFormats.isEmpty())
                    _supportedFormats = this->m_supportedFormats.value(output);

                if (_supportedLayouts.isEmpty())
                    _supportedLayouts = this->m_supportedLayouts.value(output);

                if (_supportedSampleRates.isEmpty())
                    _supportedSampleRates = this->m_supportedSampleRates.value(output);

                if (!_supportedFormats.isEmpty()
                    && !_supportedLayouts.isEmpty()
                    && !_supportedSampleRates.isEmpty()) {
                    outputs << output;
                    pinDescriptionMap[output] = description;
                    supportedFormats[output] = _supportedFormats;
                    supportedChannels[output] = _supportedLayouts;
                    supportedSampleRates[output] = _supportedSampleRates;
                }
            }
        }

        snd_device_name_free_hint(hints);
    }

    // Join virtual devices and hardware devices;
    inputs << hwInputs;
    outputs << hwOutputs;
    auto devices = hwInputs + hwOutputs;

    for (auto &device: devices) {
        pinDescriptionMap[device] = hwPinDescriptionMap[device];
        supportedFormats[device] = hwSupportedFormats[device];
        supportedChannels[device] = hwSupportedChannels[device];
        supportedSampleRates[device] = hwSupportedSampleRates[device];
    }

    // Update devices
    if (this->m_supportedFormats != supportedFormats)
        this->m_supportedFormats = supportedFormats;

    if (this->m_supportedLayouts != supportedChannels)
        this->m_supportedLayouts = supportedChannels;

    if (this->m_supportedSampleRates != supportedSampleRates)
        this->m_supportedSampleRates = supportedSampleRates;

    if (this->m_pinDescriptionMap != pinDescriptionMap)
        this->m_pinDescriptionMap = pinDescriptionMap;

    if (this->m_sources != inputs) {
        this->m_sources = inputs;
        emit self->inputsChanged(inputs);
    }

    if (this->m_sinks != outputs) {
        this->m_sinks = outputs;
        emit self->outputsChanged(outputs);
    }

    QString defaultOutput = outputs.isEmpty()? "": outputs.first();
    QString defaultInput = inputs.isEmpty()? "": inputs.first();

    if (this->m_defaultSource != defaultInput) {
        this->m_defaultSource = defaultInput;
        emit self->defaultInputChanged(defaultInput);
    }

    if (this->m_defaultSink != defaultOutput) {
        this->m_defaultSink = defaultOutput;
        emit self->defaultOutputChanged(defaultOutput);
    }
}

#include "moc_audiodevalsa.cpp"
