/* Webcamoid, camera capture application.
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

#if USE_SDL_VERSION >= 3
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#endif

#include <akaudiopacket.h>
#include <akaudioconverter.h>

#include "audiodevsdl.h"

#if SDL_VERSION_ATLEAST(3, 2, 0)
using SDL_AudioFormat_Type = SDL_AudioFormat;

#define SDL_EVENT_QUIT_TYPE               SDL_EVENT_QUIT
#define SDL_EVENT_AUDIODEVICEADDED_TYPE   SDL_EVENT_AUDIO_DEVICE_ADDED
#define SDL_EVENT_AUDIODEVICEREMOVED_TYPE SDL_EVENT_AUDIO_DEVICE_REMOVED
#else
using SDL_AudioFormat_Type = int;

#define SDL_EVENT_QUIT_TYPE               SDL_QUIT
#define SDL_EVENT_AUDIODEVICEADDED_TYPE   SDL_AUDIODEVICEADDED
#define SDL_EVENT_AUDIODEVICEREMOVED_TYPE SDL_AUDIODEVICEREMOVED
#endif

struct SampleFormat
{
    AkAudioCaps::SampleFormat akFormat;
    SDL_AudioFormat_Type sdlFormat;

    SampleFormat(AkAudioCaps::SampleFormat akFormat,
                 SDL_AudioFormat_Type sdlFormat):
        akFormat(akFormat),
        sdlFormat(sdlFormat)
    {
    }

    inline static const SampleFormat *table()
    {
        static const SampleFormat akAudioDevSDLTable[] = {
#if SDL_VERSION_ATLEAST(3, 2, 0)
            {AkAudioCaps::SampleFormat_s8  , SDL_AUDIO_S8     },
            {AkAudioCaps::SampleFormat_u8  , SDL_AUDIO_U8     },
            {AkAudioCaps::SampleFormat_s16 , SDL_AUDIO_S16    },
            {AkAudioCaps::SampleFormat_s32 , SDL_AUDIO_S32    },
            {AkAudioCaps::SampleFormat_flt , SDL_AUDIO_F32    },
            {AkAudioCaps::SampleFormat_none, SDL_AUDIO_UNKNOWN},
#else
            {AkAudioCaps::SampleFormat_s8  , AUDIO_S8 },
            {AkAudioCaps::SampleFormat_u8  , AUDIO_U8 },
            {AkAudioCaps::SampleFormat_s16 , AUDIO_S16},
            {AkAudioCaps::SampleFormat_u16 , AUDIO_U16},
            {AkAudioCaps::SampleFormat_s32 , AUDIO_S32},
            {AkAudioCaps::SampleFormat_flt , AUDIO_F32},
            {AkAudioCaps::SampleFormat_none, 0        },
#endif
        };

        return akAudioDevSDLTable;
    }

    inline static const SampleFormat *byAkFormat(AkAudioCaps::SampleFormat akFormat)
    {
        auto fmt = table();

        for (; fmt->akFormat != AkAudioCaps::SampleFormat_none; ++fmt)
            if (fmt->akFormat == akFormat)
                return fmt;

        return fmt;
    }

    inline static const SampleFormat *bySdlFormat(SDL_AudioFormat_Type sdlFormat)
    {
        auto fmt = table();

        for (; fmt->akFormat != AkAudioCaps::SampleFormat_none; ++fmt)
            if (fmt->sdlFormat == sdlFormat)
                return fmt;

        return fmt;
    }
};

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

#if SDL_VERSION_ATLEAST(3, 2, 0)
        SDL_AudioStream *m_audioStream {nullptr};
#else
        SDL_AudioDeviceID m_audioDevice {0};
#endif

        size_t m_maxBufferSize {0};
        bool m_isCapture {false};

        explicit AudioDevSDLPrivate(AudioDevSDL *self);

#if SDL_VERSION_ATLEAST(3, 2, 0)
        static void audioCallback(void *userData,
                                  SDL_AudioStream *stream,
                                  int additionalAmount,
                                  int totalAmount);
#else
        static void audioCallback(void *userData, Uint8 *data, int dataSize);
#endif

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
                              &AudioDevSDLPrivate::sdlEventLoop,
                              this->d);
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
    bool isCapture = device.startsWith("SDLIn:");

#if SDL_VERSION_ATLEAST(3, 2, 0)
    QString deviceId(device);
    deviceId.remove("SDLIn:").remove("SDLOut:");
#endif

    SDL_AudioSpec spec;
    memset(&spec, 0, sizeof(SDL_AudioSpec));

    spec.format = SampleFormat::byAkFormat(caps.format())->sdlFormat;
    spec.channels = caps.channels();
    spec.freq = caps.rate();

#if !SDL_VERSION_ATLEAST(3, 2, 0)
    spec.userdata = this->d;
    spec.callback = AudioDevSDLPrivate::audioCallback;
    spec.samples = qMax(this->latency() * caps.rate() / 1000, 1);
#endif

#if SDL_VERSION_ATLEAST(3, 2, 0)
    this->d->m_audioStream =
            SDL_OpenAudioDeviceStream(static_cast<SDL_AudioDeviceID>(deviceId.toInt()),
                                      &spec,
                                      AudioDevSDLPrivate::audioCallback,
                                      this->d);

    if (!this->d->m_audioStream) {
        this->d->m_error =
                QString("Failed to initialize SDL: %1").arg(SDL_GetError());
        qDebug() << this->d->m_error;
        emit this->errorChanged(this->d->m_error);

        return false;
    }
#else
    this->d->m_audioDevice =
            SDL_OpenAudioDevice(deviceName.toStdString().c_str(),
                                isCapture? SDL_TRUE: SDL_FALSE,
                                &spec,
                                &spec,
                                SDL_AUDIO_ALLOW_ANY_CHANGE);

    if (!this->d->m_audioDevice) {
        this->d->m_error =
                QString("Failed to initialize SDL: %1").arg(SDL_GetError());
        qDebug() << this->d->m_error;
        emit this->errorChanged(this->d->m_error);

        return false;
    }
#endif

#if SDL_VERSION_ATLEAST(3, 2, 0)
    this->d->m_maxBufferSize = 2
                               * qMax(this->latency() * caps.rate() / 1000, 1)
                               * caps.channels()
                               * caps.bps()
                               / 8;
#else
    this->d->m_maxBufferSize = 2 * spec.size;
#endif

    this->d->m_isCapture = isCapture;

    static const QMap<int, AkAudioCaps::ChannelLayout> layoutsMap {
        {1, AkAudioCaps::Layout_mono  },
        {2, AkAudioCaps::Layout_stereo},
        {4, AkAudioCaps::Layout_quad  },
        {6, AkAudioCaps::Layout_5p1   },
    };

    this->d->m_audioConvert.setOutputCaps({SampleFormat::bySdlFormat(spec.format)->akFormat,
                                           layoutsMap.value(spec.channels),
                                           false,
                                           spec.freq});
    this->d->m_audioConvert.reset();

#if SDL_VERSION_ATLEAST(3, 2, 0)
    SDL_ResumeAudioStreamDevice(this->d->m_audioStream);
#else
    SDL_PauseAudioDevice(this->d->m_audioDevice, SDL_FALSE);
#endif

    return true;
}

QByteArray AudioDevSDL::read()
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

#if SDL_VERSION_ATLEAST(3, 2, 0)
    if (!this->d->m_audioStream)
        return {};
#else
    if (!this->d->m_audioDevice)
        return {};
#endif

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

#if SDL_VERSION_ATLEAST(3, 2, 0)
    if (!this->d->m_audioStream)
        return false;
#else
    if (!this->d->m_audioDevice)
        return false;
#endif

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

#if SDL_VERSION_ATLEAST(3, 2, 0)
    if (this->d->m_audioStream) {
        SDL_DestroyAudioStream(this->d->m_audioStream);
        this->d->m_audioStream = nullptr;
    }
#else
    if (this->d->m_audioDevice) {
        SDL_PauseAudioDevice(this->d->m_audioDevice, SDL_TRUE);
        SDL_CloseAudioDevice(this->d->m_audioDevice);
        this->d->m_audioDevice = 0;
    }
#endif

    this->d->m_buffers.clear();

    return true;
}

AudioDevSDLPrivate::AudioDevSDLPrivate(AudioDevSDL *self):
    self(self)
{
}

#if SDL_VERSION_ATLEAST(3, 2, 0)
void AudioDevSDLPrivate::audioCallback(void *userData,
                                       SDL_AudioStream *stream,
                                       int additionalAmount,
                                       int totalAmount)
#else
void AudioDevSDLPrivate::audioCallback(void *userData,
                                       Uint8 *data,
                                       int dataSize)
#endif
{
    auto self = reinterpret_cast<AudioDevSDLPrivate *>(userData);
    QMutexLocker mutexLocker(&self->m_mutex);

#if SDL_VERSION_ATLEAST(3, 2, 0)
    // In SDL3, we use additionalAmount as the data size to process
    int dataSize = additionalAmount;
    // Temporary buffer for SDL3
    QByteArray tempBuffer(dataSize, 0);
    auto data = reinterpret_cast<Uint8 *>(tempBuffer.data());
#endif

    if (self->m_isCapture) {
        // Capture mode (audio input)
#if SDL_VERSION_ATLEAST(3, 2, 0)
        // Read data from the audio stream in SDL3
        int bytesRead = SDL_GetAudioStreamData(stream, data, dataSize);

        if (bytesRead < 0) {
            qDebug() << "Error reading from audio stream:" << SDL_GetError();

            return;
        }

        dataSize = bytesRead; // Adjust size to the actual bytes read
#endif

        QByteArray buffer(reinterpret_cast<char *>(data), dataSize);

        if (dataSize >= self->m_maxBufferSize) {
            // If the data size exceeds m_maxBufferSize, take only the latest data
            self->m_buffers = buffer.right(self->m_maxBufferSize);
        } else {
            // Append new data, respecting the m_maxBufferSize limit
            auto totalSize = qMin<size_t>(self->m_buffers.size() + dataSize, self->m_maxBufferSize);
            auto prevSize = totalSize - dataSize;
            self->m_buffers = self->m_buffers.right(prevSize) + buffer;
        }

        self->m_bufferIsNotEmpty.wakeAll();
    } else {
        // Playback mode (audio output)
        auto copySize = qMin(dataSize, self->m_buffers.size());

        if (copySize > 0) {
            // Copy data from the internal buffer to the output
            memcpy(data, self->m_buffers.constData(), copySize);
            // Update the internal buffer, removing the copied data
            self->m_buffers = self->m_buffers.mid(copySize);
        }

        // Fill with silence if necessary
        auto silenceSize = dataSize - copySize;

        if (silenceSize > 0)
            memset(data + copySize, 0, silenceSize);

#if SDL_VERSION_ATLEAST(3, 2, 0)
        // In SDL3, write the data to the audio stream
        if (SDL_PutAudioStreamData(stream, data, dataSize) < 0) {
            qDebug() << "Error writing to audio stream:" << SDL_GetError();

            return;
        }
#endif

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
            case SDL_EVENT_QUIT_TYPE:
                return;

            case SDL_EVENT_AUDIODEVICEADDED_TYPE:
            case SDL_EVENT_AUDIODEVICEREMOVED_TYPE:
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

#if SDL_VERSION_ATLEAST(3, 2, 0)
    int ndevices = 0;
    auto devices = SDL_GetAudioRecordingDevices(&ndevices);
#else
    auto ndevices = SDL_GetNumAudioDevices(SDL_TRUE);
#endif

    for (int i = 0; i < ndevices; ++i) {
#if SDL_VERSION_ATLEAST(3, 2, 0)
        SDL_AudioSpec spec;
        memset(&spec, 0, sizeof(SDL_AudioSpec));

        if (!SDL_GetAudioDeviceFormat(devices[i], &spec, nullptr))
            continue;
#elif SDL_VERSION_ATLEAST(2, 0, 16)
        SDL_AudioSpec spec;
        memset(&spec, 0, sizeof(SDL_AudioSpec));

        if (SDL_GetAudioDeviceSpec(i, SDL_TRUE, &spec) != 0)
            continue;
#endif

#if SDL_VERSION_ATLEAST(3, 2, 0)
        auto deviceID = QString("SDLIn:%1").arg(devices[i]);
        QString deviceName = SDL_GetAudioDeviceName(devices[i]);
#else
        auto deviceID = QString("SDLIn:%1").arg(i);
        QString deviceName = SDL_GetAudioDeviceName(i, SDL_FALSE);
#endif

        inputs << deviceID;
        pinDescriptionMap[deviceID] = deviceName;
        supportedFormats[deviceID] = commonFormats;
        supportedChannels[deviceID] = {AkAudioCaps::Layout_mono,
                                       AkAudioCaps::Layout_stereo};
        supportedSampleRates[deviceID] = commonSampleRates;

#if SDL_VERSION_ATLEAST(3, 2, 0)
        preferredFormats[deviceID] = {SampleFormat::bySdlFormat(spec.format)->akFormat,
                                      spec.channels > 1?
                                        AkAudioCaps::Layout_stereo:
                                        AkAudioCaps::Layout_mono,
                                      false,
                                      spec.freq};
#elif SDL_VERSION_ATLEAST(2, 0, 16)
        preferredFormats[deviceID] = {SampleFormat::bySdlFormat(spec.format)->akFormat,
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

#if SDL_VERSION_ATLEAST(3, 2, 0)
    QString defaultInput;

    if (!inputs.isEmpty())
        defaultInput = inputs.first();
#elif SDL_VERSION_ATLEAST(2, 24, 0)
    char *deviceName = nullptr;
    SDL_AudioSpec spec;
    memset(&spec, 0, sizeof(SDL_AudioSpec));
    auto result = SDL_GetDefaultAudioInfo(&deviceName, &spec, SDL_TRUE);
    QString defaultInput;

    if (result == 0 && deviceName) {
        defaultInput = pinDescriptionMap.key(deviceName);
        SDL_free(deviceName);
    } else if (!inputs.isEmpty()) {
        defaultInput = inputs.first();
    }
#else
    QString defaultInput;

    if (!inputs.isEmpty())
        defaultInput = inputs.first();
#endif

    // List playback devices

#if SDL_VERSION_ATLEAST(3, 2, 0)
    ndevices = 0;
    devices = SDL_GetAudioPlaybackDevices(&ndevices);
#else
    ndevices = SDL_GetNumAudioDevices(SDL_FALSE);
#endif

    for (int i = 0; i < ndevices; ++i) {
#if SDL_VERSION_ATLEAST(3, 2, 0)
        SDL_AudioSpec spec;
        memset(&spec, 0, sizeof(SDL_AudioSpec));

        if (!SDL_GetAudioDeviceFormat(devices[i], &spec, nullptr))
            continue;
#elif SDL_VERSION_ATLEAST(2, 0, 16)
        SDL_AudioSpec spec;
        memset(&spec, 0, sizeof(SDL_AudioSpec));

        if (SDL_GetAudioDeviceSpec(i, SDL_FALSE, &spec) != 0)
            continue;
#endif

#if SDL_VERSION_ATLEAST(3, 2, 0)
        auto deviceID = QString("SDLOut:%1").arg(devices[i]);
        QString deviceName = SDL_GetAudioDeviceName(devices[i]);
#else
        auto deviceID = QString("SDLOut:%1").arg(i);
        QString deviceName = SDL_GetAudioDeviceName(i, SDL_FALSE);
#endif

        outputs << deviceID;
        pinDescriptionMap[deviceID] = deviceName;
        supportedFormats[deviceID] = commonFormats;
        supportedChannels[deviceID] = commonLayouts;
        supportedSampleRates[deviceID] = commonSampleRates;

#if SDL_VERSION_ATLEAST(3, 2, 0)
        preferredFormats[deviceID] = {SampleFormat::bySdlFormat(spec.format)->akFormat,
                                      spec.channels > 1?
                                        AkAudioCaps::Layout_stereo:
                                        AkAudioCaps::Layout_mono,
                                      false,
                                      spec.freq};
#elif SDL_VERSION_ATLEAST(2, 0, 16)
        preferredFormats[deviceID] = {SampleFormat::bySdlFormat(spec.format)->akFormat,
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

#if SDL_VERSION_ATLEAST(3, 2, 0)
    QString defaultOutput;

    if (!outputs.isEmpty())
        defaultOutput = outputs.first();
#elif SDL_VERSION_ATLEAST(2, 24, 0)
    deviceName = nullptr;
    memset(&spec, 0, sizeof(SDL_AudioSpec));
    result = SDL_GetDefaultAudioInfo(&deviceName, &spec, SDL_FALSE);
    QString defaultOutput;

    if (result == 0 && deviceName) {
        defaultOutput = pinDescriptionMap.key(deviceName);
        SDL_free(deviceName);
    } else if (!outputs.isEmpty()) {
        defaultOutput = outputs.first();
    }
#else
    QString defaultOutput;

    if (!outputs.isEmpty())
        defaultOutput = outputs.first();
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
