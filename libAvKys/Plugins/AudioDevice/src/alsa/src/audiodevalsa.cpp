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

#include <cstdarg>
#include <alsa/error.h>

#include "audiodevalsa.h"

typedef QMap<AkAudioCaps::SampleFormat, snd_pcm_format_t> SampleFormatMap;

inline SampleFormatMap initSampleFormatMap()
{
    SampleFormatMap sampleFormat = {
        {AkAudioCaps::SampleFormat_s8   , SND_PCM_FORMAT_S8        },
        {AkAudioCaps::SampleFormat_u8   , SND_PCM_FORMAT_U8        },
        {AkAudioCaps::SampleFormat_s16  , SND_PCM_FORMAT_S16_LE    },
        {AkAudioCaps::SampleFormat_s16be, SND_PCM_FORMAT_S16_BE    },
        {AkAudioCaps::SampleFormat_u16le, SND_PCM_FORMAT_U16_LE    },
        {AkAudioCaps::SampleFormat_u16be, SND_PCM_FORMAT_U16_BE    },
        {AkAudioCaps::SampleFormat_s24le, SND_PCM_FORMAT_S24_LE    },
        {AkAudioCaps::SampleFormat_s24be, SND_PCM_FORMAT_S24_BE    },
        {AkAudioCaps::SampleFormat_u24le, SND_PCM_FORMAT_U24_LE    },
        {AkAudioCaps::SampleFormat_u24be, SND_PCM_FORMAT_U24_BE    },
        {AkAudioCaps::SampleFormat_s32le, SND_PCM_FORMAT_S32_LE    },
        {AkAudioCaps::SampleFormat_s32be, SND_PCM_FORMAT_S32_BE    },
        {AkAudioCaps::SampleFormat_u32le, SND_PCM_FORMAT_U32_LE    },
        {AkAudioCaps::SampleFormat_u32be, SND_PCM_FORMAT_U32_BE    },
        {AkAudioCaps::SampleFormat_fltle, SND_PCM_FORMAT_FLOAT_LE  },
        {AkAudioCaps::SampleFormat_fltbe, SND_PCM_FORMAT_FLOAT_BE  },
        {AkAudioCaps::SampleFormat_dblle, SND_PCM_FORMAT_FLOAT64_LE},
        {AkAudioCaps::SampleFormat_dblbe, SND_PCM_FORMAT_FLOAT64_BE},
        {AkAudioCaps::SampleFormat_s16  , SND_PCM_FORMAT_S16       },
        {AkAudioCaps::SampleFormat_u16  , SND_PCM_FORMAT_U16       },
        {AkAudioCaps::SampleFormat_s24  , SND_PCM_FORMAT_S24       },
        {AkAudioCaps::SampleFormat_u24  , SND_PCM_FORMAT_U24       },
        {AkAudioCaps::SampleFormat_s32  , SND_PCM_FORMAT_S32       },
        {AkAudioCaps::SampleFormat_u32  , SND_PCM_FORMAT_U32       },
        {AkAudioCaps::SampleFormat_flt  , SND_PCM_FORMAT_FLOAT     },
        {AkAudioCaps::SampleFormat_dbl  , SND_PCM_FORMAT_FLOAT64   },
    };

    return sampleFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatMap, sampleFormats, (initSampleFormatMap()))

AudioDevAlsa::AudioDevAlsa(QObject *parent):
    AudioDev(parent)
{
    this->m_pcmHnd = NULL;
    this->m_fsWatcher = NULL;
    this->m_timer.setInterval(3000);

    QObject::connect(&this->m_timer,
                     &QTimer::timeout,
                     this,
                     &AudioDevAlsa::updateDevices);

#if 1
    this->m_fsWatcher = new QFileSystemWatcher({"/dev/snd"}, this);

    QObject::connect(this->m_fsWatcher,
                     &QFileSystemWatcher::directoryChanged,
                     this,
                     &AudioDevAlsa::updateDevices);

    this->updateDevices();
#else
    this->updateDevices();
    this->m_timer.start();
#endif
}

AudioDevAlsa::~AudioDevAlsa()
{
    this->uninit();

    if (this->m_fsWatcher)
        delete this->m_fsWatcher;
}

QString AudioDevAlsa::error() const
{
    return this->m_error;
}

QString AudioDevAlsa::defaultInput()
{
    return this->m_defaultSource;
}

QString AudioDevAlsa::defaultOutput()
{
    return this->m_defaultSink;
}

QStringList AudioDevAlsa::inputs()
{
    return this->m_sources;
}

QStringList AudioDevAlsa::outputs()
{
    return this->m_sinks;
}

QString AudioDevAlsa::description(const QString &device)
{
    return this->m_pinDescriptionMap.value(device);
}

AkAudioCaps AudioDevAlsa::preferredFormat(const QString &device)
{
    return this->m_sinks.contains(device)?
                AkAudioCaps(AkAudioCaps::SampleFormat_s16,
                            2,
                            44100):
                AkAudioCaps(AkAudioCaps::SampleFormat_u8,
                            1,
                            8000);
}

QList<AkAudioCaps::SampleFormat> AudioDevAlsa::supportedFormats(const QString &device)
{
    return this->m_supportedFormats.value(device);
}

QList<int> AudioDevAlsa::supportedChannels(const QString &device)
{
    return this->m_supportedChannels.value(device);
}

QList<int> AudioDevAlsa::supportedSampleRates(const QString &device)
{
    return this->m_supportedSampleRates.value(device);
}

bool AudioDevAlsa::init(const QString &device, const AkAudioCaps &caps)
{
    QMutexLocker mutexLockeer(&this->m_mutex);

    this->m_pcmHnd = NULL;
    int error = snd_pcm_open(&this->m_pcmHnd,
                             QString(device)
                                 .remove(QRegExp(":Input$|:Output$"))
                                 .toStdString().c_str(),
                             device.endsWith(":Input")?
                                 SND_PCM_STREAM_CAPTURE: SND_PCM_STREAM_PLAYBACK,
                             SND_PCM_NONBLOCK);

    if (error < 0)
        goto init_fail;

    error = snd_pcm_set_params(this->m_pcmHnd,
                               sampleFormats->value(caps.format(),
                                                    SND_PCM_FORMAT_UNKNOWN),
                               SND_PCM_ACCESS_RW_INTERLEAVED,
                               uint(caps.channels()),
                               uint(caps.rate()),
                               1,
                               500000);

    if (error < 0)
        goto init_fail;

    return true;

init_fail:
    this->m_error = snd_strerror(error);
    emit this->errorChanged(this->m_error);
    this->uninit();

    return false;
}

QByteArray AudioDevAlsa::read(int samples)
{
    QMutexLocker mutexLockeer(&this->m_mutex);

    auto bufferSize = snd_pcm_frames_to_bytes(this->m_pcmHnd, samples);
    QByteArray buffer(int(bufferSize), 0);
    auto data = buffer.data();

    while (samples > 0) {
        auto rsamples = snd_pcm_readi(this->m_pcmHnd,
                                      data,
                                      snd_pcm_uframes_t(samples));

        if (rsamples >= 0) {
            auto dataRead = snd_pcm_frames_to_bytes(this->m_pcmHnd, rsamples);
            data += dataRead;
            samples -= rsamples;
        } else {
            if (rsamples == -EAGAIN) {
                snd_pcm_wait(this->m_pcmHnd, 1000);

                continue;
            }

            return QByteArray();
        }
    }

    return buffer;
}

bool AudioDevAlsa::write(const AkAudioPacket &packet)
{
    QMutexLocker mutexLockeer(&this->m_mutex);

    if (!this->m_pcmHnd)
        return false;

    auto data = packet.buffer().constData();
    int dataSize = packet.buffer().size();

    while (dataSize > 0) {
        auto samples = snd_pcm_bytes_to_frames(this->m_pcmHnd, dataSize);
        samples = snd_pcm_writei(this->m_pcmHnd,
                                 data,
                                 snd_pcm_uframes_t(samples));

        if (samples >= 0) {
            auto dataWritten = snd_pcm_frames_to_bytes(this->m_pcmHnd, samples);
            data += dataWritten;
            dataSize -= dataWritten;
        } else {
            if (samples == -EAGAIN) {
                snd_pcm_wait(this->m_pcmHnd, 1000);

                continue;
            }

            samples = snd_pcm_recover(this->m_pcmHnd, int(samples), 0);

            if (samples < 0)
                return false;
        }
    }

    return true;
}

bool AudioDevAlsa::uninit()
{
    if (this->m_pcmHnd) {
        snd_pcm_close(this->m_pcmHnd);
        this->m_pcmHnd = NULL;
    }

    return true;
}

void AudioDevAlsa::fillDeviceInfo(const QString &device,
                                  QList<AkAudioCaps::SampleFormat> *supportedFormats,
                                  QList<int> *supportedChannels,
                                  QList<int> *supportedSampleRates) const
{
    snd_pcm_t *pcmHnd = NULL;
    int error = snd_pcm_open(&pcmHnd,
                             QString(device)
                                 .remove(QRegExp(":Input$|:Output$"))
                                 .toStdString().c_str(),
                             device.endsWith(":Input")?
                                 SND_PCM_STREAM_CAPTURE: SND_PCM_STREAM_PLAYBACK,
                             SND_PCM_NONBLOCK);

    if (error < 0)
        return;

    snd_pcm_hw_params_t *hwParams = NULL;
    snd_pcm_hw_params_malloc(&hwParams);
    snd_pcm_hw_params_any(pcmHnd, hwParams);

    // Get params.

    if (snd_pcm_hw_params_test_access(pcmHnd,
                                      hwParams,
                                      SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
        goto deviceCaps_fail;

    static const QVector<snd_pcm_format_t> preferredFormats = {
        SND_PCM_FORMAT_FLOAT,
        SND_PCM_FORMAT_S32,
        SND_PCM_FORMAT_U32,
        SND_PCM_FORMAT_S16,
        SND_PCM_FORMAT_U16,
        SND_PCM_FORMAT_S8,
        SND_PCM_FORMAT_U8
    };

    for (auto fmt: preferredFormats)
        if (snd_pcm_hw_params_test_format(pcmHnd, hwParams, fmt) >= 0)
            supportedFormats->append(sampleFormats->key(fmt));

    for (int channels = 1; channels < 3; channels++)
        if (snd_pcm_hw_params_test_channels(pcmHnd, hwParams, uint(channels)) >= 0)
            supportedChannels->append(channels);

    for (auto &rate: this->m_commonSampleRates)
        if (snd_pcm_hw_params_test_rate(pcmHnd, hwParams, uint(rate), 0) >= 0)
            supportedSampleRates->append(rate);

deviceCaps_fail:
    snd_pcm_hw_params_free(hwParams);

    if (pcmHnd)
        snd_pcm_close(pcmHnd);
}

void AudioDevAlsa::updateDevices()
{
    decltype(this->m_sources) inputs;
    decltype(this->m_sinks) outputs;
    decltype(this->m_pinDescriptionMap) pinDescriptionMap;
    decltype(this->m_supportedFormats) supportedFormats;
    decltype(this->m_supportedChannels) supportedChannels;
    decltype(this->m_supportedSampleRates) supportedSampleRates;

    int card = -1;
    snd_ctl_card_info_t *ctlInfo = NULL;
    snd_ctl_card_info_malloc(&ctlInfo);

    while (snd_card_next(&card) >= 0 && card >= 0) {
        char name[32];
        sprintf(name, "hw:%d", card);
        snd_ctl_t *ctlHnd = NULL;

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
        QList<int> _supportedChannels;
        QList<int> _supportedSampleRates;

        auto input = deviceId + ":Input";
        this->fillDeviceInfo(input,
                             &_supportedFormats,
                             &_supportedChannels,
                             &_supportedSampleRates);

        if (_supportedFormats.isEmpty())
            _supportedFormats = this->m_supportedFormats.value(input);

        if (_supportedChannels.isEmpty())
            _supportedChannels = this->m_supportedChannels.value(input);

        if (_supportedSampleRates.isEmpty())
            _supportedSampleRates = this->m_supportedSampleRates.value(input);

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

        auto output = deviceId + ":Output";
        this->fillDeviceInfo(output,
                             &_supportedFormats,
                             &_supportedChannels,
                             &_supportedSampleRates);

        if (_supportedFormats.isEmpty())
            _supportedFormats = this->m_supportedFormats.value(output);

        if (_supportedChannels.isEmpty())
            _supportedChannels = this->m_supportedChannels.value(output);

        if (_supportedSampleRates.isEmpty())
            _supportedSampleRates = this->m_supportedSampleRates.value(output);

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

    snd_ctl_card_info_free(ctlInfo);

    // In case the first method for detecting the devices didn't worked,
    // use hints to detect the devices.
    void **hints = NULL;
    bool fillInputs = inputs.isEmpty();
    bool fillOuputs = outputs.isEmpty();

    if (snd_device_name_hint(-1, "pcm", &hints) >= 0) {
        for (auto hint = hints; *hint != NULL; hint++) {
            QString deviceId = snd_device_name_get_hint(*hint, "NAME");

            if (deviceId.isEmpty() || deviceId == "null")
                continue;

            QString description = snd_device_name_get_hint(*hint, "DESC");
            description.replace('\n', " - ");
            QString io = snd_device_name_get_hint(*hint, "IOID");

            QList<AkAudioCaps::SampleFormat> _supportedFormats;
            QList<int> _supportedChannels;
            QList<int> _supportedSampleRates;

            if (fillInputs && (io.isEmpty() || io == "Input")) {
                auto input = deviceId + ":Input";

                this->fillDeviceInfo(input,
                                     &_supportedFormats,
                                     &_supportedChannels,
                                     &_supportedSampleRates);

                if (_supportedFormats.isEmpty())
                    _supportedFormats = this->m_supportedFormats.value(input);

                if (_supportedChannels.isEmpty())
                    _supportedChannels = this->m_supportedChannels.value(input);

                if (_supportedSampleRates.isEmpty())
                    _supportedSampleRates = this->m_supportedSampleRates.value(input);

                if (!_supportedFormats.isEmpty()
                    && !_supportedChannels.isEmpty()
                    && !_supportedSampleRates.isEmpty()) {
                    inputs << input;
                    pinDescriptionMap[input] = description;
                    supportedFormats[input] = _supportedFormats;
                    supportedChannels[input] = _supportedChannels;
                    supportedSampleRates[input] = _supportedSampleRates;
                }
            }

            _supportedFormats.clear();
            _supportedChannels.clear();
            _supportedSampleRates.clear();

            if (fillOuputs && (io.isEmpty() || io == "Output")) {
                auto output = deviceId + ":Output";

                this->fillDeviceInfo(output,
                                     &_supportedFormats,
                                     &_supportedChannels,
                                     &_supportedSampleRates);

                if (_supportedFormats.isEmpty())
                    _supportedFormats = this->m_supportedFormats.value(output);

                if (_supportedChannels.isEmpty())
                    _supportedChannels = this->m_supportedChannels.value(output);

                if (_supportedSampleRates.isEmpty())
                    _supportedSampleRates = this->m_supportedSampleRates.value(output);

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
        }

        snd_device_name_free_hint(hints);
    }

    if (this->m_supportedFormats != supportedFormats)
        this->m_supportedFormats = supportedFormats;

    if (this->m_supportedChannels != supportedChannels)
        this->m_supportedChannels = supportedChannels;

    if (this->m_supportedSampleRates != supportedSampleRates)
        this->m_supportedSampleRates = supportedSampleRates;

    if (this->m_pinDescriptionMap != pinDescriptionMap)
        this->m_pinDescriptionMap = pinDescriptionMap;

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
