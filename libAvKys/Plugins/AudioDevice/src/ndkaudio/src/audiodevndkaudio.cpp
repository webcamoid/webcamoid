/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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
#include <QVector>
#include <QMutex>
#include <QWaitCondition>

#ifdef Q_OS_ANDROID
#include <QtAndroid>
#endif

#include <akaudiopacket.h>
#include <aaudio/AAudio.h>

#include "audiodevndkaudio.h"

#define N_BUFFERS 4

class AudioDevNDKAudioPrivate
{
    public:
        AudioDevNDKAudio *self;
        QString m_error;
        QStringList m_sinks;
        QStringList m_sources;
        QMap<QString, QString> m_pinDescriptionMap;
        QMap<QString, QList<AkAudioCaps::SampleFormat>> m_supportedFormats;
        QMap<QString, QList<AkAudioCaps::ChannelLayout>> m_supportedLayouts;
        QMap<QString, QList<int>> m_supportedSampleRates;
        QMap<QString, AkAudioCaps> m_preferredCaps;
        QMutex m_mutex;
        AAudioStreamBuilder *m_streamBuilder {nullptr};
        AAudioStream *m_stream {nullptr};
        int m_samples {0};

        explicit AudioDevNDKAudioPrivate(AudioDevNDKAudio *self);
        AAudioStream *createStream(AAudioStreamBuilder *streamBuilder,
                                   aaudio_direction_t direction,
                                   const AkAudioCaps &caps);
        static void errorCallback(AAudioStream *stream,
                                  void *userData,
                                  aaudio_result_t error);
        static bool hasAudioPermissions();
        void updateDevices();
};

AudioDevNDKAudio::AudioDevNDKAudio(QObject *parent):
    AudioDev(parent)
{
    this->d = new AudioDevNDKAudioPrivate(this);
    this->d->updateDevices();
}

AudioDevNDKAudio::~AudioDevNDKAudio()
{
    this->uninit();
    delete this->d;
}

QString AudioDevNDKAudio::error() const
{
    return this->d->m_error;
}

QString AudioDevNDKAudio::defaultInput()
{
    return this->d->m_sources.value(0);
}

QString AudioDevNDKAudio::defaultOutput()
{
    return this->d->m_sinks.value(0);
}

QStringList AudioDevNDKAudio::inputs()
{
    return this->d->m_sources;
}

QStringList AudioDevNDKAudio::outputs()
{
    return this->d->m_sinks;
}

QString AudioDevNDKAudio::description(const QString &device)
{
    return this->d->m_pinDescriptionMap.value(device);
}

AkAudioCaps AudioDevNDKAudio::preferredFormat(const QString &device)
{
    return this->d->m_preferredCaps.value(device);
}

QList<AkAudioCaps::SampleFormat> AudioDevNDKAudio::supportedFormats(const QString &device)
{
    return this->d->m_supportedFormats.value(device);
}

QList<AkAudioCaps::ChannelLayout> AudioDevNDKAudio::supportedChannelLayouts(const QString &device)
{
    return this->d->m_supportedLayouts.value(device);
}

QList<int> AudioDevNDKAudio::supportedSampleRates(const QString &device)
{
    return this->d->m_supportedSampleRates.value(device);
}

bool AudioDevNDKAudio::init(const QString &device, const AkAudioCaps &caps)
{
    aaudio_direction_t direction = AAUDIO_DIRECTION_OUTPUT;

    if (device == ":aaudioinput:")
        direction = AAUDIO_DIRECTION_INPUT;
    else if (device != ":aaudiooutput:")
        return false;

    if (AAudio_createStreamBuilder(&this->d->m_streamBuilder) != AAUDIO_OK)
        return false;

    this->d->m_stream = this->d->createStream(this->d->m_streamBuilder,
                                              direction,
                                              caps);

    if (!this->d->m_stream) {
        AAudioStreamBuilder_delete(this->d->m_streamBuilder);
        this->d->m_streamBuilder = nullptr;

        return false;
    }

    if (AAudioStream_requestStart(this->d->m_stream) != AAUDIO_OK) {
        AAudioStream_close(this->d->m_stream);
        this->d->m_stream = nullptr;

        AAudioStreamBuilder_delete(this->d->m_streamBuilder);
        this->d->m_streamBuilder = nullptr;

        return false;
    }

    return true;
}

QByteArray AudioDevNDKAudio::read()
{
    static const QMap<aaudio_format_t, size_t> fmtToSampleSize {
        {AAUDIO_FORMAT_PCM_I16  , sizeof(qint16)},
        {AAUDIO_FORMAT_PCM_FLOAT, sizeof(float) },
    };

    auto format = AAudioStream_getFormat(this->d->m_stream);
    auto samples = this->latency()
                   * AAudioStream_getSampleRate(this->d->m_stream)
                   / 1000;

    if (samples < 1)
        samples = 1;

    auto bufferSize = int(fmtToSampleSize.value(format))
                      * AAudioStream_getChannelCount(this->d->m_stream)
                      * samples;
    QByteArray buffer(bufferSize, Qt::Uninitialized);
    samples = AAudioStream_read(this->d->m_stream,
                                buffer.data(),
                                samples,
                                500e6);

    if (samples < 1)
        return {};

    bufferSize = int(fmtToSampleSize.value(format))
                 * AAudioStream_getChannelCount(this->d->m_stream)
                 * samples;
    buffer.resize(bufferSize);

    return buffer;
}

bool AudioDevNDKAudio::write(const AkAudioPacket &packet)
{
    if (AAudioStream_write(this->d->m_stream,
                           packet.buffer().constData(),
                           packet.caps().samples(),
                           500e6) != AAUDIO_OK)
        return false;

    return true;
}

bool AudioDevNDKAudio::uninit()
{
    if (this->d->m_stream) {
        AAudioStream_requestStop(this->d->m_stream);

        forever {
            auto state = AAudioStream_getState(this->d->m_stream);

            if (state == AAUDIO_STREAM_STATE_STOPPED)
                break;

            aaudio_stream_state_t curState;
            auto tiemeout = std::numeric_limits<int64_t>::max();
            AAudioStream_waitForStateChange(this->d->m_stream,
                                            state,
                                            &curState,
                                            tiemeout);
        }

        AAudioStream_close(this->d->m_stream);
        this->d->m_stream = nullptr;
    }

    if (this->d->m_streamBuilder) {
        AAudioStreamBuilder_delete(this->d->m_streamBuilder);
        this->d->m_streamBuilder = nullptr;
    }

    return true;
}

AudioDevNDKAudioPrivate::AudioDevNDKAudioPrivate(AudioDevNDKAudio *self):
    self(self)
{
}

AAudioStream *AudioDevNDKAudioPrivate::createStream(AAudioStreamBuilder *streamBuilder,
                                                    aaudio_direction_t direction,
                                                    const AkAudioCaps &caps)
{
    static const QMap<AkAudioCaps::SampleFormat, aaudio_format_t> formatsMap {
        {AkAudioCaps::SampleFormat_s16, AAUDIO_FORMAT_PCM_I16  },
        {AkAudioCaps::SampleFormat_flt, AAUDIO_FORMAT_PCM_FLOAT},
    };

    auto samples = this->self->latency() * caps.rate() / 1000;
    AAudioStreamBuilder_setBufferCapacityInFrames(streamBuilder,
                                                  N_BUFFERS * samples);
    AAudioStreamBuilder_setChannelCount(streamBuilder, caps.channels());
    AAudioStreamBuilder_setDeviceId(streamBuilder, AAUDIO_UNSPECIFIED);
    AAudioStreamBuilder_setDirection(streamBuilder, direction);
    AAudioStreamBuilder_setErrorCallback(streamBuilder,
                                         AudioDevNDKAudioPrivate::errorCallback,
                                         this);
    AAudioStreamBuilder_setFormat(streamBuilder,
                                  formatsMap.value(caps.format(),
                                                   AAUDIO_FORMAT_INVALID));
    AAudioStreamBuilder_setFramesPerDataCallback(streamBuilder, samples);
    AAudioStreamBuilder_setPerformanceMode(streamBuilder,
                                           AAUDIO_PERFORMANCE_MODE_NONE);
    AAudioStreamBuilder_setSampleRate(streamBuilder, caps.rate());
    AAudioStreamBuilder_setSamplesPerFrame(streamBuilder, caps.channels());
    AAudioStreamBuilder_setSharingMode(streamBuilder,
                                       AAUDIO_SHARING_MODE_SHARED);
    AAudioStream *stream = nullptr;

    if (AAudioStreamBuilder_openStream(streamBuilder, &stream) != AAUDIO_OK)
        return nullptr;

    return stream;
}

void AudioDevNDKAudioPrivate::errorCallback(AAudioStream *stream,
                                            void *userData,
                                            aaudio_result_t error)
{
    Q_UNUSED(stream)
    Q_UNUSED(userData)
    Q_UNUSED(error)
}

bool AudioDevNDKAudioPrivate::hasAudioPermissions()
{
#ifdef Q_OS_ANDROID
    static bool done = false;
    static bool result = false;

    if (done)
        return result;

    QStringList permissions {
        "android.permission.CAPTURE_AUDIO_OUTPUT",
        "android.permission.RECORD_AUDIO"
    };
    QStringList neededPermissions;

    for (auto &permission: permissions)
        if (QtAndroid::checkPermission(permission) == QtAndroid::PermissionResult::Denied)
            neededPermissions << permission;

    if (!neededPermissions.isEmpty()) {
        auto results = QtAndroid::requestPermissionsSync(neededPermissions);

        for (auto it = results.constBegin(); it != results.constEnd(); it++)
            if (it.value() == QtAndroid::PermissionResult::Denied) {
                done = true;

                return false;
            }
    }

    done = true;
    result = true;
#endif

    return true;
}

void AudioDevNDKAudioPrivate::updateDevices()
{
    if (!this->hasAudioPermissions())
        return;

    AAudioStreamBuilder *streamBuilder = nullptr;

    if (AAudio_createStreamBuilder(&streamBuilder) != AAUDIO_OK)
        return;

    static const QVector<AkAudioCaps::SampleFormat> sampleFormats {
        AkAudioCaps::SampleFormat_s16,
        AkAudioCaps::SampleFormat_flt,
    };
    static const QVector<AkAudioCaps::ChannelLayout> layouts {
        AkAudioCaps::Layout_mono,
        AkAudioCaps::Layout_stereo,
    };

    // Test audio input
    for (auto &format: sampleFormats)
        for (auto &layout: layouts)
            for (auto &rate: this->self->commonSampleRates()) {
                AkAudioCaps caps(format, layout, rate);
                auto stream = this->createStream(streamBuilder,
                                                 AAUDIO_DIRECTION_INPUT,
                                                 caps);

                if (stream) {
                    if (!this->m_supportedFormats[":aaudioinput:"].contains(format))
                        this->m_supportedFormats[":aaudioinput:"] << format;

                    if (!this->m_supportedLayouts[":aaudioinput:"].contains(layout))
                        this->m_supportedLayouts[":aaudioinput:"] << layout;

                    if (!this->m_supportedSampleRates[":aaudioinput:"].contains(rate))
                        this->m_supportedSampleRates[":aaudioinput:"] << rate;

                    AAudioStream_close(stream);
                }
            }

    if (this->m_supportedFormats.contains(":aaudioinput:")
        && this->m_supportedLayouts.contains(":aaudioinput:")
        && this->m_supportedSampleRates.contains(":aaudioinput:")) {
        this->m_sources = QStringList {":aaudioinput:"};
        this->m_pinDescriptionMap[":aaudioinput:"] = "Android Audio Input";
        this->m_preferredCaps[":aaudioinput:"] = {
            AkAudioCaps::SampleFormat_s16,
            AkAudioCaps::Layout_mono,
            44100,
        };
    }

    // Test audio output
    for (auto &format: sampleFormats)
        for (auto &layout: layouts)
            for (auto &rate: this->self->commonSampleRates()) {
                AkAudioCaps caps(format, layout, rate);
                auto stream = this->createStream(streamBuilder,
                                                 AAUDIO_DIRECTION_OUTPUT,
                                                 caps);

                if (stream) {
                    if (!this->m_supportedFormats[":aaudiooutput:"].contains(format))
                        this->m_supportedFormats[":aaudiooutput:"] << format;

                    if (!this->m_supportedLayouts[":aaudiooutput:"].contains(layout))
                        this->m_supportedLayouts[":aaudiooutput:"] << layout;

                    if (!this->m_supportedSampleRates[":aaudiooutput:"].contains(rate))
                        this->m_supportedSampleRates[":aaudiooutput:"] << rate;

                    AAudioStream_close(stream);
                }
            }

    if (this->m_supportedFormats.contains(":aaudiooutput:")
        && this->m_supportedLayouts.contains(":aaudiooutput:")
        && this->m_supportedSampleRates.contains(":aaudiooutput:")) {
        this->m_sinks = QStringList {":aaudiooutput:"};
        this->m_pinDescriptionMap[":aaudiooutput:"] = "Android Audio Output";
        this->m_preferredCaps[":aaudiooutput:"] = {
            AkAudioCaps::SampleFormat_s16,
            AkAudioCaps::Layout_stereo,
            44100,
        };
    }

    AAudioStreamBuilder_delete(streamBuilder);
}

#include "moc_audiodevndkaudio.cpp"
