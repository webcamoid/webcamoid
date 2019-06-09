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
#include <QMap>
#include <QVector>
#include <QTimer>
#include <QMutex>
#include <QFileSystemWatcher>
#include <alsa/asoundlib.h>
#include <alsa/error.h>
#include <akaudiopacket.h>

#include "audiodevalsa.h"

using SampleFormatMap = QMap<AkAudioCaps::SampleFormat, snd_pcm_format_t>;

inline const SampleFormatMap &sampleFormats()
{
    static const SampleFormatMap sampleFormat {
        {AkAudioCaps::SampleFormat_s8 , SND_PCM_FORMAT_S8     },
        {AkAudioCaps::SampleFormat_u8 , SND_PCM_FORMAT_U8     },
        {AkAudioCaps::SampleFormat_s16, SND_PCM_FORMAT_S16    },
        {AkAudioCaps::SampleFormat_u16, SND_PCM_FORMAT_U16    },
        {AkAudioCaps::SampleFormat_s32, SND_PCM_FORMAT_S32    },
        {AkAudioCaps::SampleFormat_u32, SND_PCM_FORMAT_U32    },
        {AkAudioCaps::SampleFormat_flt, SND_PCM_FORMAT_FLOAT  },
        {AkAudioCaps::SampleFormat_dbl, SND_PCM_FORMAT_FLOAT64},
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

        explicit AudioDevAlsaPrivate(AudioDevAlsa *self);
        void fillDeviceInfo(const QString &device,
                            QList<AkAudioCaps::SampleFormat> *supportedFormats,
                            QList<AkAudioCaps::ChannelLayout> *supportedLayouts,
                            QList<int> *supportedSampleRates) const;
};

AudioDevAlsa::AudioDevAlsa(QObject *parent):
    AudioDev(parent)
{
    this->d = new AudioDevAlsaPrivate(this);
    this->d->m_timer.setInterval(3000);

    QObject::connect(&this->d->m_timer,
                     &QTimer::timeout,
                     this,
                     &AudioDevAlsa::updateDevices);

#if 1
    this->d->m_fsWatcher = new QFileSystemWatcher({"/dev/snd"}, this);

    QObject::connect(this->d->m_fsWatcher,
                     &QFileSystemWatcher::directoryChanged,
                     this,
                     &AudioDevAlsa::updateDevices);

    this->updateDevices();
#else
    this->updateDevices();
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
                            44100):
                AkAudioCaps(AkAudioCaps::SampleFormat_u8,
                            AkAudioCaps::Layout_mono,
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
    QMutexLocker mutexLockeer(&this->d->m_mutex);

    this->d->m_pcmHnd = nullptr;
    int error =
            snd_pcm_open(&this->d->m_pcmHnd,
                         QString(device)
                             .remove(QRegExp(":Input$|:Output$"))
                             .toStdString().c_str(),
                         device.endsWith(":Input")?
                             SND_PCM_STREAM_CAPTURE: SND_PCM_STREAM_PLAYBACK,
                         SND_PCM_NONBLOCK);

    if (error < 0)
        goto init_fail;

    error = snd_pcm_set_params(this->d->m_pcmHnd,
                               sampleFormats().value(caps.format(),
                                                     SND_PCM_FORMAT_UNKNOWN),
                               SND_PCM_ACCESS_RW_INTERLEAVED,
                               uint(caps.channels()),
                               uint(caps.rate()),
                               1,
                               uint(1000 * this->latency()));

    if (error < 0)
        goto init_fail;

    return true;

init_fail:
    this->d->m_error = snd_strerror(error);
    emit this->errorChanged(this->d->m_error);
    this->uninit();

    return false;
}

QByteArray AudioDevAlsa::read(int samples)
{
    if (samples < 1)
        return {};

    QMutexLocker mutexLockeer(&this->d->m_mutex);

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
    QMutexLocker mutexLockeer(&this->d->m_mutex);

    if (!this->d->m_pcmHnd)
        return false;

    auto buffer = packet.buffer();
    auto data = buffer.constData();
    int dataSize = buffer.size();

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

void AudioDevAlsaPrivate::fillDeviceInfo(const QString &device,
                                         QList<AkAudioCaps::SampleFormat> *supportedFormats,
                                         QList<AkAudioCaps::ChannelLayout> *supportedLayouts,
                                         QList<int> *supportedSampleRates) const
{
    snd_pcm_t *pcmHnd = nullptr;
    int error = snd_pcm_open(&pcmHnd,
                             QString(device)
                                 .remove(QRegExp(":Input$|:Output$"))
                                 .toStdString().c_str(),
                             device.endsWith(":Input")?
                                 SND_PCM_STREAM_CAPTURE: SND_PCM_STREAM_PLAYBACK,
                             SND_PCM_NONBLOCK);

    if (error < 0)
        return;

    uint maxChannels = 0;

    snd_pcm_hw_params_t *hwParams = nullptr;
    snd_pcm_hw_params_malloc(&hwParams);
    snd_pcm_hw_params_any(pcmHnd, hwParams);

    // Get params.

    if (snd_pcm_hw_params_test_access(pcmHnd,
                                      hwParams,
                                      SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
        goto deviceCaps_fail;

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

deviceCaps_fail:
    snd_pcm_hw_params_free(hwParams);

    if (pcmHnd)
        snd_pcm_close(pcmHnd);
}

void AudioDevAlsa::updateDevices()
{
    decltype(this->d->m_sources) inputs;
    decltype(this->d->m_sinks) outputs;
    decltype(this->d->m_pinDescriptionMap) pinDescriptionMap;
    decltype(this->d->m_supportedFormats) supportedFormats;
    decltype(this->d->m_supportedLayouts) supportedChannels;
    decltype(this->d->m_supportedSampleRates) supportedSampleRates;

    int card = -1;
    snd_ctl_card_info_t *ctlInfo = nullptr;
    snd_ctl_card_info_malloc(&ctlInfo);

    while (snd_card_next(&card) >= 0 && card >= 0) {
        char name[32];
        sprintf(name, "hw:%d", card);
        snd_ctl_t *ctlHnd = nullptr;

        if (snd_ctl_open(&ctlHnd, name, SND_PCM_NONBLOCK) < 0)
            continue;

        if (snd_ctl_card_info(ctlHnd, ctlInfo) < 0) {
            snd_ctl_close(ctlHnd);

            continue;
        }

        int device = -1;

        if (snd_ctl_pcm_next_device(ctlHnd, &device) < 0
            || device < 0) {
            snd_ctl_close(ctlHnd);

            continue;
        }

        QString deviceId =
                QString("plughw:CARD=%1,DEV=0")
                    .arg(snd_ctl_card_info_get_id(ctlInfo));
        QString description = snd_ctl_card_info_get_name(ctlInfo);

        snd_ctl_close(ctlHnd);

        QList<AkAudioCaps::SampleFormat> _supportedFormats;
        QList<AkAudioCaps::ChannelLayout> _supportedLayouts;
        QList<int> _supportedSampleRates;

        auto input = deviceId + ":Input";
        this->d->fillDeviceInfo(input,
                                &_supportedFormats,
                                &_supportedLayouts,
                                &_supportedSampleRates);

        if (_supportedFormats.isEmpty())
            _supportedFormats = this->d->m_supportedFormats.value(input);

        if (_supportedLayouts.isEmpty())
            _supportedLayouts = this->d->m_supportedLayouts.value(input);

        if (_supportedSampleRates.isEmpty())
            _supportedSampleRates = this->d->m_supportedSampleRates.value(input);

        if (!_supportedFormats.isEmpty()
            && !_supportedLayouts.isEmpty()
            && !_supportedSampleRates.isEmpty()) {
            inputs << input;
            pinDescriptionMap[input] = description;
            supportedFormats[input] = _supportedFormats;
            supportedChannels[input] = _supportedLayouts;
            supportedSampleRates[input] = _supportedSampleRates;
        }

        _supportedFormats.clear();
        _supportedLayouts.clear();
        _supportedSampleRates.clear();

        auto output = deviceId + ":Output";
        this->d->fillDeviceInfo(output,
                                &_supportedFormats,
                                &_supportedLayouts,
                                &_supportedSampleRates);

        if (_supportedFormats.isEmpty())
            _supportedFormats = this->d->m_supportedFormats.value(output);

        if (_supportedLayouts.isEmpty())
            _supportedLayouts = this->d->m_supportedLayouts.value(output);

        if (_supportedSampleRates.isEmpty())
            _supportedSampleRates = this->d->m_supportedSampleRates.value(output);

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

    snd_ctl_card_info_free(ctlInfo);

    // In case the first method for detecting the devices didn't worked,
    // use hints to detect the devices.
    void **hints = nullptr;
    bool fillInputs = inputs.isEmpty();
    bool fillOuputs = outputs.isEmpty();

    if (snd_device_name_hint(-1, "pcm", &hints) >= 0) {
        for (auto hint = hints; *hint != nullptr; hint++) {
            QString deviceId = snd_device_name_get_hint(*hint, "NAME");

            if (deviceId.isEmpty() || deviceId == "null")
                continue;

            QString description = snd_device_name_get_hint(*hint, "DESC");
            description.replace('\n', " - ");
            QString io = snd_device_name_get_hint(*hint, "IOID");

            QList<AkAudioCaps::SampleFormat> _supportedFormats;
            QList<AkAudioCaps::ChannelLayout> _supportedLayouts;
            QList<int> _supportedSampleRates;

            if (fillInputs && (io.isEmpty() || io == "Input")) {
                auto input = deviceId + ":Input";

                this->d->fillDeviceInfo(input,
                                        &_supportedFormats,
                                        &_supportedLayouts,
                                        &_supportedSampleRates);

                if (_supportedFormats.isEmpty())
                    _supportedFormats = this->d->m_supportedFormats.value(input);

                if (_supportedLayouts.isEmpty())
                    _supportedLayouts = this->d->m_supportedLayouts.value(input);

                if (_supportedSampleRates.isEmpty())
                    _supportedSampleRates = this->d->m_supportedSampleRates.value(input);

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

            if (fillOuputs && (io.isEmpty() || io == "Output")) {
                auto output = deviceId + ":Output";

                this->d->fillDeviceInfo(output,
                                        &_supportedFormats,
                                        &_supportedLayouts,
                                        &_supportedSampleRates);

                if (_supportedFormats.isEmpty())
                    _supportedFormats = this->d->m_supportedFormats.value(output);

                if (_supportedLayouts.isEmpty())
                    _supportedLayouts = this->d->m_supportedLayouts.value(output);

                if (_supportedSampleRates.isEmpty())
                    _supportedSampleRates = this->d->m_supportedSampleRates.value(output);

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

    if (this->d->m_supportedFormats != supportedFormats)
        this->d->m_supportedFormats = supportedFormats;

    if (this->d->m_supportedLayouts != supportedChannels)
        this->d->m_supportedLayouts = supportedChannels;

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

#include "moc_audiodevalsa.cpp"
