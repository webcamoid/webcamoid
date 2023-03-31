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
#include <portaudio.h>
#include <akaudiopacket.h>
#include <akaudioconverter.h>

#include "audiodevportaudio.h"

using SampleFormatMap = QMap<AkAudioCaps::SampleFormat, PaSampleFormat>;

inline const SampleFormatMap &initSampleFormats()
{
    static const SampleFormatMap sampleFormats {
        {AkAudioCaps::SampleFormat_s8 , paInt8   },
        {AkAudioCaps::SampleFormat_u8 , paUInt8  },
        {AkAudioCaps::SampleFormat_s16, paInt16  },
        {AkAudioCaps::SampleFormat_s32, paInt32  },
        {AkAudioCaps::SampleFormat_flt, paFloat32},
    };

    return sampleFormats;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatMap,
                          sampleFormats,
                          (initSampleFormats()))

class AudioDevPortAudioPrivate
{
    public:
        AudioDevPortAudio *self;
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
        PaStream *m_stream {nullptr};
        AkAudioCaps m_curCaps;
        int m_samples {0};
        size_t m_maxBufferSize {0};
        bool m_isCapture {false};

        explicit AudioDevPortAudioPrivate(AudioDevPortAudio *self);
        static int audioCallback(const void *inputBuffer,
                                 void *outputBuffer,
                                 unsigned long framesPerBuffer,
                                 const PaStreamCallbackTimeInfo *timeInfo,
                                 PaStreamCallbackFlags statusFlags,
                                 void *userData);
        void fillDeviceInfo(PaDeviceIndex deviceIndex,
                            const PaDeviceInfo *deviceInfo,
                            bool isCapture,
                            QList<AkAudioCaps::SampleFormat> *supportedFormats,
                            QList<AkAudioCaps::ChannelLayout> *supportedLayouts,
                            QList<int> *supportedSampleRates) const;
        void updateDevices();
};

AudioDevPortAudio::AudioDevPortAudio(QObject *parent):
    AudioDev(parent)
{
    this->d = new AudioDevPortAudioPrivate(this);

    auto result = Pa_Initialize();

    if (result != paNoError) {
        qDebug() << "Failed to initialize PortAudio:" << Pa_GetErrorText(result);

        return;
    }

#if 0
    /* NOTE: PortAudio does not support HotPlug API yet, so disabling for now.
     *
     * https://github.com/PortAudio/portaudio/wiki/HotPlug
     */
    Pa_SetDevicesChangedCallback(this->d,
                                 [] (void *userData) {
        auto self = reinterpret_cast<AudioDevPortAudioPrivate *>(userData);

        if (Pa_RefreshDeviceList() == paNoError)
            self->updateDevices();
    });
#endif
    this->d->updateDevices();
}

AudioDevPortAudio::~AudioDevPortAudio()
{
    this->uninit();
    this->d->m_runLoop = false;
    this->d->m_threadResult.waitForFinished();
    auto result = Pa_Terminate();

    if (result != paNoError)
        qDebug() << "Failed to terminate PortAudio:" << Pa_GetErrorText(result);

    delete this->d;
}

QString AudioDevPortAudio::error() const
{
    return this->d->m_error;
}

QString AudioDevPortAudio::defaultInput()
{
    return this->d->m_defaultSource;
}

QString AudioDevPortAudio::defaultOutput()
{
    return this->d->m_defaultSink;
}

QStringList AudioDevPortAudio::inputs()
{
    return this->d->m_sources;
}

QStringList AudioDevPortAudio::outputs()
{
    return this->d->m_sinks;
}

QString AudioDevPortAudio::description(const QString &device)
{
    return this->d->m_pinDescriptionMap.value(device);
}

AkAudioCaps AudioDevPortAudio::preferredFormat(const QString &device)
{
    return this->d->m_preferredFormat.value(device);
}

QList<AkAudioCaps::SampleFormat> AudioDevPortAudio::supportedFormats(const QString &device)
{
    return this->d->m_supportedFormats.value(device);
}

QList<AkAudioCaps::ChannelLayout> AudioDevPortAudio::supportedChannelLayouts(const QString &device)
{
    return this->d->m_supportedLayouts.value(device);
}

QList<int> AudioDevPortAudio::supportedSampleRates(const QString &device)
{
    return this->d->m_supportedSampleRates.value(device);
}

bool AudioDevPortAudio::init(const QString &device, const AkAudioCaps &caps)
{
    this->d->m_buffers.clear();
    this->d->m_isCapture = device.startsWith("PortAudioIn:");
    QString deviceIndex(device);
    deviceIndex.remove("PortAudioIn:");
    deviceIndex.remove("PortAudioOut:");

    PaStreamParameters parameters;
    memset(&parameters, 0, sizeof(PaStreamParameters));
    parameters.device = deviceIndex.toInt();
    parameters.channelCount = caps.channels();
    parameters.sampleFormat = sampleFormats->value(caps.format());
    auto latency =
            this->d->m_isCapture?
                Pa_GetDeviceInfo(parameters.device)->defaultLowInputLatency:
                qMin(Pa_GetDeviceInfo(parameters.device)->defaultLowOutputLatency,
                     1024.0 / caps.rate());
    parameters.suggestedLatency = qMax(this->latency() / 1000.0, latency);

    this->d->m_samples = qRound(parameters.suggestedLatency * caps.rate());
    PaError result = paNoError;

    if (this->d->m_isCapture) {
        result = Pa_OpenStream(&this->d->m_stream,
                               &parameters,
                               nullptr,
                               caps.rate(),
                               this->d->m_samples,
                               paNoFlag,
                               AudioDevPortAudioPrivate::audioCallback,
                               this->d);
    } else {
        result = Pa_OpenStream(&this->d->m_stream,
                               nullptr,
                               &parameters,
                               caps.rate(),
                               this->d->m_samples,
                               paNoFlag,
                               AudioDevPortAudioPrivate::audioCallback,
                               this->d);
    }

    if (result != paNoError) {
        this->d->m_error = QString("Failed to initialize PortAudio: %1").arg(Pa_GetErrorText(result));
        qDebug() << this->d->m_error;
        emit this->errorChanged(this->d->m_error);

        return false;
    }

    size_t bufferSize =
            qMax(this->latency() * caps.rate() / 1000, 1)
                 * AkAudioCaps::bitsPerSample(caps.format())
                 * caps.channels() / 8;
    this->d->m_maxBufferSize = 4 * bufferSize;
    this->d->m_curCaps = caps;
    result = Pa_StartStream(this->d->m_stream);

    if (result != paNoError) {
        this->d->m_error = QString("Failed to start PortAudio: %1").arg(Pa_GetErrorText(result));
        qDebug() << this->d->m_error;
        emit this->errorChanged(this->d->m_error);

        return false;
    }

    return true;
}

QByteArray AudioDevPortAudio::read()
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_stream)
        return {};

    if (this->d->m_buffers.isEmpty())
        if (!this->d->m_bufferIsNotEmpty.wait(&this->d->m_mutex, 1000))
            return {};

    auto buffers = this->d->m_buffers;
    this->d->m_buffers.clear();

    return buffers;
}

bool AudioDevPortAudio::write(const AkAudioPacket &packet)
{
    if (!packet)
        return false;

    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_stream)
        return false;

    if (this->d->m_buffers.size() >= this->d->m_maxBufferSize)
        if (!this->d->m_bufferIsNotFull.wait(&this->d->m_mutex, 1000))
            return false;

    this->d->m_buffers += QByteArray(packet.constData(), packet.size());

    return true;
}

bool AudioDevPortAudio::uninit()
{
    if (this->d->m_stream) {
        Pa_StopStream(this->d->m_stream);
        Pa_CloseStream(this->d->m_stream);
        this->d->m_stream = nullptr;
    }

    this->d->m_buffers.clear();

    return true;
}

AudioDevPortAudioPrivate::AudioDevPortAudioPrivate(AudioDevPortAudio *self):
    self(self)
{
}

int AudioDevPortAudioPrivate::audioCallback(const void *inputBuffer,
                                            void *outputBuffer,
                                            unsigned long framesPerBuffer,
                                            const PaStreamCallbackTimeInfo *timeInfo,
                                            PaStreamCallbackFlags statusFlags,
                                            void *userData)
{
    Q_UNUSED(timeInfo);
    Q_UNUSED(statusFlags);

    auto self = reinterpret_cast<AudioDevPortAudioPrivate *>(userData);
    QMutexLocker mutexLocker(&self->m_mutex);

    if (self->m_isCapture) {
        size_t dataSize = framesPerBuffer
                          * self->m_curCaps.channels()
                          * self->m_curCaps.bps() / 8;
        QByteArray buffer(reinterpret_cast<const char *>(inputBuffer),
                          dataSize);

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
        size_t dataSize = framesPerBuffer
                          * self->m_curCaps.channels()
                          * self->m_curCaps.bps() / 8;

        auto copySize = qMin<size_t>(dataSize, self->m_buffers.size());

        if (copySize > 0)
            memcpy(outputBuffer, self->m_buffers.constData(), copySize);

        auto silenceSize = dataSize - copySize;

        if (silenceSize > 0)
            memset(reinterpret_cast<char *>(outputBuffer) + copySize,
                   0,
                   silenceSize);

        auto remainingSize = self->m_buffers.size() - copySize;

        if (remainingSize > 0)
            self->m_buffers = self->m_buffers.mid(copySize, remainingSize);
        else
            self->m_buffers.clear();

        if (self->m_buffers.size() < self->m_maxBufferSize)
            self->m_bufferIsNotFull.wakeAll();
    }

    return paContinue;
}

void AudioDevPortAudioPrivate::fillDeviceInfo(PaDeviceIndex deviceIndex,
                                              const PaDeviceInfo *deviceInfo,
                                              bool isCapture,
                                              QList<AkAudioCaps::SampleFormat> *supportedFormats,
                                              QList<AkAudioCaps::ChannelLayout> *supportedLayouts,
                                              QList<int> *supportedSampleRates) const
{
    static const QVector<PaSampleFormat> preferredFormats {
        paFloat32,
        paInt32,
        paInt16,
        paInt8,
        paUInt8,
    };

    auto maxChannels = qBound(1, isCapture?
                                  deviceInfo->maxInputChannels:
                                  deviceInfo->maxOutputChannels, 2);

    for (auto &format: preferredFormats) {
        PaStreamParameters parameters;
        memset(&parameters, 0, sizeof(PaStreamParameters));
        parameters.device = deviceIndex;
        parameters.channelCount = maxChannels;
        parameters.sampleFormat = format;
        PaError result = isCapture?
                             Pa_IsFormatSupported(&parameters,
                                                  nullptr,
                                                  deviceInfo->defaultSampleRate):
                             Pa_IsFormatSupported(nullptr,
                                                  &parameters,
                                                  deviceInfo->defaultSampleRate);

        if (result == paFormatIsSupported) {
            auto fmt = sampleFormats->key(format);

            if (!supportedFormats->contains(fmt))
                supportedFormats->append(fmt);
        }
    }

    supportedLayouts->append(AkAudioCaps::Layout_mono);

    if (maxChannels > 1)
        supportedLayouts->append(AkAudioCaps::Layout_stereo);

    PaSampleFormat format =
            supportedFormats->contains(AkAudioCaps::SampleFormat_s16)?
                paInt16:
                sampleFormats->value(supportedFormats->first());

    for (auto &rate: self->commonSampleRates()) {
        PaStreamParameters parameters;
        memset(&parameters, 0, sizeof(PaStreamParameters));
        parameters.device = deviceIndex;
        parameters.channelCount = maxChannels;
        parameters.sampleFormat = format;
        PaError result = isCapture?
                             Pa_IsFormatSupported(&parameters,
                                                  nullptr,
                                                  rate):
                             Pa_IsFormatSupported(nullptr,
                                                  &parameters,
                                                  rate);

        if (result == paFormatIsSupported)
            supportedSampleRates->append(rate);
    }
}

void AudioDevPortAudioPrivate::updateDevices()
{
    decltype(this->m_sources) inputs;
    decltype(this->m_sinks) outputs;
    decltype(this->m_pinDescriptionMap) pinDescriptionMap;
    decltype(this->m_supportedFormats) supportedFormats;
    decltype(this->m_supportedLayouts) supportedChannels;
    decltype(this->m_supportedSampleRates) supportedSampleRates;
    decltype(this->m_preferredFormat) preferredFormats;

    // List devices

    auto devices = Pa_GetDeviceCount();
    auto defaultInputIndex = Pa_GetDefaultInputDevice();
    auto defaultOutputIndex = Pa_GetDefaultOutputDevice();
    QString defaultInput;
    QString defaultOutput;

    for (PaDeviceIndex i = 0; i < devices; i++) {
        auto deviceInfo = Pa_GetDeviceInfo(i);

        if (!deviceInfo)
            continue;

        if (deviceInfo->maxInputChannels > 0) {
            auto deviceID = QString("PortAudioIn:%1").arg(i);
            QString deviceName = deviceInfo->name;
            inputs << deviceID;
            pinDescriptionMap[deviceID] = deviceName;
            QList<AkAudioCaps::SampleFormat> _supportedFormats;
            QList<AkAudioCaps::ChannelLayout> _supportedLayouts;
            QList<int> _supportedSampleRates;
            this->fillDeviceInfo(i,
                                 deviceInfo,
                                 true,
                                 &_supportedFormats,
                                 &_supportedLayouts,
                                 &_supportedSampleRates);
            supportedFormats[deviceID] = _supportedFormats;
            supportedChannels[deviceID] = _supportedLayouts;
            supportedSampleRates[deviceID] = _supportedSampleRates;
            auto defaultFormat =
                    _supportedFormats.contains(AkAudioCaps::SampleFormat_s16)?
                        AkAudioCaps::SampleFormat_s16:
                        _supportedFormats.first();
            preferredFormats[deviceID] = {defaultFormat,
                                          AkAudioCaps::Layout_mono,
                                          false,
                                          qRound(deviceInfo->defaultSampleRate)};
            auto apiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);

            if (i == defaultInputIndex || i == apiInfo->defaultInputDevice)
                defaultInput = deviceID;
        }

        if (deviceInfo->maxOutputChannels > 0) {
            auto deviceID = QString("PortAudioOut:%1").arg(i);
            QString deviceName = deviceInfo->name;
            outputs << deviceID;
            pinDescriptionMap[deviceID] = deviceName;
            QList<AkAudioCaps::SampleFormat> _supportedFormats;
            QList<AkAudioCaps::ChannelLayout> _supportedLayouts;
            QList<int> _supportedSampleRates;
            this->fillDeviceInfo(i,
                                 deviceInfo,
                                 false,
                                 &_supportedFormats,
                                 &_supportedLayouts,
                                 &_supportedSampleRates);
            supportedFormats[deviceID] = _supportedFormats;
            supportedChannels[deviceID] = _supportedLayouts;
            supportedSampleRates[deviceID] = _supportedSampleRates;
            auto defaultFormat =
                    _supportedFormats.contains(AkAudioCaps::SampleFormat_s16)?
                        AkAudioCaps::SampleFormat_s16:
                        _supportedFormats.first();
            preferredFormats[deviceID] = {defaultFormat,
                                          deviceInfo->maxOutputChannels > 1?
                                            AkAudioCaps::Layout_stereo:
                                            AkAudioCaps::Layout_mono,
                                          false,
                                          qRound(deviceInfo->defaultSampleRate)};
            auto apiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);

            if (i == defaultOutputIndex || i == apiInfo->defaultOutputDevice)
                defaultOutput = deviceID;
        }
    }

    if (defaultInput.isEmpty())
        defaultInput = inputs.first();

    if (defaultOutput.isEmpty())
        defaultOutput = outputs.first();

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

#include "moc_audiodevportaudio.cpp"
