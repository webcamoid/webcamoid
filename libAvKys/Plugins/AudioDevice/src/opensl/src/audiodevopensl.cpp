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

#include <QDebug>
#include <QMap>
#include <QMutex>
#include <QVector>
#include <QWaitCondition>

#ifdef Q_OS_ANDROID
#include <QtAndroid>
#endif

#include <akaudiopacket.h>
#include <SLES/OpenSLES_Android.h>

#include "audiodevopensl.h"

#define N_BUFFERS 4

using SlErrorToStrMap = QMap<SLresult, QString>;

inline const SlErrorToStrMap initSlErrorToStrMap()
{
    const SlErrorToStrMap slErrorToStrMap {
        {SL_RESULT_SUCCESS               , "SL_RESULT_SUCCESS"               },
        {SL_RESULT_PRECONDITIONS_VIOLATED, "SL_RESULT_PRECONDITIONS_VIOLATED"},
        {SL_RESULT_PARAMETER_INVALID     , "SL_RESULT_PARAMETER_INVALID"     },
        {SL_RESULT_MEMORY_FAILURE        , "SL_RESULT_MEMORY_FAILURE"        },
        {SL_RESULT_RESOURCE_ERROR        , "SL_RESULT_RESOURCE_ERROR"        },
        {SL_RESULT_RESOURCE_LOST         , "SL_RESULT_RESOURCE_LOST"         },
        {SL_RESULT_IO_ERROR              , "SL_RESULT_IO_ERROR"              },
        {SL_RESULT_BUFFER_INSUFFICIENT   , "SL_RESULT_BUFFER_INSUFFICIENT"   },
        {SL_RESULT_CONTENT_CORRUPTED     , "SL_RESULT_CONTENT_CORRUPTED"     },
        {SL_RESULT_CONTENT_UNSUPPORTED   , "SL_RESULT_CONTENT_UNSUPPORTED"   },
        {SL_RESULT_CONTENT_NOT_FOUND     , "SL_RESULT_CONTENT_NOT_FOUND"     },
        {SL_RESULT_PERMISSION_DENIED     , "SL_RESULT_PERMISSION_DENIED"     },
        {SL_RESULT_FEATURE_UNSUPPORTED   , "SL_RESULT_FEATURE_UNSUPPORTED"   },
        {SL_RESULT_INTERNAL_ERROR        , "SL_RESULT_INTERNAL_ERROR"        },
        {SL_RESULT_UNKNOWN_ERROR         , "SL_RESULT_UNKNOWN_ERROR"         },
        {SL_RESULT_OPERATION_ABORTED     , "SL_RESULT_OPERATION_ABORTED"     },
        {SL_RESULT_CONTROL_LOST          , "SL_RESULT_CONTROL_LOST"          },
    };

    return slErrorToStrMap;
}

Q_GLOBAL_STATIC_WITH_ARGS(SlErrorToStrMap,
                          slErrorToStrMap,
                          (initSlErrorToStrMap()))

class AudioDevOpenSLPrivate
{
    public:
        AudioDevOpenSL *self;
        QString m_error;
        QStringList m_sinks;
        QStringList m_sources;
        QMap<QString, QString> m_pinDescriptionMap;
        QMap<QString, QList<AkAudioCaps::SampleFormat>> m_supportedFormats;
        QMap<QString, QList<AkAudioCaps::ChannelLayout>> m_supportedLayouts;
        QMap<QString, QList<int>> m_supportedSampleRates;
        QMap<QString, AkAudioCaps> m_preferredCaps;
        SLObjectItf m_engine {nullptr};
        SLObjectItf m_outputMix {nullptr};
        SLObjectItf m_audioPlayer {nullptr};
        SLObjectItf m_audioRecorder {nullptr};
        QVector<QByteArray> m_audioBuffers;
        QMutex m_mutex;
        QWaitCondition m_bufferReady;
        QByteArray m_tmpBuffer;
        AkAudioCaps m_curCaps;
        int m_samples {0};

        explicit AudioDevOpenSLPrivate(AudioDevOpenSL *self);
        static SLObjectItf createEngine();
        static SLObjectItf createOutputMix(SLObjectItf engine);
        static SLObjectItf createAudioPlayer(SLObjectItf engine,
                                             SLObjectItf outputMix,
                                             const AkAudioCaps &caps);
        static SLObjectItf createAudioRecorder(SLObjectItf engine,
                                               const AkAudioCaps &caps);
        static SLAndroidDataFormat_PCM_EX dataFormatFromCaps(const AkAudioCaps &caps);
        static SLuint32 channelMaskFromLayout(AkAudioCaps::ChannelLayout layout);
        static void sampleProcess(SLAndroidSimpleBufferQueueItf bufferQueue,
                                  void *context);
        static bool hasAudioCapturePermissions();
        void updateDevices();
};

AudioDevOpenSL::AudioDevOpenSL(QObject *parent):
    AudioDev(parent)
{
    this->d = new AudioDevOpenSLPrivate(this);
    this->d->updateDevices();
}

AudioDevOpenSL::~AudioDevOpenSL()
{
    this->uninit();
    delete this->d;
}

QString AudioDevOpenSL::error() const
{
    return this->d->m_error;
}

QString AudioDevOpenSL::defaultInput()
{
    return this->d->m_sources.value(0);
}

QString AudioDevOpenSL::defaultOutput()
{
    return this->d->m_sinks.value(0);
}

QStringList AudioDevOpenSL::inputs()
{
    return this->d->m_sources;
}

QStringList AudioDevOpenSL::outputs()
{
    return this->d->m_sinks;
}

QString AudioDevOpenSL::description(const QString &device)
{
    return this->d->m_pinDescriptionMap.value(device);
}

AkAudioCaps AudioDevOpenSL::preferredFormat(const QString &device)
{
    return this->d->m_preferredCaps.value(device);
}

QList<AkAudioCaps::SampleFormat> AudioDevOpenSL::supportedFormats(const QString &device)
{
    return this->d->m_supportedFormats.value(device);
}

QList<AkAudioCaps::ChannelLayout> AudioDevOpenSL::supportedChannelLayouts(const QString &device)
{
    return this->d->m_supportedLayouts.value(device);
}

QList<int> AudioDevOpenSL::supportedSampleRates(const QString &device)
{
    return this->d->m_supportedSampleRates.value(device);
}

bool AudioDevOpenSL::init(const QString &device, const AkAudioCaps &caps)
{
    this->d->m_engine = AudioDevOpenSLPrivate::createEngine();

    if (!this->d->m_engine)
        return false;

    this->d->m_samples = qMax(this->latency() * caps.rate() / 1000, 1);
    this->d->m_curCaps = caps;

    if (device == ":openslinput:") {
        this->d->m_audioRecorder =
                AudioDevOpenSLPrivate::createAudioRecorder(this->d->m_engine,
                                                           caps);

        if (!this->d->m_audioRecorder) {
            (*this->d->m_engine)->Destroy(this->d->m_engine);
            this->d->m_engine = nullptr;
            this->d->m_audioBuffers.clear();

            return false;
        }

        SLPlayItf audioRecorder = nullptr;

        if ((*this->d->m_audioRecorder)->GetInterface(this->d->m_audioRecorder,
                                                      SL_IID_RECORD,
                                                      &audioRecorder) != SL_RESULT_SUCCESS) {
            (*this->d->m_audioRecorder)->Destroy(this->d->m_audioRecorder);
            this->d->m_audioRecorder = nullptr;

            (*this->d->m_engine)->Destroy(this->d->m_engine);
            this->d->m_engine = nullptr;
            this->d->m_audioBuffers.clear();

            return false;
        }

        SLAndroidSimpleBufferQueueItf bufferQueue = nullptr;

        if ((*this->d->m_audioRecorder)->GetInterface(this->d->m_audioRecorder,
                                                      SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                                      &bufferQueue) != SL_RESULT_SUCCESS) {
            (*this->d->m_audioRecorder)->Destroy(this->d->m_audioRecorder);
            this->d->m_audioRecorder = nullptr;

            (*this->d->m_engine)->Destroy(this->d->m_engine);
            this->d->m_engine = nullptr;
            this->d->m_audioBuffers.clear();

            return false;
        }

        if ((*bufferQueue)->RegisterCallback(bufferQueue,
                                             AudioDevOpenSLPrivate::sampleProcess,
                                             this->d) != SL_RESULT_SUCCESS) {
            (*this->d->m_audioRecorder)->Destroy(this->d->m_audioRecorder);
            this->d->m_audioRecorder = nullptr;

            (*this->d->m_engine)->Destroy(this->d->m_engine);
            this->d->m_engine = nullptr;
            this->d->m_audioBuffers.clear();

            return false;
        }

        auto bufferSize = this->d->self->latency()
                          * this->d->m_curCaps.bps()
                          * this->d->m_curCaps.channels()
                          * this->d->m_curCaps.rate()
                          / 1000;
        this->d->m_tmpBuffer = {bufferSize, 0};
        AudioDevOpenSLPrivate::sampleProcess(bufferQueue, this->d);

        if ((*audioRecorder)->SetPlayState(audioRecorder,
                                           SL_RECORDSTATE_RECORDING) != SL_RESULT_SUCCESS) {
            (*this->d->m_audioRecorder)->Destroy(this->d->m_audioRecorder);
            this->d->m_audioRecorder = nullptr;

            (*this->d->m_engine)->Destroy(this->d->m_engine);
            this->d->m_engine = nullptr;
            this->d->m_audioBuffers.clear();

            return false;
        }
    } else {
        this->d->m_outputMix =
                AudioDevOpenSLPrivate::createOutputMix(this->d->m_engine);

        if (!this->d->m_outputMix) {
            (*this->d->m_engine)->Destroy(this->d->m_engine);
            this->d->m_engine = nullptr;
            this->d->m_audioBuffers.clear();

            return false;
        }

        this->d->m_audioPlayer =
                AudioDevOpenSLPrivate::createAudioPlayer(this->d->m_engine,
                                                         this->d->m_outputMix,
                                                         caps);

        if (!this->d->m_audioPlayer) {
            if (this->d->m_outputMix) {
                (*this->d->m_outputMix)->Destroy(this->d->m_outputMix);
                this->d->m_outputMix = nullptr;
            }

            (*this->d->m_engine)->Destroy(this->d->m_engine);
            this->d->m_engine = nullptr;
            this->d->m_audioBuffers.clear();

            return false;
        }

        SLPlayItf audioPlayer = nullptr;

        if ((*this->d->m_audioPlayer)->GetInterface(this->d->m_audioPlayer,
                                                    SL_IID_PLAY,
                                                    &audioPlayer) != SL_RESULT_SUCCESS) {
            if (this->d->m_audioPlayer) {
                (*this->d->m_audioPlayer)->Destroy(this->d->m_audioPlayer);
                this->d->m_audioPlayer = nullptr;
            }

            if (this->d->m_outputMix) {
                (*this->d->m_outputMix)->Destroy(this->d->m_outputMix);
                this->d->m_outputMix = nullptr;
            }

            (*this->d->m_engine)->Destroy(this->d->m_engine);
            this->d->m_engine = nullptr;
            this->d->m_audioBuffers.clear();

            return false;
        }

        SLAndroidSimpleBufferQueueItf bufferQueue = nullptr;

        if ((*this->d->m_audioPlayer)->GetInterface(this->d->m_audioPlayer,
                                                    SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                                    &bufferQueue) != SL_RESULT_SUCCESS) {
            if (this->d->m_audioPlayer) {
                (*this->d->m_audioPlayer)->Destroy(this->d->m_audioPlayer);
                this->d->m_audioPlayer = nullptr;
            }

            if (this->d->m_outputMix) {
                (*this->d->m_outputMix)->Destroy(this->d->m_outputMix);
                this->d->m_outputMix = nullptr;
            }

            (*this->d->m_engine)->Destroy(this->d->m_engine);
            this->d->m_engine = nullptr;
            this->d->m_audioBuffers.clear();

            return false;
        }

        if ((*bufferQueue)->RegisterCallback(bufferQueue,
                                             AudioDevOpenSLPrivate::sampleProcess,
                                             this->d) != SL_RESULT_SUCCESS) {
            if (this->d->m_audioPlayer) {
                (*this->d->m_audioPlayer)->Destroy(this->d->m_audioPlayer);
                this->d->m_audioPlayer = nullptr;
            }

            if (this->d->m_outputMix) {
                (*this->d->m_outputMix)->Destroy(this->d->m_outputMix);
                this->d->m_outputMix = nullptr;
            }

            (*this->d->m_engine)->Destroy(this->d->m_engine);
            this->d->m_engine = nullptr;
            this->d->m_audioBuffers.clear();

            return false;
        }

        if ((*audioPlayer)->SetPlayState(audioPlayer,
                                         SL_PLAYSTATE_PLAYING) != SL_RESULT_SUCCESS) {
            if (this->d->m_audioPlayer) {
                (*this->d->m_audioPlayer)->Destroy(this->d->m_audioPlayer);
                this->d->m_audioPlayer = nullptr;
            }

            if (this->d->m_outputMix) {
                (*this->d->m_outputMix)->Destroy(this->d->m_outputMix);
                this->d->m_outputMix = nullptr;
            }

            (*this->d->m_engine)->Destroy(this->d->m_engine);
            this->d->m_engine = nullptr;
            this->d->m_audioBuffers.clear();

            return false;
        }

        AudioDevOpenSLPrivate::sampleProcess(bufferQueue, this->d);
    }

    return true;
}

QByteArray AudioDevOpenSL::read()
{
    this->d->m_mutex.lock();

    if (this->d->m_audioBuffers.size() < 1)
        this->d->m_bufferReady.wait(&this->d->m_mutex);

    auto buffer = this->d->m_audioBuffers.takeFirst();
    this->d->m_mutex.unlock();

    return buffer;
}

bool AudioDevOpenSL::write(const AkAudioPacket &packet)
{
    this->d->m_mutex.lock();

    if (this->d->m_audioBuffers.size() >= N_BUFFERS)
        this->d->m_bufferReady.wait(&this->d->m_mutex);

    this->d->m_audioBuffers << QByteArray(packet.constData(), packet.size());
    this->d->m_mutex.unlock();

    return true;
}

bool AudioDevOpenSL::uninit()
{
    if (this->d->m_audioRecorder) {
        SLRecordItf audioRecorder = nullptr;

        if ((*this->d->m_audioRecorder)->GetInterface(this->d->m_audioRecorder,
                                                      SL_IID_RECORD,
                                                      &audioRecorder) == SL_RESULT_SUCCESS) {
            (*audioRecorder)->SetRecordState(audioRecorder, SL_RECORDSTATE_STOPPED);
        }

        (*this->d->m_audioRecorder)->Destroy(this->d->m_audioRecorder);
        this->d->m_audioRecorder = nullptr;
    }

    if (this->d->m_audioPlayer) {
        SLPlayItf audioPlayer = nullptr;

        if ((*this->d->m_audioPlayer)->GetInterface(this->d->m_audioPlayer,
                                                    SL_IID_PLAY,
                                                    &audioPlayer) == SL_RESULT_SUCCESS) {
            (*audioPlayer)->SetPlayState(audioPlayer, SL_PLAYSTATE_STOPPED);
        }

        (*this->d->m_audioPlayer)->Destroy(this->d->m_audioPlayer);
        this->d->m_audioPlayer = nullptr;
    }

    if (this->d->m_outputMix) {
        (*this->d->m_outputMix)->Destroy(this->d->m_outputMix);
        this->d->m_outputMix = nullptr;
    }

    if (this->d->m_engine) {
        (*this->d->m_engine)->Destroy(this->d->m_engine);
        this->d->m_engine = nullptr;
    }

    this->d->m_audioBuffers.clear();

    return true;
}

AudioDevOpenSLPrivate::AudioDevOpenSLPrivate(AudioDevOpenSL *self):
    self(self)
{
}

SLObjectItf AudioDevOpenSLPrivate::createEngine()
{
    SLObjectItf engineObject = nullptr;
    auto result = slCreateEngine(&engineObject,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 nullptr);

    if (result != SL_RESULT_SUCCESS) {
        qInfo() << "Failed to create OpenSL engine:"
                << slErrorToStrMap->value(result);

        return nullptr;
    }

    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);

    if (result != SL_RESULT_SUCCESS) {
        (*engineObject)->Destroy(engineObject);
        qInfo() << "Can't realize OpenSL engine:"
                << slErrorToStrMap->value(result);

        return nullptr;
    }

    return engineObject;
}

SLObjectItf AudioDevOpenSLPrivate::createOutputMix(SLObjectItf engine)
{
    SLEngineItf engineInterface = nullptr;
    auto result = (*engine)->GetInterface(engine,
                                          SL_IID_ENGINE,
                                          &engineInterface);

    if (result != SL_RESULT_SUCCESS) {
        qInfo() << "Can't get engine interface:"
                << slErrorToStrMap->value(result);

        return nullptr;
    }

    SLObjectItf outputMixObject = nullptr;
    result = (*engineInterface)->CreateOutputMix(engineInterface,
                                                 &outputMixObject,
                                                 0,
                                                 nullptr,
                                                 nullptr);

    if (result != SL_RESULT_SUCCESS) {
        qInfo() << "Can't create OutputMix:"
                << slErrorToStrMap->value(result);

        return nullptr;
    }

    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);

    if (result != SL_RESULT_SUCCESS) {
        (*outputMixObject)->Destroy(outputMixObject);
        qInfo() << "Can't realize OutputMix:"
                << slErrorToStrMap->value(result);

        return nullptr;
    }

    return outputMixObject;
}

SLObjectItf AudioDevOpenSLPrivate::createAudioPlayer(SLObjectItf engine,
                                                     SLObjectItf outputMix,
                                                     const AkAudioCaps &caps)
{
    if (!outputMix)
        return nullptr;

    SLEngineItf engineInterface = nullptr;
    auto result = (*engine)->GetInterface(engine,
                                          SL_IID_ENGINE,
                                          &engineInterface);

    if (result != SL_RESULT_SUCCESS) {
        qInfo() << "Can't get engine interface:"
                << slErrorToStrMap->value(result);

        return nullptr;
    }

    SLObjectItf audioPlayerObject = nullptr;

    SLDataLocator_AndroidSimpleBufferQueue bufferQueueLocator {
        SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
        N_BUFFERS
    };
    auto format = AudioDevOpenSLPrivate::dataFormatFromCaps(caps);
    SLDataSource dataSource {
        &bufferQueueLocator,
        &format
    };
    SLDataLocator_OutputMix outputMixLocator {
        SL_DATALOCATOR_OUTPUTMIX,
        outputMix
    };
    SLDataSink dataSink {
        &outputMixLocator,
        nullptr
    };

    QMap<SLInterfaceID, SLboolean> interfaces {
        {SL_IID_BUFFERQUEUE, SL_BOOLEAN_TRUE},
    };
    auto requiredKeys = interfaces.keys().toVector();
    auto requiredValues = interfaces.values().toVector();
    result = (*engineInterface)->CreateAudioPlayer(engineInterface,
                                                   &audioPlayerObject,
                                                   &dataSource,
                                                   &dataSink,
                                                   SLuint32(interfaces.size()),
                                                   requiredKeys.data(),
                                                   requiredValues.data());

    if (result != SL_RESULT_SUCCESS) {
        qInfo() << "Can't create AudioPlayer:"
                << slErrorToStrMap->value(result);

        return nullptr;
    }

    result = (*audioPlayerObject)->Realize(audioPlayerObject, SL_BOOLEAN_FALSE);

    if (result != SL_RESULT_SUCCESS) {
        (*audioPlayerObject)->Destroy(audioPlayerObject);
        qInfo() << "Can't realize AudioPlayer:"
                << slErrorToStrMap->value(result);

        return nullptr;
    }

    return audioPlayerObject;
}

SLObjectItf AudioDevOpenSLPrivate::createAudioRecorder(SLObjectItf engine,
                                                       const AkAudioCaps &caps)
{
    SLEngineItf engineInterface = nullptr;
    auto result = (*engine)->GetInterface(engine,
                                          SL_IID_ENGINE,
                                          &engineInterface);

    if (result != SL_RESULT_SUCCESS) {
        qInfo() << "Can't get engine interface:"
                << slErrorToStrMap->value(result);

        return nullptr;
    }

    SLObjectItf audioRecorderObject = nullptr;
    SLDataLocator_IODevice ioDeviceLocator {
        SL_DATALOCATOR_IODEVICE,
        SL_IODEVICE_AUDIOINPUT,
        SL_DEFAULTDEVICEID_AUDIOINPUT,
        nullptr
    };
    SLDataLocator_AndroidSimpleBufferQueue bufferQueueLocator {
        SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
        N_BUFFERS
    };
    auto format = AudioDevOpenSLPrivate::dataFormatFromCaps(caps);
    SLDataSource dataSource {
        &ioDeviceLocator,
        nullptr
    };
    SLDataSink dataSink {
        &bufferQueueLocator,
        &format
    };
    QMap<SLInterfaceID, SLboolean> interfaces {
        {SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_BOOLEAN_TRUE},
        {SL_IID_ANDROIDCONFIGURATION    , SL_BOOLEAN_TRUE},
    };
    auto requiredKeys = interfaces.keys().toVector();
    auto requiredValues = interfaces.values().toVector();
    result = (*engineInterface)->CreateAudioRecorder(engineInterface,
                                                     &audioRecorderObject,
                                                     &dataSource,
                                                     &dataSink,
                                                     SLuint32(interfaces.size()),
                                                     requiredKeys.data(),
                                                     requiredValues.data());

    if (result != SL_RESULT_SUCCESS) {
        qInfo() << "Can't create AudioRecorder:"
                << slErrorToStrMap->value(result);

        return nullptr;
    }

    result = (*audioRecorderObject)->Realize(audioRecorderObject,
                                             SL_BOOLEAN_FALSE);

    if (result != SL_RESULT_SUCCESS) {
        (*audioRecorderObject)->Destroy(audioRecorderObject);
        qInfo() << "Can't realize AudioRecorder:"
                << slErrorToStrMap->value(result);

        return nullptr;
    }

    return audioRecorderObject;
}

SLAndroidDataFormat_PCM_EX AudioDevOpenSLPrivate::dataFormatFromCaps(const AkAudioCaps &caps)
{
    SLuint32 bitsPerSample = 0;

    switch (AkAudioCaps::bitsPerSample(caps.format())) {
    case 8:
        bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_8;

        break;

    case 16:
        bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;

        break;

    case 32:
        bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_32;

        break;

    default:
        break;
    }

    SLuint32 representation = 0;

    switch (AkAudioCaps::sampleType(caps.format())) {
    case AkAudioCaps::SampleType_int:
        representation = SL_ANDROID_PCM_REPRESENTATION_SIGNED_INT;

        break;

    case AkAudioCaps::SampleType_uint:
        representation = SL_ANDROID_PCM_REPRESENTATION_UNSIGNED_INT;

        break;

    case AkAudioCaps::SampleType_float:
        representation = SL_ANDROID_PCM_REPRESENTATION_FLOAT;

        break;

    default:
        break;
    }

    return {
        SL_ANDROID_DATAFORMAT_PCM_EX,
        SLuint32(caps.channels()),
        1000 * SLuint32(caps.rate()),
        bitsPerSample,
        bitsPerSample,
        AudioDevOpenSLPrivate::channelMaskFromLayout(caps.layout()),
        AkAudioCaps::endianness(caps.format()) == Q_LITTLE_ENDIAN?
                    SL_BYTEORDER_LITTLEENDIAN: SL_BYTEORDER_BIGENDIAN,
        representation
    };
}

SLuint32 AudioDevOpenSLPrivate::channelMaskFromLayout(AkAudioCaps::ChannelLayout layout)
{
    static const QMap<AkAudioCaps::Position, SLuint32> positiosMap {
        {AkAudioCaps::Position_FrontLeft         , SL_SPEAKER_FRONT_LEFT           },
        {AkAudioCaps::Position_FrontRight        , SL_SPEAKER_FRONT_RIGHT          },
        {AkAudioCaps::Position_FrontCenter       , SL_SPEAKER_FRONT_CENTER         },
        {AkAudioCaps::Position_LowFrequency1     , SL_SPEAKER_LOW_FREQUENCY        },
        {AkAudioCaps::Position_BackLeft          , SL_SPEAKER_BACK_LEFT            },
        {AkAudioCaps::Position_BackRight         , SL_SPEAKER_BACK_RIGHT           },
        {AkAudioCaps::Position_FrontLeftOfCenter , SL_SPEAKER_FRONT_LEFT_OF_CENTER },
        {AkAudioCaps::Position_FrontRightOfCenter, SL_SPEAKER_FRONT_RIGHT_OF_CENTER},
        {AkAudioCaps::Position_BackCenter        , SL_SPEAKER_BACK_CENTER          },
        {AkAudioCaps::Position_SideLeft          , SL_SPEAKER_SIDE_LEFT            },
        {AkAudioCaps::Position_SideRight         , SL_SPEAKER_SIDE_RIGHT           },
        {AkAudioCaps::Position_TopCenter         , SL_SPEAKER_TOP_CENTER           },
        {AkAudioCaps::Position_TopFrontLeft      , SL_SPEAKER_TOP_FRONT_LEFT       },
        {AkAudioCaps::Position_TopFrontCenter    , SL_SPEAKER_TOP_FRONT_CENTER     },
        {AkAudioCaps::Position_TopFrontRight     , SL_SPEAKER_TOP_FRONT_RIGHT      },
        {AkAudioCaps::Position_TopBackLeft       , SL_SPEAKER_TOP_BACK_LEFT        },
        {AkAudioCaps::Position_TopBackCenter     , SL_SPEAKER_TOP_BACK_CENTER      },
        {AkAudioCaps::Position_TopBackRight      , SL_SPEAKER_TOP_BACK_RIGHT       },
    };
    SLuint32 channelMask = 0;

    for (auto &position: AkAudioCaps::positions(layout))
        channelMask |= positiosMap.value(position, 0);

    return channelMask;
}

void AudioDevOpenSLPrivate::sampleProcess(SLAndroidSimpleBufferQueueItf bufferQueue,
                                          void *context)
{
    auto self = reinterpret_cast<AudioDevOpenSLPrivate *>(context);
    QByteArray buffer;

    if (self->m_audioPlayer) {
        self->m_mutex.lock();

        if (self->m_audioBuffers.isEmpty()) {
            auto bufferSize = self->self->latency()
                              * self->m_curCaps.bps()
                              * self->m_curCaps.channels()
                              * self->m_curCaps.rate()
                              / 8000;
            buffer = {bufferSize, 0};
        } else {
            buffer = self->m_audioBuffers.takeFirst();
        }

        self->m_bufferReady.wakeAll();
        self->m_mutex.unlock();

        (*bufferQueue)->Enqueue(bufferQueue,
                                buffer.data(),
                                SLuint32(buffer.size()));
    } else {
        self->m_mutex.lock();
        self->m_audioBuffers << self->m_tmpBuffer;

        if (self->m_audioBuffers.size() > N_BUFFERS)
            self->m_audioBuffers.takeFirst();

        self->m_bufferReady.wakeAll();
        self->m_mutex.unlock();

        (*bufferQueue)->Enqueue(bufferQueue,
                                self->m_tmpBuffer.data(),
                                SLuint32(self->m_tmpBuffer.size()));
    }
}

bool AudioDevOpenSLPrivate::hasAudioCapturePermissions()
{
#ifdef Q_OS_ANDROID
    static bool done = false;
    static bool result = false;

    if (done)
        return result;

    QStringList permissions {
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

void AudioDevOpenSLPrivate::updateDevices()
{
    auto engine = AudioDevOpenSLPrivate::createEngine();

    if (!engine)
        return;

    static const QVector<AkAudioCaps::SampleFormat> sampleFormats {
        AkAudioCaps::SampleFormat_s8,
        AkAudioCaps::SampleFormat_u8,
        AkAudioCaps::SampleFormat_s16,
        AkAudioCaps::SampleFormat_u16,
        AkAudioCaps::SampleFormat_s32,
        AkAudioCaps::SampleFormat_u32,
        AkAudioCaps::SampleFormat_s64,
        AkAudioCaps::SampleFormat_u64,
        AkAudioCaps::SampleFormat_flt,
        AkAudioCaps::SampleFormat_dbl,
    };
    static const QVector<AkAudioCaps::ChannelLayout> layouts {
        AkAudioCaps::Layout_mono,
        AkAudioCaps::Layout_stereo,
    };
    static const QVector<int> sampleRates {
        SL_SAMPLINGRATE_8      / 1000,
        SL_SAMPLINGRATE_11_025 / 1000,
        SL_SAMPLINGRATE_12     / 1000,
        SL_SAMPLINGRATE_16     / 1000,
        SL_SAMPLINGRATE_22_05  / 1000,
        SL_SAMPLINGRATE_24     / 1000,
        SL_SAMPLINGRATE_32     / 1000,
        SL_SAMPLINGRATE_44_1   / 1000,
        SL_SAMPLINGRATE_48     / 1000,
        SL_SAMPLINGRATE_64     / 1000,
        SL_SAMPLINGRATE_88_2   / 1000,
        SL_SAMPLINGRATE_96     / 1000,
        SL_SAMPLINGRATE_192	   / 1000,
    };

    if (this->hasAudioCapturePermissions()) {
        // Test audio input
        for (auto &format: sampleFormats)
            for (auto &layout: layouts)
                for (auto &rate: sampleRates) {
                    AkAudioCaps caps(format, layout, false, rate);
                    auto audioRecorder =
                            AudioDevOpenSLPrivate::createAudioRecorder(engine, caps);

                    if (audioRecorder) {
                        if (!this->m_supportedFormats[":openslinput:"].contains(format))
                            this->m_supportedFormats[":openslinput:"] << format;

                        if (!this->m_supportedLayouts[":openslinput:"].contains(layout))
                            this->m_supportedLayouts[":openslinput:"] << layout;

                        if (!this->m_supportedSampleRates[":openslinput:"].contains(rate))
                            this->m_supportedSampleRates[":openslinput:"] << rate;

                        (*audioRecorder)->Destroy(audioRecorder);
                    }
                }

        if (this->m_supportedFormats.contains(":openslinput:")
            && this->m_supportedLayouts.contains(":openslinput:")
            && this->m_supportedSampleRates.contains(":openslinput:")) {
            this->m_sources = QStringList {":openslinput:"};
            this->m_pinDescriptionMap[":openslinput:"] = "OpenSL ES Input";
            this->m_preferredCaps[":openslinput:"] = {
                AkAudioCaps::SampleFormat_s16,
                AkAudioCaps::Layout_mono,
                false,
                44100,
            };
        }
    }

    // Test audio output
    auto outputMix = AudioDevOpenSLPrivate::createOutputMix(engine);

    if (outputMix) {
        for (auto &format: sampleFormats)
            for (auto &layout: layouts)
                for (auto &rate: sampleRates) {
                    AkAudioCaps caps(format, layout, false, rate);
                    auto audioPlayer =
                            AudioDevOpenSLPrivate::createAudioPlayer(engine,
                                                                     outputMix,
                                                                     caps);

                    if (audioPlayer) {
                        if (!this->m_supportedFormats[":opensloutput:"].contains(format))
                            this->m_supportedFormats[":opensloutput:"] << format;

                        if (!this->m_supportedLayouts[":opensloutput:"].contains(layout))
                            this->m_supportedLayouts[":opensloutput:"] << layout;

                        if (!this->m_supportedSampleRates[":opensloutput:"].contains(rate))
                            this->m_supportedSampleRates[":opensloutput:"] << rate;

                        (*audioPlayer)->Destroy(audioPlayer);
                    }
                }

        if (this->m_supportedFormats.contains(":opensloutput:")
            && this->m_supportedLayouts.contains(":opensloutput:")
            && this->m_supportedSampleRates.contains(":opensloutput:")) {
            this->m_sinks = QStringList {":opensloutput:"};
            this->m_pinDescriptionMap[":opensloutput:"] = "OpenSL ES Output";
            this->m_preferredCaps[":opensloutput:"] = {
                AkAudioCaps::SampleFormat_s16,
                AkAudioCaps::Layout_stereo,
                false,
                44100,
            };
        }

        (*outputMix)->Destroy(outputMix);
    }

    (*engine)->Destroy(engine);
}

#include "moc_audiodevopensl.cpp"
