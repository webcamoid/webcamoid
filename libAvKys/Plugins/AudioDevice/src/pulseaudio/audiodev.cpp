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

#include <QCoreApplication>
#include <QMap>

#include "audiodev.h"

typedef QMap<AkAudioCaps::SampleFormat, pa_sample_format_t> SampleFormatMap;

inline SampleFormatMap initSampleFormatMap()
{
    SampleFormatMap sampleFormat = {
        {AkAudioCaps::SampleFormat_u8 , PA_SAMPLE_U8       },
        {AkAudioCaps::SampleFormat_s16, PA_SAMPLE_S16LE    },
        {AkAudioCaps::SampleFormat_s32, PA_SAMPLE_S32LE    },
        {AkAudioCaps::SampleFormat_flt, PA_SAMPLE_FLOAT32LE}
    };

    return sampleFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatMap, sampleFormats, (initSampleFormatMap()))

AudioDev::AudioDev(QObject *parent):
    QObject(parent)
{
    this->m_paSimple = NULL;
    this->m_curBps = 0;
    this->m_curChannels = 0;

    // Create a threaded main loop for PulseAudio
    this->m_mainLoop = pa_threaded_mainloop_new();

    if (!this->m_mainLoop)
        return;

    // Start main loop.
    if (pa_threaded_mainloop_start(this->m_mainLoop) != 0) {
        pa_threaded_mainloop_free(this->m_mainLoop);
        this->m_mainLoop = NULL;

        return;
    }

    pa_threaded_mainloop_lock(this->m_mainLoop);

    // Get main loop abstration layer.
    pa_mainloop_api *mainLoopApi =
            pa_threaded_mainloop_get_api(this->m_mainLoop);

    if (!mainLoopApi) {
        pa_threaded_mainloop_lock(this->m_mainLoop);
        pa_threaded_mainloop_stop(this->m_mainLoop);
        pa_threaded_mainloop_free(this->m_mainLoop);
        this->m_mainLoop = NULL;

        return;
    }

    // Get a PulseAudio context.
    this->m_context = pa_context_new(mainLoopApi,
                                     QCoreApplication::applicationName()
                                        .toStdString()
                                        .c_str());

    if (!this->m_context) {
        pa_threaded_mainloop_lock(this->m_mainLoop);
        pa_threaded_mainloop_stop(this->m_mainLoop);
        pa_threaded_mainloop_free(this->m_mainLoop);
        this->m_mainLoop = NULL;

        return;
    }

    // We need to set a state callback in order to connect to the server.
    pa_context_set_state_callback(this->m_context,
                                  contextStateCallbackInit,
                                  this);

    // Connect to PulseAudio server.
    if (pa_context_connect(this->m_context, 0, static_cast<pa_context_flags_t>(0), 0) < 0) {
        pa_context_unref(this->m_context);
        this->m_context = NULL;
        pa_threaded_mainloop_lock(this->m_mainLoop);
        pa_threaded_mainloop_stop(this->m_mainLoop);
        pa_threaded_mainloop_free(this->m_mainLoop);
        this->m_mainLoop = NULL;

        return;
    }

    static const QList<pa_context_state_t> expectedStates = {
        PA_CONTEXT_READY,
        PA_CONTEXT_FAILED,
        PA_CONTEXT_TERMINATED
    };

    pa_context_state_t state;

    // Wait until the connection to the server is stablished.
    forever {
        state = pa_context_get_state(this->m_context);

        if (expectedStates.contains(state))
            break;

        pa_threaded_mainloop_wait(this->m_mainLoop);
    }

    if (state != PA_CONTEXT_READY) {
        pa_context_disconnect(this->m_context);
        pa_context_unref(this->m_context);
        this->m_context = NULL;
        pa_threaded_mainloop_lock(this->m_mainLoop);
        pa_threaded_mainloop_stop(this->m_mainLoop);
        pa_threaded_mainloop_free(this->m_mainLoop);
        this->m_mainLoop = NULL;

        return;
    }

    // Get server information.
    pa_operation *operation = pa_context_get_server_info(this->m_context,
                                                         serverInfoCallback,
                                                         this);

    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(this->m_mainLoop);

    pa_operation_unref(operation);

    // Get sources information.
    operation = pa_context_get_source_info_list(this->m_context,
                                                sourceInfoCallback,
                                                this);

    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(this->m_mainLoop);

    pa_operation_unref(operation);

    // Get sinks information.
    operation = pa_context_get_sink_info_list(this->m_context,
                                              sinkInfoCallback,
                                              this);

    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(this->m_mainLoop);

    pa_operation_unref(operation);

    pa_context_set_subscribe_callback(this->m_context,
                                      this->deviceUpdateCallback,
                                      this);

    pa_operation_unref(pa_context_subscribe(this->m_context,
                                            pa_subscription_mask_t(PA_SUBSCRIPTION_MASK_SINK
                                                                   | PA_SUBSCRIPTION_MASK_SOURCE
                                                                   | PA_SUBSCRIPTION_MASK_SERVER),
                                            NULL,
                                            this));

    pa_threaded_mainloop_unlock(this->m_mainLoop);
}

AudioDev::~AudioDev()
{
    this->uninit();

    if (this->m_context) {
        pa_context_disconnect(this->m_context);
        pa_context_unref(this->m_context);
    }

    if (this->m_mainLoop) {
        pa_threaded_mainloop_stop(this->m_mainLoop);
        pa_threaded_mainloop_free(this->m_mainLoop);
    }
}

QString AudioDev::error() const
{
    return this->m_error;
}

QString AudioDev::defaultInput()
{
    this->m_mutex.lock();
    QString defaultSource = this->m_defaultSource;
    this->m_mutex.unlock();

    return defaultSource;
}

QString AudioDev::defaultOutput()
{
    this->m_mutex.lock();
    QString defaultSink = this->m_defaultSink;
    this->m_mutex.unlock();

    return defaultSink;
}

QStringList AudioDev::inputs()
{
    this->m_mutex.lock();
    QStringList inputs = this->m_sources.values();
    this->m_mutex.unlock();

    return inputs;
}

QStringList AudioDev::outputs()
{
    this->m_mutex.lock();
    QStringList outputs = this->m_sinks.values();
    this->m_mutex.unlock();

    return outputs;
}

QString AudioDev::description(const QString &device)
{
    this->m_mutex.lock();
    QString description = this->m_pinDescriptionMap.value(device, QString());
    this->m_mutex.unlock();

    return description;
}

AkAudioCaps AudioDev::preferredFormat(const QString &device)
{
    this->m_mutex.lock();
    AkAudioCaps caps = this->m_pinCapsMap.value(device, AkAudioCaps());
    this->m_mutex.unlock();

    return caps;
}

bool AudioDev::init(const QString &device, const AkAudioCaps &caps)
{
    int error;

    pa_sample_spec ss;
    ss.format = sampleFormats->value(caps.format());
    ss.channels = uint8_t(caps.channels());
    ss.rate = uint32_t(caps.rate());
    this->m_curBps = AkAudioCaps::bitsPerSample(caps.format()) / 8;
    this->m_curChannels = caps.channels();

    this->m_mutex.lock();
    bool isInput = this->m_sources.values().contains(device);
    this->m_mutex.unlock();

    this->m_paSimple = pa_simple_new(NULL,
                                     QCoreApplication::applicationName().toStdString().c_str(),
                                     isInput? PA_STREAM_RECORD: PA_STREAM_PLAYBACK,
                                     device.toStdString().c_str(),
                                     QCoreApplication::organizationName().toStdString().c_str(),
                                     &ss,
                                     NULL,
                                     NULL,
                                     &error);

    if (!this->m_paSimple) {
        this->m_error = QString(pa_strerror(error));
        emit this->errorChanged(this->m_error);

        return false;
    }

    return true;
}

QByteArray AudioDev::read(int samples)
{
    if (!this->m_paSimple)
        return QByteArray();

    int error;

    QByteArray buffer(samples
                      * this->m_curBps
                      * this->m_curChannels,
                      Qt::Uninitialized);

    if (pa_simple_read(this->m_paSimple,
                       buffer.data(),
                       size_t(buffer.size()),
                       &error) < 0) {
        this->m_error = QString(pa_strerror(error));
        emit this->errorChanged(this->m_error);

        return QByteArray();
    }

    return buffer;
}

bool AudioDev::write(const QByteArray &frame)
{
    if (!this->m_paSimple)
        return false;

    int error;

    if (pa_simple_write(this->m_paSimple,
                        frame.data(),
                        size_t(frame.size()),
                        &error) < 0) {
        this->m_error = QString(pa_strerror(error));
        emit this->errorChanged(this->m_error);

        return false;
    }

    return true;
}

bool AudioDev::uninit()
{
    bool ok = true;

    if (this->m_paSimple) {
        int error;

        if (pa_simple_drain(this->m_paSimple, &error) < 0) {
            this->m_error = QString(pa_strerror(error));
            emit this->errorChanged(this->m_error);
            ok = false;
        }

        pa_simple_free(this->m_paSimple);
    } else
        ok = false;

    this->m_paSimple = NULL;
    this->m_curBps = 0;
    this->m_curChannels = 0;

    return ok;
}

void AudioDev::deviceUpdateCallback(pa_context *context,
                                    pa_subscription_event_type_t eventType,
                                    uint32_t index,
                                    void *userData)
{
    AudioDev *audioDevice = static_cast<AudioDev *>(userData);

    int type = eventType & PA_SUBSCRIPTION_EVENT_TYPE_MASK;
    int facility = eventType & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;

    switch (type) {
    case PA_SUBSCRIPTION_EVENT_NEW:
    case PA_SUBSCRIPTION_EVENT_CHANGE:
        switch (facility) {
        case PA_SUBSCRIPTION_EVENT_SERVER:
            pa_operation_unref(pa_context_get_server_info(context, serverInfoCallback, userData));

            break;
        case PA_SUBSCRIPTION_EVENT_SINK:
            pa_operation_unref(pa_context_get_sink_info_by_index(context, index, sinkInfoCallback, userData));

            break;
        case PA_SUBSCRIPTION_EVENT_SOURCE:
            pa_operation_unref(pa_context_get_source_info_by_index(context, index, sourceInfoCallback, userData));

            break;
        default:
            break;
        }
        break;
    case PA_SUBSCRIPTION_EVENT_REMOVE:
        switch (facility) {
        case PA_SUBSCRIPTION_EVENT_SINK: {
            audioDevice->m_mutex.lock();
            QString device = audioDevice->m_sinks.value(index);
            audioDevice->m_pinCapsMap.remove(device);
            audioDevice->m_pinDescriptionMap.remove(device);
            audioDevice->m_sinks.remove(index);
            emit audioDevice->outputsChanged(audioDevice->m_sinks.values());
            audioDevice->m_mutex.unlock();

            break;
        }
        case PA_SUBSCRIPTION_EVENT_SOURCE: {
            audioDevice->m_mutex.lock();
            QString device = audioDevice->m_sources.value(index);
            audioDevice->m_pinCapsMap.remove(device);
            audioDevice->m_pinDescriptionMap.remove(device);
            audioDevice->m_sources.remove(index);
            emit audioDevice->inputsChanged(audioDevice->m_sources.values());
            audioDevice->m_mutex.unlock();

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

void AudioDev::contextStateCallbackInit(pa_context *context, void *userdata)
{
    Q_UNUSED(context)

    AudioDev *audioDevice = reinterpret_cast<AudioDev *>(userdata);

    // Return as soon as possible.
    pa_threaded_mainloop_signal(audioDevice->m_mainLoop, 0);
}

void AudioDev::serverInfoCallback(pa_context *context,
                                  const pa_server_info *info,
                                  void *userdata)
{
    Q_UNUSED(context)

    // Get default input and output devices.
    AudioDev *audioDevice = static_cast<AudioDev *>(userdata);

    audioDevice->m_mutex.lock();

    if (audioDevice->m_defaultSink != info->default_sink_name) {
        audioDevice->m_defaultSink = info->default_sink_name;
        emit audioDevice->defaultOutputChanged(audioDevice->m_defaultSink);
    }

    if (audioDevice->m_defaultSource != info->default_source_name) {
        audioDevice->m_defaultSource = info->default_source_name;
        emit audioDevice->defaultInputChanged(audioDevice->m_defaultSource);
    }

    audioDevice->m_mutex.unlock();


    // Return as soon as possible.
    pa_threaded_mainloop_signal(audioDevice->m_mainLoop, 0);
}

void AudioDev::sourceInfoCallback(pa_context *context,
                                  const pa_source_info *info,
                                  int isLast,
                                  void *userdata)
{
    AudioDev *audioDevice = reinterpret_cast<AudioDev *>(userdata);

    if (isLast < 0) {
        audioDevice->m_error = QString(pa_strerror(pa_context_errno(context)));
        emit audioDevice->errorChanged(audioDevice->m_error);

        return;
    }

    // Finish info querying.
    if (isLast) {
        // Return as soon as possible.
        pa_threaded_mainloop_signal(audioDevice->m_mainLoop, 0);

        return;
    }

    // Get info for the pin.
    audioDevice->m_mutex.lock();

    QMap<uint32_t, QString> sources = audioDevice->m_sources;
    QMap<QString, AkAudioCaps> pinCapsMap = audioDevice->m_pinCapsMap;
    QMap<QString, QString> pinDescriptionMap = audioDevice->m_pinDescriptionMap;

    audioDevice->m_sources[info->index] = info->name;

    audioDevice->m_pinDescriptionMap[info->name] =
            strlen(info->description) < 1?
                  info->name: info->description;

    AkAudioCaps audioCaps;
    audioCaps.isValid() = true;
    audioCaps.format() = sampleFormats->key(info->sample_spec.format);
    audioCaps.bps() = AkAudioCaps::bitsPerSample(audioCaps.format());
    audioCaps.channels() = info->sample_spec.channels;
    audioCaps.rate() = int(info->sample_spec.rate);
    audioCaps.layout() = AkAudioCaps::defaultChannelLayout(info->sample_spec.channels);
    audioCaps.align() = false;

    audioDevice->m_pinCapsMap[info->name] = audioCaps;

    if (sources != audioDevice->m_sources
        || pinCapsMap != audioDevice->m_pinCapsMap
        || pinDescriptionMap != audioDevice->m_pinDescriptionMap)
        emit audioDevice->inputsChanged(audioDevice->m_sources.values());

    audioDevice->m_mutex.unlock();
}

void AudioDev::sinkInfoCallback(pa_context *context,
                                const pa_sink_info *info,
                                int isLast,
                                void *userdata)
{
    AudioDev *audioDevice = reinterpret_cast<AudioDev *>(userdata);

    if (isLast < 0) {
        audioDevice->m_error = QString(pa_strerror(pa_context_errno(context)));
        emit audioDevice->errorChanged(audioDevice->m_error);

        return;
    }

    // Finish info querying.
    if (isLast) {
        // Return as soon as possible.
        pa_threaded_mainloop_signal(audioDevice->m_mainLoop, 0);

        return;
    }

    // Get info for the pin.
    audioDevice->m_mutex.lock();

    QMap<uint32_t, QString> sinks = audioDevice->m_sinks;
    QMap<QString, AkAudioCaps> pinCapsMap = audioDevice->m_pinCapsMap;
    QMap<QString, QString> pinDescriptionMap = audioDevice->m_pinDescriptionMap;

    audioDevice->m_sinks[info->index] = info->name;

    audioDevice->m_pinDescriptionMap[info->name] =
            strlen(info->description) < 1?
                  info->name: info->description;

    AkAudioCaps audioCaps;
    audioCaps.isValid() = true;
    audioCaps.format() = sampleFormats->key(info->sample_spec.format);
    audioCaps.bps() = AkAudioCaps::bitsPerSample(audioCaps.format());
    audioCaps.channels() = info->sample_spec.channels;
    audioCaps.rate() = int(info->sample_spec.rate);
    audioCaps.layout() = AkAudioCaps::defaultChannelLayout(info->sample_spec.channels);
    audioCaps.align() = false;

    audioDevice->m_pinCapsMap[info->name] = audioCaps;

    if (sinks != audioDevice->m_sinks
        || pinCapsMap != audioDevice->m_pinCapsMap
        || pinDescriptionMap != audioDevice->m_pinDescriptionMap)
        emit audioDevice->outputsChanged(audioDevice->m_sinks.values());

    audioDevice->m_mutex.unlock();
}
