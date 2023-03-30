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

#include <QCoreApplication>
#include <QMap>
#include <QMutex>
#include <QVector>
#include <QtDebug>
#include <akaudiopacket.h>
#include <pulse/simple.h>
#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/subscribe.h>
#include <pulse/thread-mainloop.h>
#include <pulse/error.h>

#include "audiodevpulseaudio.h"

using SampleFormatMap = QMap<pa_sample_format_t, AkAudioCaps::SampleFormat>;

inline SampleFormatMap initSampleFormatMap()
{
    SampleFormatMap sampleFormat {
        {PA_SAMPLE_U8       , AkAudioCaps::SampleFormat_u8   },
        {PA_SAMPLE_S16BE    , AkAudioCaps::SampleFormat_s16be},
        {PA_SAMPLE_S16LE    , AkAudioCaps::SampleFormat_s16le},
        {PA_SAMPLE_S32BE    , AkAudioCaps::SampleFormat_s32be},
        {PA_SAMPLE_S32LE    , AkAudioCaps::SampleFormat_s32le},
        {PA_SAMPLE_FLOAT32BE, AkAudioCaps::SampleFormat_fltbe},
        {PA_SAMPLE_FLOAT32LE, AkAudioCaps::SampleFormat_fltle},
    };

    return sampleFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatMap, sampleFormats, (initSampleFormatMap()))

class AudioDevPulseAudioPrivate
{
    public:
        AudioDevPulseAudio *self;
        QString m_error;
        pa_simple *m_paSimple {nullptr};
        pa_threaded_mainloop *m_mainLoop {nullptr};
        pa_context *m_context {nullptr};
        QString m_defaultSink;
        QString m_defaultSource;
        QMap<uint32_t, QString> m_sinks;
        QMap<uint32_t, QString> m_sources;
        QMap<QString, AkAudioCaps> m_pinCapsMap;
        QMap<QString, QString> m_pinDescriptionMap;
        QMutex m_mutex;
        QMutex m_streamMutex;
        int m_samples {0};
        int m_curBps {0};
        int m_curChannels {0};

        explicit AudioDevPulseAudioPrivate(AudioDevPulseAudio *self);
        static void deviceUpdateCallback(pa_context *context,
                                         pa_subscription_event_type_t eventType,
                                         uint32_t index,
                                         void *userData);
        static void contextStateCallbackInit(pa_context *context,
                                             void *userdata);
        static void serverInfoCallback(pa_context *context,
                                       const pa_server_info *info,
                                       void *userdata);
        static void sourceInfoCallback(pa_context *context,
                                       const pa_source_info *info,
                                       int isLast,
                                       void *userdata);
        static void sinkInfoCallback(pa_context *context,
                                     const pa_sink_info *info,
                                     int isLast,
                                     void *userdata);
};

AudioDevPulseAudio::AudioDevPulseAudio(QObject *parent):
    AudioDev(parent)
{
    this->d = new AudioDevPulseAudioPrivate(this);

    // Create a threaded main loop for PulseAudio
    this->d->m_mainLoop = pa_threaded_mainloop_new();

    if (!this->d->m_mainLoop)
        return;

    // Start main loop.
    if (pa_threaded_mainloop_start(this->d->m_mainLoop) != 0) {
        pa_threaded_mainloop_free(this->d->m_mainLoop);
        this->d->m_mainLoop = nullptr;

        return;
    }

    pa_threaded_mainloop_lock(this->d->m_mainLoop);

    // Get main loop abstration layer.
    auto mainLoopApi = pa_threaded_mainloop_get_api(this->d->m_mainLoop);

    if (!mainLoopApi) {
        pa_threaded_mainloop_unlock(this->d->m_mainLoop);
        pa_threaded_mainloop_stop(this->d->m_mainLoop);
        pa_threaded_mainloop_free(this->d->m_mainLoop);
        this->d->m_mainLoop = nullptr;

        return;
    }

    // Get a PulseAudio context.
    this->d->m_context = pa_context_new(mainLoopApi,
                                        QCoreApplication::applicationName()
                                           .toStdString()
                                           .c_str());

    if (!this->d->m_context) {
        pa_threaded_mainloop_unlock(this->d->m_mainLoop);
        pa_threaded_mainloop_stop(this->d->m_mainLoop);
        pa_threaded_mainloop_free(this->d->m_mainLoop);
        this->d->m_mainLoop = nullptr;

        return;
    }

    // We need to set a state callback in order to connect to the server.
    pa_context_set_state_callback(this->d->m_context,
                                  AudioDevPulseAudioPrivate::contextStateCallbackInit,
                                  this);

    // Connect to PulseAudio server.
    if (pa_context_connect(this->d->m_context,
                           nullptr,
                           PA_CONTEXT_NOFLAGS,
                           nullptr) < 0) {
        pa_context_unref(this->d->m_context);
        this->d->m_context = nullptr;
        pa_threaded_mainloop_unlock(this->d->m_mainLoop);
        pa_threaded_mainloop_stop(this->d->m_mainLoop);
        pa_threaded_mainloop_free(this->d->m_mainLoop);
        this->d->m_mainLoop = nullptr;

        return;
    }

    static const QList<pa_context_state_t> expectedStates {
        PA_CONTEXT_READY,
        PA_CONTEXT_FAILED,
        PA_CONTEXT_TERMINATED
    };

    pa_context_state_t state;

    // Wait until the connection to the server is stablished.
    forever {
        state = pa_context_get_state(this->d->m_context);

        if (expectedStates.contains(state))
            break;

        pa_threaded_mainloop_wait(this->d->m_mainLoop);
    }

    if (state != PA_CONTEXT_READY) {
        pa_context_disconnect(this->d->m_context);
        pa_context_unref(this->d->m_context);
        this->d->m_context = nullptr;
        pa_threaded_mainloop_unlock(this->d->m_mainLoop);
        pa_threaded_mainloop_stop(this->d->m_mainLoop);
        pa_threaded_mainloop_free(this->d->m_mainLoop);
        this->d->m_mainLoop = nullptr;

        return;
    }

    // Get server information.
    auto operation =
            pa_context_get_server_info(this->d->m_context,
                                       AudioDevPulseAudioPrivate::serverInfoCallback,
                                       this);

    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(this->d->m_mainLoop);

    pa_operation_unref(operation);

    // Get sources information.
    operation = pa_context_get_source_info_list(this->d->m_context,
                                                AudioDevPulseAudioPrivate::sourceInfoCallback,
                                                this);

    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(this->d->m_mainLoop);

    pa_operation_unref(operation);

    // Get sinks information.
    operation = pa_context_get_sink_info_list(this->d->m_context,
                                              AudioDevPulseAudioPrivate::sinkInfoCallback,
                                              this);

    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(this->d->m_mainLoop);

    pa_operation_unref(operation);

    pa_context_set_subscribe_callback(this->d->m_context,
                                      AudioDevPulseAudioPrivate::deviceUpdateCallback,
                                      this);

    pa_operation_unref(pa_context_subscribe(this->d->m_context,
                                            pa_subscription_mask_t(PA_SUBSCRIPTION_MASK_SINK
                                                                   | PA_SUBSCRIPTION_MASK_SOURCE
                                                                   | PA_SUBSCRIPTION_MASK_SERVER),
                                            nullptr,
                                            this));

    pa_threaded_mainloop_unlock(this->d->m_mainLoop);
}

AudioDevPulseAudio::~AudioDevPulseAudio()
{
    this->uninit();

    if (this->d->m_context) {
        pa_context_disconnect(this->d->m_context);
        pa_context_unref(this->d->m_context);
    }

    if (this->d->m_mainLoop) {
        pa_threaded_mainloop_stop(this->d->m_mainLoop);
        pa_threaded_mainloop_free(this->d->m_mainLoop);
    }

    delete this->d;
}

QString AudioDevPulseAudio::error() const
{
    return this->d->m_error;
}

QString AudioDevPulseAudio::defaultInput()
{
    this->d->m_mutex.lock();
    auto defaultSource = this->d->m_defaultSource;
    this->d->m_mutex.unlock();

    return defaultSource;
}

QString AudioDevPulseAudio::defaultOutput()
{
    this->d->m_mutex.lock();
    auto defaultSink = this->d->m_defaultSink;
    this->d->m_mutex.unlock();

    return defaultSink;
}

QStringList AudioDevPulseAudio::inputs()
{
    this->d->m_mutex.lock();
    auto inputs = this->d->m_sources.values();
    this->d->m_mutex.unlock();

    return inputs;
}

QStringList AudioDevPulseAudio::outputs()
{
    this->d->m_mutex.lock();
    auto outputs = this->d->m_sinks.values();
    this->d->m_mutex.unlock();

    return outputs;
}

QString AudioDevPulseAudio::description(const QString &device)
{
    this->d->m_mutex.lock();
    auto description = this->d->m_pinDescriptionMap.value(device);
    this->d->m_mutex.unlock();

    return description;
}

AkAudioCaps AudioDevPulseAudio::preferredFormat(const QString &device)
{
    this->d->m_mutex.lock();
    auto caps = this->d->m_pinCapsMap.value(device);
    this->d->m_mutex.unlock();

    return caps;
}

QList<AkAudioCaps::SampleFormat> AudioDevPulseAudio::supportedFormats(const QString &device)
{
    Q_UNUSED(device)

    return sampleFormats->values();
}

QList<AkAudioCaps::ChannelLayout> AudioDevPulseAudio::supportedChannelLayouts(const QString &device)
{
    Q_UNUSED(device)

    return {AkAudioCaps::Layout_mono, AkAudioCaps::Layout_stereo};
}

QList<int> AudioDevPulseAudio::supportedSampleRates(const QString &device)
{
    Q_UNUSED(device)

    return this->commonSampleRates().toList();
}

bool AudioDevPulseAudio::init(const QString &device, const AkAudioCaps &caps)
{
   this->d->m_streamMutex.lock();

    int error;

    pa_sample_spec ss;
    ss.format = sampleFormats->key(caps.format());
    ss.channels = uint8_t(caps.channels());
    ss.rate = uint32_t(caps.rate());
    this->d->m_curBps = AkAudioCaps::bitsPerSample(caps.format()) / 8;
    this->d->m_curChannels = caps.channels();

    this->d->m_mutex.lock();
    bool isInput = std::find(this->d->m_sources.cbegin(),
                             this->d->m_sources.cend(),
                             device) != this->d->m_sources.cend();
    this->d->m_mutex.unlock();

    this->d->m_paSimple =
            pa_simple_new(nullptr,
                          QCoreApplication::applicationName().toStdString().c_str(),
                          isInput? PA_STREAM_RECORD: PA_STREAM_PLAYBACK,
                          device.toStdString().c_str(),
                          QCoreApplication::organizationName().toStdString().c_str(),
                          &ss,
                          nullptr,
                          nullptr,
                          &error);

    if (!this->d->m_paSimple) {
        this->d->m_error = QString(pa_strerror(error));
        this->d->m_streamMutex.unlock();
        emit this->errorChanged(this->d->m_error);

        return false;
    }

    this->d->m_samples = qMax(this->latency() * caps.rate() / 1000, 1);
    this->d->m_streamMutex.unlock();

    return true;
}

QByteArray AudioDevPulseAudio::read()
{
    this->d->m_streamMutex.lock();

    if (!this->d->m_paSimple) {
        this->d->m_streamMutex.unlock();

        return {};
    }

    int error;

    QByteArray buffer(this->d->m_samples
                      * this->d->m_curBps
                      * this->d->m_curChannels,
                      0);

    if (pa_simple_read(this->d->m_paSimple,
                       buffer.data(),
                       size_t(buffer.size()),
                       &error) < 0) {
        this->d->m_error = QString(pa_strerror(error));
        this->d->m_streamMutex.unlock();
        emit this->errorChanged(this->d->m_error);

        return {};
    }

    this->d->m_streamMutex.unlock();

    return buffer;
}

bool AudioDevPulseAudio::write(const AkAudioPacket &packet)
{
    this->d->m_streamMutex.lock();

    if (!this->d->m_paSimple) {
        this->d->m_streamMutex.unlock();

        return false;
    }

    int error;

    if (pa_simple_write(this->d->m_paSimple,
                        packet.constData(),
                        packet.size(),
                        &error) < 0) {
        this->d->m_error = QString(pa_strerror(error));
        this->d->m_streamMutex.unlock();
        qDebug() << this->d->m_error;
        emit this->errorChanged(this->d->m_error);

        return false;
    }

    this->d->m_streamMutex.unlock();

    return true;
}

bool AudioDevPulseAudio::uninit()
{
    QString errorStr;
    bool ok = true;

    this->d->m_streamMutex.lock();

    if (this->d->m_paSimple) {
        int error;

        if (pa_simple_drain(this->d->m_paSimple, &error) < 0) {
            errorStr = QString(pa_strerror(error));
            ok = false;
        }

        pa_simple_free(this->d->m_paSimple);
    } else {
        ok = false;
    }

    this->d->m_paSimple = nullptr;
    this->d->m_curBps = 0;
    this->d->m_curChannels = 0;
    this->d->m_streamMutex.unlock();

    if (!errorStr.isEmpty()) {
        this->d->m_error = errorStr;
        emit this->errorChanged(this->d->m_error);
    }

    return ok;
}

AudioDevPulseAudioPrivate::AudioDevPulseAudioPrivate(AudioDevPulseAudio *self):
    self(self)
{
}

void AudioDevPulseAudioPrivate::deviceUpdateCallback(pa_context *context,
                                                     pa_subscription_event_type_t eventType,
                                                     uint32_t index,
                                                     void *userData)
{
    auto audioDevice = static_cast<AudioDevPulseAudio *>(userData);

    int type = eventType & PA_SUBSCRIPTION_EVENT_TYPE_MASK;
    int facility = eventType & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;

    switch (type) {
    case PA_SUBSCRIPTION_EVENT_NEW:
    case PA_SUBSCRIPTION_EVENT_CHANGE:
        switch (facility) {
        case PA_SUBSCRIPTION_EVENT_SERVER:
            pa_operation_unref(pa_context_get_server_info(context,
                                                          serverInfoCallback,
                                                          userData));

            break;
        case PA_SUBSCRIPTION_EVENT_SINK:
            pa_operation_unref(pa_context_get_sink_info_by_index(context,
                                                                 index,
                                                                 sinkInfoCallback,
                                                                 userData));

            break;
        case PA_SUBSCRIPTION_EVENT_SOURCE:
            pa_operation_unref(pa_context_get_source_info_by_index(context,
                                                                   index,
                                                                   sourceInfoCallback,
                                                                   userData));

            break;
        default:
            break;
        }
        break;
    case PA_SUBSCRIPTION_EVENT_REMOVE:
        switch (facility) {
        case PA_SUBSCRIPTION_EVENT_SINK: {
            audioDevice->d->m_mutex.lock();

            auto device = audioDevice->d->m_sinks.value(index);
            audioDevice->d->m_pinCapsMap.remove(device);
            audioDevice->d->m_pinDescriptionMap.remove(device);
            audioDevice->d->m_sinks.remove(index);
            auto outputs = audioDevice->d->m_sinks.values();

            audioDevice->d->m_mutex.unlock();

            emit audioDevice->outputsChanged(outputs);

            break;
        }
        case PA_SUBSCRIPTION_EVENT_SOURCE: {
            audioDevice->d->m_mutex.lock();

            auto device = audioDevice->d->m_sources.value(index);
            audioDevice->d->m_pinCapsMap.remove(device);
            audioDevice->d->m_pinDescriptionMap.remove(device);
            audioDevice->d->m_sources.remove(index);
            auto sources = audioDevice->d->m_sources.values();

            audioDevice->d->m_mutex.unlock();

            emit audioDevice->inputsChanged(sources);

            break;
        }
        default:
            break;
        }
        break;
    default:
        break;
    }
}

void AudioDevPulseAudioPrivate::contextStateCallbackInit(pa_context *context,
                                                         void *userdata)
{
    Q_UNUSED(context)

    auto audioDevice = reinterpret_cast<AudioDevPulseAudio *>(userdata);

    // Return as soon as possible.
    pa_threaded_mainloop_signal(audioDevice->d->m_mainLoop, 0);
}

void AudioDevPulseAudioPrivate::serverInfoCallback(pa_context *context,
                                                   const pa_server_info *info,
                                                   void *userdata)
{
    Q_UNUSED(context)

    // Get default input and output devices.
    auto audioDevice = static_cast<AudioDevPulseAudio *>(userdata);
    bool defaultInputChanged = false;
    bool defaultOutputChanged = false;

    audioDevice->d->m_mutex.lock();

    if (audioDevice->d->m_defaultSink != info->default_sink_name) {
        audioDevice->d->m_defaultSink = info->default_sink_name;
        defaultInputChanged = true;
    }

    if (audioDevice->d->m_defaultSource != info->default_source_name) {
        audioDevice->d->m_defaultSource = info->default_source_name;
        defaultOutputChanged = true;
    }

    audioDevice->d->m_mutex.unlock();

    if (defaultInputChanged)
        emit audioDevice->defaultInputChanged(audioDevice->d->m_defaultSource);

    if (defaultOutputChanged)
        emit audioDevice->defaultOutputChanged(audioDevice->d->m_defaultSink);

    // Return as soon as possible.
    pa_threaded_mainloop_signal(audioDevice->d->m_mainLoop, 0);
}

void AudioDevPulseAudioPrivate::sourceInfoCallback(pa_context *context,
                                                   const pa_source_info *info,
                                                   int isLast,
                                                   void *userdata)
{
    auto audioDevice = reinterpret_cast<AudioDevPulseAudio *>(userdata);

    if (isLast < 0) {
        audioDevice->d->m_error =
                QString(pa_strerror(pa_context_errno(context)));
        emit audioDevice->errorChanged(audioDevice->d->m_error);

        return;
    }

    // Finish info querying.
    if (isLast) {
        // Return as soon as possible.
        pa_threaded_mainloop_signal(audioDevice->d->m_mainLoop, 0);

        return;
    }

    // Get info for the pin.
    audioDevice->d->m_mutex.lock();

    auto sources = audioDevice->d->m_sources;
    auto pinCapsMap = audioDevice->d->m_pinCapsMap;
    auto pinDescriptionMap = audioDevice->d->m_pinDescriptionMap;

    audioDevice->d->m_sources[info->index] = info->name;

    audioDevice->d->m_pinDescriptionMap[info->name] =
            QString(info->description).isEmpty()?
                  info->name: info->description;

    audioDevice->d->m_pinCapsMap[info->name] =
            AkAudioCaps(sampleFormats->value(info->sample_spec.format),
                        AkAudioCaps::defaultChannelLayout(info->sample_spec.channels),
                        false,
                        int(info->sample_spec.rate));

    audioDevice->d->m_mutex.unlock();

    if (sources != audioDevice->d->m_sources
        || pinCapsMap != audioDevice->d->m_pinCapsMap
        || pinDescriptionMap != audioDevice->d->m_pinDescriptionMap)
        emit audioDevice->inputsChanged(audioDevice->d->m_sources.values());
}

void AudioDevPulseAudioPrivate::sinkInfoCallback(pa_context *context,
                                                 const pa_sink_info *info,
                                                 int isLast,
                                                 void *userdata)
{
    auto audioDevice = reinterpret_cast<AudioDevPulseAudio *>(userdata);

    if (isLast < 0) {
        audioDevice->d->m_error =
                QString(pa_strerror(pa_context_errno(context)));
        emit audioDevice->errorChanged(audioDevice->d->m_error);

        return;
    }

    // Finish info querying.
    if (isLast) {
        // Return as soon as possible.
        pa_threaded_mainloop_signal(audioDevice->d->m_mainLoop, 0);

        return;
    }

    // Get info for the pin.
    audioDevice->d->m_mutex.lock();

    auto sinks = audioDevice->d->m_sinks;
    auto pinCapsMap = audioDevice->d->m_pinCapsMap;
    auto pinDescriptionMap = audioDevice->d->m_pinDescriptionMap;

    audioDevice->d->m_sinks[info->index] = info->name;

    audioDevice->d->m_pinDescriptionMap[info->name] =
            QString(info->description).isEmpty()?
                  info->name: info->description;

    audioDevice->d->m_pinCapsMap[info->name] =
            AkAudioCaps(sampleFormats->value(info->sample_spec.format),
                        AkAudioCaps::defaultChannelLayout(info->sample_spec.channels),
                        false,
                        int(info->sample_spec.rate));

    audioDevice->d->m_mutex.unlock();

    if (sinks != audioDevice->d->m_sinks
        || pinCapsMap != audioDevice->d->m_pinCapsMap
        || pinDescriptionMap != audioDevice->d->m_pinDescriptionMap) {
        emit audioDevice->outputsChanged(audioDevice->d->m_sinks.values());
    }
}

#include "moc_audiodevpulseaudio.cpp"
