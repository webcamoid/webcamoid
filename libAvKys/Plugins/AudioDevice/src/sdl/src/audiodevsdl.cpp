/* Webcamoid, webcam capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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

#include <QMap>
#include <QMutex>
#include <QVector>
#include <QWaitCondition>
#include <QtConcurrent>
#include <QtDebug>
#include <SDL.h>
#include <SDL_audio.h>
#include <akaudiopacket.h>
#include <akaudioconverter.h>

#include "audiodevsdl.h"

using SampleFormatMap = QMap<AkAudioCaps::SampleFormat, int>;

inline const SampleFormatMap &initSampleFormats()
{
    static const SampleFormatMap sampleFormats {
        {AkAudioCaps::SampleFormat_s8 , AUDIO_S8 },
        {AkAudioCaps::SampleFormat_u8 , AUDIO_U8 },
        {AkAudioCaps::SampleFormat_s16, AUDIO_S16},
        {AkAudioCaps::SampleFormat_u16, AUDIO_U16},
        {AkAudioCaps::SampleFormat_s32, AUDIO_S32},
        {AkAudioCaps::SampleFormat_flt, AUDIO_F32},
    };

    return sampleFormats;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatMap,
                          sampleFormats,
                          (initSampleFormats()))

class AudioDevSDLPrivate
{
    public:
        AudioDevSDL *self;
        QString m_error;
        QString m_defaultSink;
        QString m_defaultSource;
        QStringList m_sinks;
        QStringList m_sources;
        QMap<QString, QString> m_pinDescriptionMap;
        QMap<QString, QList<AkAudioCaps::SampleFormat>> m_supportedFormats;
        QMap<QString, QList<AkAudioCaps::ChannelLayout>> m_supportedLayouts;
        QMap<QString, QList<int>> m_supportedSampleRates;
        QMap<QString, AkAudioCaps> m_preferredFormat;
        QMutex m_mutex;
        QWaitCondition m_bufferIsNotEmpty;
        QWaitCondition m_bufferIsNotFull;
        QThreadPool m_threadPool;
        bool m_runLoop {true};
        QFuture<void> m_threadResult;
        QByteArray m_buffers;
        AkAudioConverter m_audioConvert;
        SDL_AudioDeviceID m_audioDevice {0};
        int m_samples {0};
        size_t m_maxBufferSize {0};
        bool m_isCapture {false};

        explicit AudioDevSDLPrivate(AudioDevSDL *self);
        static void audioCallback(void *userData, Uint8 *data, int dataSize);
        void sdlEventLoop();
        void updateDevices();
};

AudioDevSDL::AudioDevSDL(QObject *parent):
    AudioDev(parent)
{
    this->d = new AudioDevSDLPrivate(this);

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        qDebug() << "Failed to initialize SDL:" << SDL_GetError();

        return;
    }

    this->d->updateDevices();
    this->d->m_threadResult =
            QtConcurrent::run(&this->d->m_threadPool,
                              this->d,
                              &AudioDevSDLPrivate::sdlEventLoop);
}

AudioDevSDL::~AudioDevSDL()
{
    this->uninit();
    this->d->m_runLoop = false;
    this->d->m_threadResult.waitForFinished();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);

    delete this->d;
}

QString AudioDevSDL::error() const
{
    return this->d->m_error;
}

QString AudioDevSDL::defaultInput()
{
    return this->d->m_defaultSource;
}

QString AudioDevSDL::defaultOutput()
{
    return this->d->m_defaultSink;
}

QStringList AudioDevSDL::inputs()
{
    return this->d->m_sources;
}

QStringList AudioDevSDL::outputs()
{
    return this->d->m_sinks;
}

QString AudioDevSDL::description(const QString &device)
{
    return this->d->m_pinDescriptionMap.value(device);
}

AkAudioCaps AudioDevSDL::preferredFormat(const QString &device)
{
    return this->d->m_preferredFormat.value(device);
}

QList<AkAudioCaps::SampleFormat> AudioDevSDL::supportedFormats(const QString &device)
{
    return this->d->m_supportedFormats.value(device);
}

QList<AkAudioCaps::ChannelLayout> AudioDevSDL::supportedChannelLayouts(const QString &device)
{
    return this->d->m_supportedLayouts.value(device);
}

QList<int> AudioDevSDL::supportedSampleRates(const QString &device)
{
    return this->d->m_supportedSampleRates.value(device);
}

bool AudioDevSDL::init(const QString &device, const AkAudioCaps &caps)
{
    this->d->m_buffers.clear();
    auto deviceName = this->d->m_pinDescriptionMap.value(device);
    SDL_bool isCapture = device.startsWith("SDLIn:")? SDL_TRUE: SDL_FALSE;
    SDL_AudioSpec spec;
    memset(&spec, 0, sizeof(SDL_AudioSpec));
    spec.userdata = this->d;
    spec.callback = AudioDevSDLPrivate::audioCallback;
    spec.format = sampleFormats->value(caps.format());
    spec.channels = caps.channels();
    spec.freq = caps.rate();
    spec.samples = qMax(this->latency() * caps.rate() / 1000, 1);
    this->d->m_audioDevice =
            SDL_OpenAudioDevice(deviceName.toStdString().c_str(),
                                isCapture,
                                &spec,
                                &spec,
                                SDL_AUDIO_ALLOW_ANY_CHANGE);

    if (!this->d->m_audioDevice) {
        this->d->m_error = QString("Failed to initialize SDL: %1").arg(SDL_GetError());
        qDebug() << this->d->m_error;
        emit this->errorChanged(this->d->m_error);

        return false;
    }

    this->d->m_maxBufferSize = 2 * spec.size;
    this->d->m_isCapture = isCapture;

    static const QMap<int, AkAudioCaps::ChannelLayout> layoutsMap {
        {1, AkAudioCaps::Layout_mono  },
        {2, AkAudioCaps::Layout_stereo},
        {4, AkAudioCaps::Layout_quad  },
        {6, AkAudioCaps::Layout_5p1   },
    };

    this->d->m_audioConvert.setOutputCaps({sampleFormats->key(spec.format),
                                           layoutsMap.value(spec.channels),
                                           false,
                                           spec.freq});
    this->d->m_audioConvert.reset();
    SDL_PauseAudioDevice(this->d->m_audioDevice, SDL_FALSE);

    return true;
}

QByteArray AudioDevSDL::read()
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_audioDevice)
        return {};

    if (this->d->m_buffers.isEmpty())
        if (!this->d->m_bufferIsNotEmpty.wait(&this->d->m_mutex, 1000))
            return {};

    auto buffers = this->d->m_buffers;
    this->d->m_buffers.clear();

    return buffers;
}

bool AudioDevSDL::write(const AkAudioPacket &packet)
{
    if (!packet)
        return false;

    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_audioDevice)
        return false;

    if (this->d->m_buffers.size() >= this->d->m_maxBufferSize)
        if (!this->d->m_bufferIsNotFull.wait(&this->d->m_mutex, 1000))
            return false;

    auto audioPacket = this->d->m_audioConvert.convert(packet);

    if (!audioPacket)
        return false;

    this->d->m_buffers += QByteArray(audioPacket.constData(),
                                     audioPacket.size());

    return true;
}

bool AudioDevSDL::uninit()
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (this->d->m_audioDevice) {
        SDL_PauseAudioDevice(this->d->m_audioDevice, SDL_TRUE);
        SDL_CloseAudioDevice(this->d->m_audioDevice);
        this->d->m_audioDevice = 0;
    }

    this->d->m_buffers.clear();

    return true;
}

AudioDevSDLPrivate::AudioDevSDLPrivate(AudioDevSDL *self):
    self(self)
{
}

void AudioDevSDLPrivate::audioCallback(void *userData,
                                       Uint8 *data,
                                       int dataSize)
{
    auto self = reinterpret_cast<AudioDevSDLPrivate *>(userData);
    QMutexLocker mutexLocker(&self->m_mutex);

    if (self->m_isCapture) {
        QByteArray buffer(reinterpret_cast<char *>(data), dataSize);

        if (dataSize >= self->m_maxBufferSize) {
            self->m_buffers = buffer;
        } else {
            auto totalSize = qMin<size_t>(self->m_buffers.size() + dataSize,
                                          self->m_maxBufferSize);
            auto prevSize = totalSize - dataSize;
            self->m_buffers =
                    self->m_buffers.mid(self->m_buffers.size() - int(prevSize),
                                        int(prevSize))
                    + buffer;
        }

        self->m_bufferIsNotEmpty.wakeAll();
    } else {
        auto copySize = qMin(dataSize, self->m_buffers.size());

        if (copySize > 0)
            memcpy(data, self->m_buffers.constData(), copySize);

        auto silenceSize = dataSize - copySize;

        if (silenceSize > 0)
            memset(data + copySize, 0, silenceSize);

        auto remainingSize = self->m_buffers.size() - copySize;

        if (remainingSize > 0)
            self->m_buffers = self->m_buffers.mid(copySize, remainingSize);
        else
            self->m_buffers.clear();

        if (self->m_buffers.size() < self->m_maxBufferSize)
            self->m_bufferIsNotFull.wakeAll();
    }
}

void AudioDevSDLPrivate::sdlEventLoop()
{
    while (this->m_runLoop) {
        SDL_Event event;

        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
            case SDL_QUIT:
                return;

            case SDL_AUDIODEVICEADDED:
            case SDL_AUDIODEVICEREMOVED:
                this->updateDevices();

                break;

            default:
                break;
            }
        }

        QThread::msleep(1000);
    }
}

void AudioDevSDLPrivate::updateDevices()
{
    decltype(this->m_sources) inputs;
    decltype(this->m_sinks) outputs;
    decltype(this->m_pinDescriptionMap) pinDescriptionMap;
    decltype(this->m_supportedFormats) supportedFormats;
    decltype(this->m_supportedLayouts) supportedChannels;
    decltype(this->m_supportedSampleRates) supportedSampleRates;
    decltype(this->m_preferredFormat) preferredFormats;

    /* NOTE: SDL doesn't have a proper way of detecting supported formats, so
     * support the most commons and leave the converter to do the rest.
     */

    static const QList<AkAudioCaps::SampleFormat> commonFormats {
        AkAudioCaps::SampleFormat_s8,
        AkAudioCaps::SampleFormat_u8,
        AkAudioCaps::SampleFormat_s16,
        AkAudioCaps::SampleFormat_u16,
        AkAudioCaps::SampleFormat_s32,
        AkAudioCaps::SampleFormat_flt,
    };
    static const QList<AkAudioCaps::ChannelLayout> commonLayouts {
        AkAudioCaps::Layout_mono,
        AkAudioCaps::Layout_stereo,
        AkAudioCaps::Layout_quad,
        AkAudioCaps::Layout_5p1
    };
    static const auto commonSampleRates = self->commonSampleRates().toList();

    // List capture devices

    auto devices = SDL_GetNumAudioDevices(SDL_TRUE);

    for (int i = 0; i < devices; i++) {
#if SDL_VERSION_ATLEAST(2, 0, 16)
        SDL_AudioSpec spec;
        memset(&spec, 0, sizeof(SDL_AudioSpec));

        if (SDL_GetAudioDeviceSpec(i,
                                   SDL_TRUE,
                                   &spec) != 0) {
            continue;
        }
#endif
        auto deviceID = QString("SDLIn:%1").arg(i);
        QString deviceName = SDL_GetAudioDeviceName(i, SDL_TRUE);
        inputs << deviceID;
        pinDescriptionMap[deviceID] = deviceName;
        supportedFormats[deviceID] = commonFormats;
        supportedChannels[deviceID] = {AkAudioCaps::Layout_mono,
                                       AkAudioCaps::Layout_stereo};
        supportedSampleRates[deviceID] = commonSampleRates;
#if SDL_VERSION_ATLEAST(2, 0, 16)
        preferredFormats[deviceID] = {sampleFormats->key(spec.format),
                                      spec.channels > 1?
                                        AkAudioCaps::Layout_stereo:
                                        AkAudioCaps::Layout_mono,
                                      false,
                                      spec.freq};
#else
        preferredFormats[deviceID] = {AkAudioCaps::SampleFormat_s16,
                                      AkAudioCaps::Layout_mono,
                                      false,
                                      8000};
#endif
    }

#if SDL_VERSION_ATLEAST(2, 24, 0)
    char *deviceName = nullptr;
    SDL_AudioSpec spec;
    memset(&spec, 0, sizeof(SDL_AudioSpec));
    auto result = SDL_GetDefaultAudioInfo(&deviceName,
                                          &spec,
                                          SDL_TRUE);
    QString defaultInput;

    if (result == 0 && deviceName) {
        defaultInput = pinDescriptionMap.key(deviceName);
        SDL_free(deviceName);
    } else {
        defaultInput = inputs.first();
    }
#else
    auto defaultInput = inputs.first();
#endif

    // List playback devices

    devices = SDL_GetNumAudioDevices(SDL_FALSE);

    for (int i = 0; i < devices; i++) {
#if SDL_VERSION_ATLEAST(2, 0, 16)
        SDL_AudioSpec spec;
        memset(&spec, 0, sizeof(SDL_AudioSpec));

        if (SDL_GetAudioDeviceSpec(i,
                                   SDL_FALSE,
                                   &spec) != 0) {
            continue;
        }
#endif
        auto deviceID = QString("SDLOut:%1").arg(i);
        QString deviceName = SDL_GetAudioDeviceName(i, SDL_FALSE);
        outputs << deviceID;
        pinDescriptionMap[deviceID] = deviceName;
        supportedFormats[deviceID] = commonFormats;
        supportedChannels[deviceID] = commonLayouts;
        supportedSampleRates[deviceID] = commonSampleRates;
#if SDL_VERSION_ATLEAST(2, 0, 16)
        preferredFormats[deviceID] = {sampleFormats->key(spec.format),
                                      spec.channels > 1?
                                        AkAudioCaps::Layout_stereo:
                                        AkAudioCaps::Layout_mono,
                                      false,
                                      spec.freq};
#else
        preferredFormats[deviceID] = {AkAudioCaps::SampleFormat_s16,
                                      AkAudioCaps::Layout_stereo,
                                      false,
                                      44100};
#endif
    }

#if SDL_VERSION_ATLEAST(2, 24, 0)
    deviceName = nullptr;
    memset(&spec, 0, sizeof(SDL_AudioSpec));
    result = SDL_GetDefaultAudioInfo(&deviceName,
                                     &spec,
                                     SDL_FALSE);
    QString defaultOutput;

    if (result == 0 && deviceName) {
        defaultOutput = pinDescriptionMap.key(deviceName);
        SDL_free(deviceName);
    } else {
        defaultOutput = outputs.first();
    }
#else
    auto defaultOutput = outputs.first();
#endif

    // Update devices

    if (this->m_supportedFormats != supportedFormats)
        this->m_supportedFormats = supportedFormats;

    if (this->m_supportedLayouts != supportedChannels)
        this->m_supportedLayouts = supportedChannels;

    if (this->m_supportedSampleRates != supportedSampleRates)
        this->m_supportedSampleRates = supportedSampleRates;

    if (this->m_pinDescriptionMap != pinDescriptionMap)
        this->m_pinDescriptionMap = pinDescriptionMap;

    if (this->m_preferredFormat != preferredFormats)
        this->m_preferredFormat = preferredFormats;

    if (this->m_sources != inputs) {
        this->m_sources = inputs;
        emit self->inputsChanged(inputs);
    }

    if (this->m_sinks != outputs) {
        this->m_sinks = outputs;
        emit self->outputsChanged(outputs);
    }

    if (this->m_defaultSource != defaultInput) {
        this->m_defaultSource = defaultInput;
        emit self->defaultInputChanged(defaultInput);
    }

    if (this->m_defaultSink != defaultOutput) {
        this->m_defaultSink = defaultOutput;
        emit self->defaultOutputChanged(defaultOutput);
    }
}

#include "moc_audiodevsdl.cpp"
