/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include <QCoreApplication>
#include <QMap>

#include "audiodevice.h"

typedef QMap<QbAudioCaps::SampleFormat, pa_sample_format_t> SampleFormatMap;

inline SampleFormatMap initSampleFormatMap()
{
    SampleFormatMap sampleFormat;
    sampleFormat[QbAudioCaps::SampleFormat_u8] = PA_SAMPLE_U8;
    sampleFormat[QbAudioCaps::SampleFormat_s16] = PA_SAMPLE_S16LE;
    sampleFormat[QbAudioCaps::SampleFormat_s32] = PA_SAMPLE_S32LE;
    sampleFormat[QbAudioCaps::SampleFormat_flt] = PA_SAMPLE_FLOAT32LE;

    return sampleFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatMap, sampleFormats, (initSampleFormatMap()))

typedef QMap<QbAudioCaps::SampleFormat, int> BytesPerSampleMap;

inline BytesPerSampleMap initBytesPerSampleMap()
{
    BytesPerSampleMap bytesPerSample;
    bytesPerSample[QbAudioCaps::SampleFormat_u8] = 1;
    bytesPerSample[QbAudioCaps::SampleFormat_s16] = 2;
    bytesPerSample[QbAudioCaps::SampleFormat_s32] = 4;
    bytesPerSample[QbAudioCaps::SampleFormat_flt] = 4;

    return bytesPerSample;
}

Q_GLOBAL_STATIC_WITH_ARGS(BytesPerSampleMap, bytesPerSample, (initBytesPerSampleMap()))

AudioDevice::AudioDevice(QObject *parent):
    QObject(parent)
{
    this->m_paSimple = NULL;
    this->m_defaultFormat = PA_SAMPLE_INVALID;
    this->m_defaultChannels = 0;
    this->m_defaultRate = 0;
    this->m_curBps = 0;
    this->m_curChannels = 0;
}

AudioDevice::~AudioDevice()
{
    this->uninit();
}

QString AudioDevice::error() const
{
    return this->m_error;
}

// Get native format for the default audio device.
bool AudioDevice::preferredFormat(DeviceMode mode,
                                  QbAudioCaps::SampleFormat *sampleFormat,
                                  int *channels,
                                  int *sampleRate)
{
    // Create a threaded main loop for PulseAudio
    this->m_mainLoop = pa_threaded_mainloop_new();

    if (!this->m_mainLoop) {
        this->m_error = "preferredFormat: pa_threaded_mainloop_new";
        emit this->errorChanged(this->m_error);

        return false;
    }

    // Start main loop.
    if (pa_threaded_mainloop_start(this->m_mainLoop) != 0) {
        this->m_error = "preferredFormat: pa_threaded_mainloop_start";
        emit this->errorChanged(this->m_error);
        pa_threaded_mainloop_free(this->m_mainLoop);

        return false;
    }

    // Get main loop abstration layer.
    pa_mainloop_api *mainLoopApi = pa_threaded_mainloop_get_api(this->m_mainLoop);

    if (!mainLoopApi) {
        this->m_error = "preferredFormat: pa_threaded_mainloop_get_api";
        emit this->errorChanged(this->m_error);
        pa_threaded_mainloop_stop(this->m_mainLoop);
        pa_threaded_mainloop_free(this->m_mainLoop);

        return false;
    }

    // Protect main loop from begin modified from other threads.
    pa_threaded_mainloop_lock(this->m_mainLoop);

    // Get a PulseAudio context.
    pa_context *context = pa_context_new(mainLoopApi,
                                         QCoreApplication::applicationName()
                                            .toStdString()
                                            .c_str());

    if (!context) {
        this->m_error = QString(pa_strerror(pa_context_errno(context)));
        emit this->errorChanged(this->m_error);
        pa_threaded_mainloop_unlock(this->m_mainLoop);
        pa_threaded_mainloop_stop(this->m_mainLoop);
        pa_threaded_mainloop_free(this->m_mainLoop);

        return false;
    }

    // We need to set a state callback in order to connect to the server.
    pa_context_set_state_callback(context, contextStateCallbackInit, this);

    // Connect to PulseAudio server.
    if (pa_context_connect(context, 0, (pa_context_flags_t) 0, 0) < 0) {
        this->m_error = QString(pa_strerror(pa_context_errno(context)));
        emit this->errorChanged(this->m_error);
        pa_context_unref(context);
        pa_threaded_mainloop_unlock(this->m_mainLoop);
        pa_threaded_mainloop_stop(this->m_mainLoop);
        pa_threaded_mainloop_free(this->m_mainLoop);

        return false;
    }

    QList<pa_context_state_t> expectedStates;
    expectedStates << PA_CONTEXT_READY
                   << PA_CONTEXT_FAILED
                   << PA_CONTEXT_TERMINATED;

    pa_context_state_t state;

    // Wait until the connection to the server is stablished.
    forever {
        state = pa_context_get_state(context);

        if (expectedStates.contains(state))
            break;

        pa_threaded_mainloop_wait(this->m_mainLoop);
    }

    if (state != PA_CONTEXT_READY) {
        this->m_error = QString("preferredFormat: %1").arg(state);
        emit this->errorChanged(this->m_error);
        pa_context_disconnect(context);
        pa_context_unref(context);
        pa_threaded_mainloop_unlock(this->m_mainLoop);
        pa_threaded_mainloop_stop(this->m_mainLoop);
        pa_threaded_mainloop_free(this->m_mainLoop);

        return false;
    }

    // Get server information.
    pa_operation *operation = pa_context_get_server_info(context,
                                                         serverInfoCallback,
                                                         this);

    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(this->m_mainLoop);

    pa_operation_unref(operation);

    // Get source/sink information.
    if (mode == DeviceModeCapture)
        operation = pa_context_get_source_info_list(context,
                                                    sourceInfoCallback,
                                                    this);
    else
        operation = pa_context_get_sink_info_list(context,
                                                  sinkInfoCallback,
                                                  this);

    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(this->m_mainLoop);

    pa_operation_unref(operation);

    // Convert device format to a supported one.
    *sampleFormat = sampleFormats->key(this->m_defaultFormat,
                                       QbAudioCaps::SampleFormat_u8);
    *channels = this->m_defaultChannels;
    *sampleRate = this->m_defaultRate;

    // Stop server.
    pa_context_disconnect(context);
    pa_context_unref(context);
    pa_threaded_mainloop_unlock(this->m_mainLoop);
    pa_threaded_mainloop_stop(this->m_mainLoop);
    pa_threaded_mainloop_free(this->m_mainLoop);

    return true;
}

bool AudioDevice::init(DeviceMode mode,
                       QbAudioCaps::SampleFormat sampleFormat,
                       int channels,
                       int sampleRate)
{
    int error;

    pa_sample_spec ss;
    ss.format = sampleFormats->value(sampleFormat);
    ss.channels = channels;
    ss.rate = sampleRate;
    this->m_curBps = bytesPerSample->value(sampleFormat);
    this->m_curChannels = channels;

    this->m_paSimple = pa_simple_new(NULL,
                                     QCoreApplication::applicationName().toStdString().c_str(),
                                     mode == DeviceModeCapture? PA_STREAM_RECORD: PA_STREAM_PLAYBACK,
                                     NULL,
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

QByteArray AudioDevice::read(int samples)
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
                       buffer.size(),
                       &error) < 0) {
        this->m_error = QString(pa_strerror(error));
        emit this->errorChanged(this->m_error);

        return QByteArray();
    }

    return buffer;
}

bool AudioDevice::write(const QByteArray &frame)
{
    if (!this->m_paSimple)
        return false;

    int error;

    if (pa_simple_write(this->m_paSimple,
                        frame.data(),
                        frame.size(),
                        &error) < 0) {
        this->m_error = QString(pa_strerror(error));
        emit this->errorChanged(this->m_error);

        return false;
    }

    return true;
}

bool AudioDevice::uninit()
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
    this->m_defaultFormat = PA_SAMPLE_INVALID;
    this->m_defaultChannels = 0;
    this->m_defaultRate = 0;
    this->m_curBps = 0;
    this->m_curChannels = 0;

    return ok;
}

void AudioDevice::contextStateCallbackInit(pa_context *context, void *userdata)
{
    Q_UNUSED(context)

    AudioDevice *audioDevice = reinterpret_cast<AudioDevice *>(userdata);

    // Return as soon as possible.
    pa_threaded_mainloop_signal(audioDevice->m_mainLoop, 0);
}

void AudioDevice::serverInfoCallback(pa_context *context,
                                     const pa_server_info *info,
                                     void *userdata)
{
    Q_UNUSED(context)

    // Get default input and output devices.
    AudioDevice *audioDevice = static_cast<AudioDevice *>(userdata);
    audioDevice->m_defaultSink = info->default_sink_name;
    audioDevice->m_defaultSource = info->default_source_name;

    // Return as soon as possible.
    pa_threaded_mainloop_signal(audioDevice->m_mainLoop, 0);
}

void AudioDevice::sourceInfoCallback(pa_context *context,
                                     const pa_source_info *info,
                                     int isLast,
                                     void *userdata)
{
    AudioDevice *audioDevice = reinterpret_cast<AudioDevice *>(userdata);

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

    // Get info for the default source.
    if (audioDevice->m_defaultSource == QString(info->name)) {
        audioDevice->m_defaultFormat = info->sample_spec.format;
        audioDevice->m_defaultChannels = info->sample_spec.channels;
        audioDevice->m_defaultRate = info->sample_spec.rate;
    }
}

void AudioDevice::sinkInfoCallback(pa_context *context,
                                   const pa_sink_info *info,
                                   int isLast,
                                   void *userdata)
{
    AudioDevice *audioDevice = reinterpret_cast<AudioDevice *>(userdata);

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

    // Get info for the default sink.
    if (audioDevice->m_defaultSink == QString(info->name)) {
        audioDevice->m_defaultFormat = info->sample_spec.format;
        audioDevice->m_defaultChannels = info->sample_spec.channels;
        audioDevice->m_defaultRate = info->sample_spec.rate;
    }
}
