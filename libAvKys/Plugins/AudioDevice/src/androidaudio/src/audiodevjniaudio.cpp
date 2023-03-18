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
#include <QVector>
#include <QMutex>
#include <QtAndroid>
#include <QAndroidJniEnvironment>
#include <QAndroidJniObject>
#include <akaudiopacket.h>

#include "audiodevjniaudio.h"

#define JNAMESPACE "org/webcamoid/plugins/AudioDevice/submodules/androidaudio"
#define JCLASS(jclass) JNAMESPACE "/" #jclass

// Device Input/Output type

#define GET_DEVICES_INPUTS  0x1
#define GET_DEVICES_OUTPUTS 0x2
#define GET_DEVICES_ALL     (GET_DEVICES_INPUTS | GET_DEVICES_OUTPUTS)

// Read/Write mode

#define READ_BLOCKING     0
#define READ_NON_BLOCKING 1

#define WRITE_BLOCKING     0
#define WRITE_NON_BLOCKING 1

// Performance mode

#define PERFORMANCE_MODE_NONE         0
#define PERFORMANCE_MODE_LOW_LATENCY  1
#define PERFORMANCE_MODE_POWER_SAVING 2

// Attribute flags

#define FLAG_AUDIBILITY_ENFORCED (1 << 0)
#define FLAG_HW_AV_SYNC          (1 << 4)
#define FLAG_LOW_LATENCY         (1 << 8)

// Transfer mode

#define MODE_STREAM 0
#define MODE_STATIC 1

// Usage

#define USAGE_UNKNOWN                         0
#define USAGE_MEDIA                           1
#define USAGE_VOICE_COMMUNICATION             2
#define USAGE_VOICE_COMMUNICATION_SIGNALLING  3
#define USAGE_ALARM                           4
#define USAGE_NOTIFICATION                    5
#define USAGE_NOTIFICATION_RINGTONE           6
#define USAGE_NOTIFICATION_EVENT             10
#define USAGE_ASSISTANCE_ACCESSIBILITY       11
#define USAGE_ASSISTANCE_NAVIGATION_GUIDANCE 12
#define USAGE_ASSISTANCE_SONIFICATION        13
#define USAGE_GAME                           14
#define USAGE_ASSISTANT                      16

// Content type

#define CONTENT_TYPE_UNKNOWN      0
#define CONTENT_TYPE_SPEECH       1
#define CONTENT_TYPE_MUSIC        2
#define CONTENT_TYPE_MOVIE        3
#define CONTENT_TYPE_SONIFICATION 4

// Channel mask

#define CHANNEL_OUT_DEFAULT                        1
#define CHANNEL_OUT_FRONT_LEFT                   0x4
#define CHANNEL_OUT_FRONT_RIGHT                  0x8
#define CHANNEL_OUT_FRONT_CENTER                0x10
#define CHANNEL_OUT_LOW_FREQUENCY               0x20
#define CHANNEL_OUT_BACK_LEFT                   0x40
#define CHANNEL_OUT_BACK_RIGHT                  0x80
#define CHANNEL_OUT_FRONT_LEFT_OF_CENTER       0x100
#define CHANNEL_OUT_FRONT_RIGHT_OF_CENTER      0x200
#define CHANNEL_OUT_BACK_CENTER                0x400
#define CHANNEL_OUT_SIDE_LEFT                  0x800
#define CHANNEL_OUT_SIDE_RIGHT                0x1000
#define CHANNEL_OUT_TOP_CENTER                0x2000
#define CHANNEL_OUT_TOP_FRONT_LEFT            0x4000
#define CHANNEL_OUT_TOP_FRONT_CENTER          0x8000
#define CHANNEL_OUT_TOP_FRONT_RIGHT          0x10000
#define CHANNEL_OUT_TOP_BACK_LEFT            0x20000
#define CHANNEL_OUT_TOP_BACK_CENTER          0x40000
#define CHANNEL_OUT_TOP_BACK_RIGHT           0x80000
#define CHANNEL_OUT_TOP_SIDE_LEFT           0x100000
#define CHANNEL_OUT_TOP_SIDE_RIGHT          0x200000
#define CHANNEL_OUT_BOTTOM_FRONT_LEFT       0x400000
#define CHANNEL_OUT_BOTTOM_FRONT_CENTER     0x800000
#define CHANNEL_OUT_BOTTOM_FRONT_RIGHT     0x1000000
#define CHANNEL_OUT_LOW_FREQUENCY_2        0x2000000
#define CHANNEL_OUT_FRONT_WIDE_LEFT        0x4000000
#define CHANNEL_OUT_FRONT_WIDE_RIGHT       0x8000000
#define CHANNEL_OUT_HAPTIC_B              0x10000000
#define CHANNEL_OUT_HAPTIC_A              0x20000000
#define CHANNEL_OUT_MONO                  CHANNEL_OUT_FRONT_LEFT
#define CHANNEL_OUT_STEREO                (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT)
#define CHANNEL_OUT_QUAD                  (CHANNEL_OUT_STEREO \
                                           | CHANNEL_OUT_BACK_LEFT | CHANNEL_OUT_BACK_RIGHT)
#define CHANNEL_OUT_QUAD_SIDE             (CHANNEL_OUT_STEREO \
                                           | CHANNEL_OUT_SIDE_LEFT | CHANNEL_OUT_SIDE_RIGHT)
#define CHANNEL_OUT_SURROUND              (CHANNEL_OUT_STEREO \
                                           | CHANNEL_OUT_FRONT_CENTER | CHANNEL_OUT_BACK_CENTER)
#define CHANNEL_OUT_5POINT1               (CHANNEL_OUT_QUAD \
                                           | CHANNEL_OUT_FRONT_CENTER | CHANNEL_OUT_LOW_FREQUENCY)
#define CHANNEL_OUT_5POINT1_SIDE          (CHANNEL_OUT_QUAD_SIDE \
                                           | CHANNEL_OUT_FRONT_CENTER | CHANNEL_OUT_LOW_FREQUENCY)
#define CHANNEL_OUT_7POINT1_SURROUND      (CHANNEL_OUT_5POINT1 \
                                           | CHANNEL_OUT_SIDE_LEFT | CHANNEL_OUT_SIDE_RIGHT)
#define CHANNEL_OUT_5POINT1POINT2         (CHANNEL_OUT_5POINT1 \
                                           | CHANNEL_OUT_TOP_SIDE_LEFT | CHANNEL_OUT_TOP_SIDE_RIGHT)
#define CHANNEL_OUT_5POINT1POINT4         (CHANNEL_OUT_5POINT1 \
                                           | CHANNEL_OUT_TOP_FRONT_LEFT | CHANNEL_OUT_TOP_FRONT_RIGHT \
                                           | CHANNEL_OUT_TOP_BACK_LEFT | CHANNEL_OUT_TOP_BACK_RIGHT)
#define CHANNEL_OUT_7POINT1POINT2         (CHANNEL_OUT_7POINT1_SURROUND \
                                           | CHANNEL_OUT_TOP_SIDE_LEFT | CHANNEL_OUT_TOP_SIDE_RIGHT)
#define CHANNEL_OUT_7POINT1POINT4         (CHANNEL_OUT_7POINT1_SURROUND \
                                           | CHANNEL_OUT_TOP_FRONT_LEFT | CHANNEL_OUT_TOP_FRONT_RIGHT \
                                           | CHANNEL_OUT_TOP_BACK_LEFT | CHANNEL_OUT_TOP_BACK_RIGHT)
#define CHANNEL_OUT_9POINT1POINT4         (CHANNEL_OUT_7POINT1POINT4 \
                                           | CHANNEL_OUT_FRONT_WIDE_LEFT | CHANNEL_OUT_FRONT_WIDE_RIGHT)
#define CHANNEL_OUT_9POINT1POINT6         (CHANNEL_OUT_9POINT1POINT4 \
                                           | CHANNEL_OUT_TOP_SIDE_LEFT | CHANNEL_OUT_TOP_SIDE_RIGHT)
#define CHANNEL_OUT_13POINT_360RA         (CHANNEL_OUT_QUAD_SIDE \
                                           | CHANNEL_OUT_FRONT_CENTER \
                                           | CHANNEL_OUT_TOP_FRONT_LEFT| CHANNEL_OUT_TOP_FRONT_RIGHT \
                                           | CHANNEL_OUT_TOP_FRONT_CENTER \
                                           | CHANNEL_OUT_TOP_BACK_LEFT | CHANNEL_OUT_TOP_BACK_RIGHT \
                                           | CHANNEL_OUT_BOTTOM_FRONT_LEFT | CHANNEL_OUT_BOTTOM_FRONT_RIGHT \
                                           | CHANNEL_OUT_BOTTOM_FRONT_CENTER)
#define CHANNEL_OUT_22POINT2              (CHANNEL_OUT_7POINT1POINT4 \
                                           | CHANNEL_OUT_FRONT_LEFT_OF_CENTER | CHANNEL_OUT_FRONT_RIGHT_OF_CENTER \
                                           | CHANNEL_OUT_BACK_CENTER | CHANNEL_OUT_TOP_CENTER \
                                           | CHANNEL_OUT_TOP_FRONT_CENTER | CHANNEL_OUT_TOP_BACK_CENTER \
                                           | CHANNEL_OUT_TOP_SIDE_LEFT | CHANNEL_OUT_TOP_SIDE_RIGHT \
                                           | CHANNEL_OUT_BOTTOM_FRONT_LEFT | CHANNEL_OUT_BOTTOM_FRONT_RIGHT \
                                           | CHANNEL_OUT_BOTTOM_FRONT_CENTER \
                                           | CHANNEL_OUT_LOW_FREQUENCY_2)

#define CHANNEL_IN_DEFAULT                1
#define CHANNEL_IN_LEFT                 0x4
#define CHANNEL_IN_RIGHT                0x8
#define CHANNEL_IN_FRONT               0x10
#define CHANNEL_IN_BACK                0x20
#define CHANNEL_IN_LEFT_PROCESSED      0x40
#define CHANNEL_IN_RIGHT_PROCESSED     0x80
#define CHANNEL_IN_FRONT_PROCESSED    0x100
#define CHANNEL_IN_BACK_PROCESSED     0x200
#define CHANNEL_IN_PRESSURE           0x400
#define CHANNEL_IN_X_AXIS             0x800
#define CHANNEL_IN_Y_AXIS            0x1000
#define CHANNEL_IN_Z_AXIS            0x2000
#define CHANNEL_IN_VOICE_UPLINK      0x4000
#define CHANNEL_IN_VOICE_DNLINK      0x8000
#define CHANNEL_IN_BACK_LEFT        0x10000
#define CHANNEL_IN_BACK_RIGHT       0x20000
#define CHANNEL_IN_CENTER           0x40000
#define CHANNEL_IN_LOW_FREQUENCY   0x100000
#define CHANNEL_IN_TOP_LEFT        0x200000
#define CHANNEL_IN_TOP_RIGHT       0x400000
#define CHANNEL_IN_MONO            CHANNEL_IN_FRONT
#define CHANNEL_IN_STEREO          (CHANNEL_IN_LEFT | CHANNEL_IN_RIGHT)
#define CHANNEL_IN_2POINT0POINT2   (CHANNEL_IN_STEREO \
                                    | CHANNEL_IN_TOP_LEFT | CHANNEL_IN_TOP_RIGHT)
#define CHANNEL_IN_2POINT1POINT2   (CHANNEL_IN_2POINT0POINT2 \
                                    | CHANNEL_IN_LOW_FREQUENCY)
#define CHANNEL_IN_3POINT0POINT2   (CHANNEL_IN_2POINT0POINT2 | CHANNEL_IN_CENTER)
#define CHANNEL_IN_3POINT1POINT2   (CHANNEL_IN_3POINT0POINT2 | CHANNEL_IN_LOW_FREQUENCY)
#define CHANNEL_IN_5POINT1         (CHANNEL_IN_STEREO | CHANNEL_IN_CENTER \
                                    | CHANNEL_IN_BACK_LEFT | CHANNEL_IN_BACK_RIGHT \
                                    | CHANNEL_IN_LOW_FREQUENCY)
#define CHANNEL_IN_FRONT_BACK      (CHANNEL_IN_FRONT | CHANNEL_IN_BACK)

// Audio source types

#define AUDIOSOURCE_DEFAULT              0
#define AUDIOSOURCE_MIC                  1
#define AUDIOSOURCE_VOICE_UPLINK         2
#define AUDIOSOURCE_VOICE_DOWNLINK       3
#define AUDIOSOURCE_VOICE_CALL           4
#define AUDIOSOURCE_CAMCORDER            5
#define AUDIOSOURCE_VOICE_RECOGNITION    6
#define AUDIOSOURCE_VOICE_COMMUNICATION  7
#define AUDIOSOURCE_UNPROCESSED          9
#define AUDIOSOURCE_VOICE_PERFORMANCE   10

// Encodings

#define ENCODING_INVALID                      0
#define ENCODING_DEFAULT                      1
#define ENCODING_PCM_16BIT                    2
#define ENCODING_PCM_8BIT                     3
#define ENCODING_PCM_FLOAT                    4
#define ENCODING_AC3                          5
#define ENCODING_E_AC3                        6
#define ENCODING_DTS                          7
#define ENCODING_DTS_HD                       8
#define ENCODING_MP3                          9
#define ENCODING_AAC_LC                       10
#define ENCODING_AAC_HE_V1                    11
#define ENCODING_AAC_HE_V2                    12
#define ENCODING_IEC61937                     13
#define ENCODING_DOLBY_TRUEHD                 14
#define ENCODING_AAC_ELD                      15
#define ENCODING_AAC_XHE                      16
#define ENCODING_AC4                          17
#define ENCODING_E_AC3_JOC                    18
#define ENCODING_DOLBY_MAT                    19
#define ENCODING_OPUS                         20
#define ENCODING_LEGACY_SHORT_ARRAY_THRESHOLD ENCODING_OPUS
#define ENCODING_PCM_24BIT_PACKED             21
#define ENCODING_PCM_32BIT                    22
#define ENCODING_MPEGH_BL_L3                  23
#define ENCODING_MPEGH_BL_L4                  24
#define ENCODING_MPEGH_LC_L3                  25
#define ENCODING_MPEGH_LC_L4                  26
#define ENCODING_DRA                          28
#define ENCODING_DTS_HD_MA                    29
#define ENCODING_DTS_UHD_P1                   27
#define ENCODING_DTS_UHD_P2                   30

using SampleFormatMap = QMap<jint, AkAudioCaps::SampleFormat>;

inline SampleFormatMap initSampleFormatMap()
{
    SampleFormatMap sampleFormats {
        {ENCODING_PCM_8BIT , AkAudioCaps::SampleFormat_u8 },
        {ENCODING_PCM_16BIT, AkAudioCaps::SampleFormat_s16},
        {ENCODING_PCM_32BIT, AkAudioCaps::SampleFormat_s32},
        {ENCODING_PCM_FLOAT, AkAudioCaps::SampleFormat_flt},
    };

    return sampleFormats;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatMap,
                          sampleFormatsMap,
                          (initSampleFormatMap()))

// Device types

#define TYPE_UNKNOWN               0
#define TYPE_BUILTIN_EARPIECE      1
#define TYPE_BUILTIN_SPEAKER       2
#define TYPE_WIRED_HEADSET         3
#define TYPE_WIRED_HEADPHONES      4
#define TYPE_LINE_ANALOG           5
#define TYPE_LINE_DIGITAL          6
#define TYPE_BLUETOOTH_SCO         7
#define TYPE_BLUETOOTH_A2DP        8
#define TYPE_HDMI                  9
#define TYPE_HDMI_ARC             10
#define TYPE_USB_DEVICE           11
#define TYPE_USB_ACCESSORY        12
#define TYPE_DOCK                 13
#define TYPE_FM                   14
#define TYPE_BUILTIN_MIC          15
#define TYPE_FM_TUNER             16
#define TYPE_TV_TUNER             17
#define TYPE_TELEPHONY            18
#define TYPE_AUX_LINE             19
#define TYPE_IP                   20
#define TYPE_BUS                  21
#define TYPE_USB_HEADSET          22
#define TYPE_HEARING_AID          23
#define TYPE_BUILTIN_SPEAKER_SAFE 24
#define TYPE_REMOTE_SUBMIX        25
#define TYPE_BLE_HEADSET          26
#define TYPE_BLE_SPEAKER          27
#define TYPE_HDMI_EARC            29
#define TYPE_BLE_BROADCAST        30

using DeviceTypeMap = QMap<jint, QString>;

inline DeviceTypeMap initDeviceTypeMap()
{
    DeviceTypeMap deviceTypes {
        {TYPE_UNKNOWN             , "Unknown"             },
        {TYPE_BUILTIN_EARPIECE    , "Builtin Earpiece"    },
        {TYPE_BUILTIN_SPEAKER     , "Builtin speaker"     },
        {TYPE_WIRED_HEADSET       , "Wired headset"       },
        {TYPE_WIRED_HEADPHONES    , "Wired headphones"    },
        {TYPE_LINE_ANALOG         , "Line Analog"         },
        {TYPE_LINE_DIGITAL        , "Line Digital"        },
        {TYPE_BLUETOOTH_SCO       , "Bluetooth SCO"       },
        {TYPE_BLUETOOTH_A2DP      , "Bluetooth A2DP"      },
        {TYPE_HDMI                , "HDMI"                },
        {TYPE_HDMI_ARC            , "HDMI ARC"            },
        {TYPE_USB_DEVICE          , "USB Device"          },
        {TYPE_USB_ACCESSORY       , "USB Accessory"       },
        {TYPE_DOCK                , "Dock"                },
        {TYPE_FM                  , "FM"                  },
        {TYPE_BUILTIN_MIC         , "Builtin Mic"         },
        {TYPE_FM_TUNER            , "FM Tuner"            },
        {TYPE_TV_TUNER            , "TV Tuner"            },
        {TYPE_TELEPHONY           , "Telephony"           },
        {TYPE_AUX_LINE            , "Auxiliar Line"       },
        {TYPE_IP                  , "IP"                  },
        {TYPE_BUS                 , "Bus"                 },
        {TYPE_USB_HEADSET         , "USB Headset"         },
        {TYPE_HEARING_AID         , "Hearing Aid"         },
        {TYPE_BUILTIN_SPEAKER_SAFE, "Builtin Speaker Safe"},
        {TYPE_REMOTE_SUBMIX       , "Remote Submix"       },
        {TYPE_BLE_HEADSET         , "BLE Headset"         },
        {TYPE_BLE_SPEAKER         , "BLE Speaker"         },
        {TYPE_HDMI_EARC           , "HDMI EARC"           },
        {TYPE_BLE_BROADCAST       , "BLE Broadcast"       },
    };

    return deviceTypes;
}

Q_GLOBAL_STATIC_WITH_ARGS(DeviceTypeMap,
                          deviceTypeMap,
                          (initDeviceTypeMap()))

class AudioDevJNIAudioPrivate
{
    public:
        AudioDevJNIAudio *self;
        QString m_error;
        QString m_defaultSink;
        QString m_defaultSource;
        QStringList m_sinks;
        QStringList m_sources;
        QMap<QString, QString> m_pinDescriptionMap;
        QMap<QString, QList<AkAudioCaps::SampleFormat>> m_supportedFormats;
        QMap<QString, QList<AkAudioCaps::ChannelLayout>> m_supportedLayouts;
        QMap<QString, QList<int>> m_supportedSampleRates;
        QMap<QString, AkAudioCaps> m_preferredCaps;
        QAndroidJniObject m_context;
        QAndroidJniObject m_audioManager;
        QAndroidJniObject m_callbacks;
        QAndroidJniObject m_player;
        QAndroidJniObject m_recorder;
        QMutex m_mutex;
        AkAudioCaps m_curCaps;
        jint m_sessionId {0};
        int m_bufferSize {0};

        explicit AudioDevJNIAudioPrivate(AudioDevJNIAudio *self);
        void registerNatives();
        QAndroidJniObject deviceInfo(const QString &device);
        bool initPlayer(const QString &device, const AkAudioCaps &caps);
        bool initRecorder(const QString &device, const AkAudioCaps &caps);
        static bool hasAudioCapturePermissions();
        static void devicesUpdated(JNIEnv *env, jobject obj, jlong userPtr);
        void updateDevices();
};

AudioDevJNIAudio::AudioDevJNIAudio(QObject *parent):
    AudioDev(parent)
{
    this->d = new AudioDevJNIAudioPrivate(this);
    this->d->hasAudioCapturePermissions();

    auto serviceName =
            this->d->m_context.getStaticObjectField("android/content/Context",
                                                    "AUDIO_SERVICE",
                                                    "Ljava/lang/String;");
    this->d->m_audioManager =
            this->d->m_context.callObjectMethod("getSystemService",
                                                 "(Ljava/lang/String;)Ljava/lang/Object;",
                                                 serviceName.object());
    this->d->updateDevices();
}

AudioDevJNIAudio::~AudioDevJNIAudio()
{
    delete this->d;
}

QString AudioDevJNIAudio::error() const
{
    return this->d->m_error;
}

QString AudioDevJNIAudio::defaultInput()
{
    return this->d->m_sources.value(0);
}

QString AudioDevJNIAudio::defaultOutput()
{
    return this->d->m_sinks.value(0);
}

QStringList AudioDevJNIAudio::inputs()
{
    return this->d->m_sources;
}

QStringList AudioDevJNIAudio::outputs()
{
    return this->d->m_sinks;
}

QString AudioDevJNIAudio::description(const QString &device)
{
    return this->d->m_pinDescriptionMap.value(device);
}

AkAudioCaps AudioDevJNIAudio::preferredFormat(const QString &device)
{
    return this->d->m_preferredCaps.value(device);
}

QList<AkAudioCaps::SampleFormat> AudioDevJNIAudio::supportedFormats(const QString &device)
{
    return this->d->m_supportedFormats.value(device);
}

QList<AkAudioCaps::ChannelLayout> AudioDevJNIAudio::supportedChannelLayouts(const QString &device)
{
    return this->d->m_supportedLayouts.value(device);
}

QList<int> AudioDevJNIAudio::supportedSampleRates(const QString &device)
{
    return this->d->m_supportedSampleRates.value(device);
}

bool AudioDevJNIAudio::init(const QString &device, const AkAudioCaps &caps)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);
    this->d->m_curCaps = caps;
    this->d->m_bufferSize =
            qMax(this->latency() * caps.rate() / 1000, 1)
                 * AkAudioCaps::bitsPerSample(caps.format())
                 * caps.channels() / 8;
    return device.startsWith("OutputDevice_")?
                this->d->initPlayer(device, caps):
                this->d->initRecorder(device, caps);
}

QByteArray AudioDevJNIAudio::read()
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_recorder.isValid())
        return {};

    QAndroidJniEnvironment jniEnv;
    auto audioData = jniEnv->NewByteArray(this->d->m_bufferSize);

    if (audioData) {
        auto bytesRead =
            this->d->m_recorder.callMethod<jint>("read",
                                                 "([BIII)I",
                                                 audioData,
                                                 0,
                                                 jniEnv->GetArrayLength(audioData),
                                                 WRITE_BLOCKING);

        if (bytesRead < 1) {
            jniEnv->DeleteLocalRef(audioData);

            return {};
        }

        QByteArray buffer(bytesRead, 0);
        jniEnv->GetByteArrayRegion(audioData,
                                   0,
                                   bytesRead,
                                   reinterpret_cast<jbyte *>(buffer.data()));
        jniEnv->DeleteLocalRef(audioData);

        return buffer;
    }

    return {};
}

bool AudioDevJNIAudio::write(const AkAudioPacket &packet)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!packet)
        return false;

    if (!this->d->m_player.isValid())
        return false;

    QAndroidJniEnvironment jniEnv;
    auto audioData = jniEnv->NewByteArray(packet.size());

    if (audioData) {
        jniEnv->SetByteArrayRegion(audioData,
                                   0,
                                   packet.size(),
                                   reinterpret_cast<const jbyte *>(packet.constData()));

        jint length = jniEnv->GetArrayLength(audioData);
        jint offset = 0;

        while (length > 0) {
            auto bytesWritten =
                this->d->m_player.callMethod<jint>("write",
                                                   "([BIII)I",
                                                   audioData,
                                                   offset,
                                                   length,
                                                   WRITE_BLOCKING);

            length -= bytesWritten;
            offset += bytesWritten;
        }

        jniEnv->DeleteLocalRef(audioData);
    }

    return true;
}

bool AudioDevJNIAudio::uninit()
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (this->d->m_player.isValid()) {
        this->d->m_player.callMethod<void>("pause");
        this->d->m_player.callMethod<void>("flush");
        this->d->m_player.callMethod<void>("stop");
        this->d->m_player.callMethod<void>("release");
        this->d->m_player = {};
    }

    if (this->d->m_recorder.isValid()) {
        this->d->m_recorder.callMethod<void>("stop");
        this->d->m_recorder.callMethod<void>("release");
        this->d->m_recorder = {};
    }

    return true;
}

AudioDevJNIAudioPrivate::AudioDevJNIAudioPrivate(AudioDevJNIAudio *self):
    self(self)
{
    this->m_context = QtAndroid::androidContext();
    this->registerNatives();
    jlong userPtr = intptr_t(this);
    this->m_callbacks = QAndroidJniObject(JCLASS(AkAndroidAudioCallbacks),
                                          "(J)V",
                                          userPtr);
    auto intentFilter =
            QAndroidJniObject("android/content/IntentFilter", "()V");

    auto actionHdmiAudioPlug =
            QAndroidJniObject::getStaticObjectField("android/media/AudioManager",
                                                    "ACTION_HDMI_AUDIO_PLUG",
                                                    "Ljava/lang/String;");
    intentFilter.callMethod<void>("addAction",
                                  "(Ljava/lang/String;)V",
                                  actionHdmiAudioPlug.object());
    auto actionHeadsetPlug =
            QAndroidJniObject::getStaticObjectField("android/media/AudioManager",
                                                    "ACTION_HEADSET_PLUG",
                                                    "Ljava/lang/String;");
    intentFilter.callMethod<void>("addAction",
                                  "(Ljava/lang/String;)V",
                                  actionHeadsetPlug.object());
    this->m_context.callObjectMethod("registerReceiver",
                                     "(Landroid.content.BroadcastReceiver;,Landroid.content.IntentFilter;)Landroid/content/IntentFilter;",
                                     this->m_callbacks.object(),
                                     intentFilter.object());
}

void AudioDevJNIAudioPrivate::registerNatives()
{
    static bool ready = false;

    if (ready)
        return;

    QAndroidJniEnvironment jniEnv;

    if (auto jclass = jniEnv.findClass(JCLASS(AkAndroidAudioCallbacks))) {
        static const QVector<JNINativeMethod> methods {
            {"devicesUpdated", "(J)V", reinterpret_cast<void *>(AudioDevJNIAudioPrivate::devicesUpdated)},
        };

        jniEnv->RegisterNatives(jclass, methods.data(), methods.size());
    }

    ready = true;
}

QAndroidJniObject AudioDevJNIAudioPrivate::deviceInfo(const QString &device)
{
    jint deviceType = device.startsWith("OutputDevice_")?
               GET_DEVICES_OUTPUTS: GET_DEVICES_INPUTS;

    auto devices = this->m_audioManager.callObjectMethod("getDevices",
                                                         "(I)[Landroid/media/AudioDeviceInfo;",
                                                         deviceType);

    if (!devices.isValid())
        return {};

    auto dev(device);

    if (deviceType == GET_DEVICES_OUTPUTS)
        dev.remove(QRegExp("^OutputDevice_"));
    else
        dev.remove(QRegExp("^InputDevice_"));

    auto deviceId = dev.toInt();
    QAndroidJniEnvironment jniEnv;

    for (jsize i = 0; i < jniEnv->GetArrayLength(static_cast<jobjectArray>(devices.object())); i++) {
        QAndroidJniObject deviceInfo =
                jniEnv->GetObjectArrayElement(static_cast<jobjectArray>(devices.object()), i);
        auto id = deviceInfo.callMethod<jint>("getId");

        if (id == deviceId)
            return deviceInfo;
    }

    return {};
}

bool AudioDevJNIAudioPrivate::initPlayer(const QString &device,
                                         const AkAudioCaps &caps)
{
    jint encoding = sampleFormatsMap->key(caps.format(), ENCODING_INVALID);

    if (encoding == ENCODING_INVALID)
        return false;

    jint channelMask =
            caps.layout() == AkAudioCaps::Layout_mono?
                CHANNEL_OUT_MONO:
            caps.layout() == AkAudioCaps::Layout_stereo?
                CHANNEL_OUT_STEREO:
                0;

    if (channelMask == 0)
        return false;

    jint sampleRate = caps.rate();

    if (sampleRate < 1)
        return false;

    // Get device info

    auto deviceInfo = this->deviceInfo(device);

    if (!deviceInfo.isValid())
        return false;

    auto apiLevel = android_get_device_api_level();

    // Configure format

    auto formatBuilder =
            QAndroidJniObject("android/media/AudioFormat$Builder", "()V");
    formatBuilder.callObjectMethod("setEncoding",
                                   "(I)Landroid/media/AudioFormat$Builder;",
                                   encoding);
    formatBuilder.callObjectMethod("setChannelMask",
                                   "(I)Landroid/media/AudioFormat$Builder;",
                                   channelMask);
    formatBuilder.callObjectMethod("setSampleRate",
                                   "(I)Landroid/media/AudioFormat$Builder;",
                                   sampleRate);
    auto format =
            formatBuilder.callObjectMethod("build",
                                           "()Landroid/media/AudioFormat;");

    // Configure attributes

    auto attributesBuilder =
            QAndroidJniObject("android/media/AudioAttributes$Builder", "()V");
    attributesBuilder.callObjectMethod("setUsage",
                                       "(I)Landroid/media/AudioAttributes$Builder;",
                                       USAGE_MEDIA);
    attributesBuilder.callObjectMethod("setContentType",
                                       "(I)Landroid/media/AudioAttributes$Builder;",
                                       CONTENT_TYPE_MOVIE);

    if (apiLevel >= __ANDROID_API_N__ && apiLevel < __ANDROID_API_O__)
        attributesBuilder.callObjectMethod("setFlags",
                                           "(I)Landroid/media/AudioAttributes$Builder;",
                                           FLAG_LOW_LATENCY);

    auto attributes =
            attributesBuilder.callObjectMethod("build",
                                               "()Landroid/media/AudioAttributes;");

    // Get mimimum buffer size

    auto minBufferSize =
            QAndroidJniObject::callStaticMethod<jint>("android/media/AudioTrack",
                                                      "getMinBufferSize",
                                                      "(III)I",
                                                      sampleRate,
                                                      channelMask,
                                                      encoding);

    // Calculate buffer size

    auto bufferSize = this->self->latency()
                      * caps.bps()
                      * caps.channels()
                      * caps.rate()
                      / 1000;
    bufferSize = qMax(bufferSize, minBufferSize);

    // Get session ID

    this->m_sessionId =
            this->m_audioManager.callMethod<jint>("generateAudioSessionId");

    // Configure player

    auto trackBuilder =
            QAndroidJniObject("android/media/AudioTrack$Builder", "()V");
    trackBuilder.callObjectMethod("setAudioFormat",
                                  "(Landroid/media/AudioFormat;)Landroid/media/AudioTrack$Builder;",
                                  format.object());
    trackBuilder.callObjectMethod("setAudioAttributes",
                                  "(Landroid/media/AudioAttributes;)Landroid/media/AudioTrack$Builder;",
                                  attributes.object());
    trackBuilder.callObjectMethod("setBufferSizeInBytes",
                                  "(I)Landroid/media/AudioTrack$Builder;",
                                  bufferSize);

    if (apiLevel >= __ANDROID_API_O__)
        trackBuilder.callObjectMethod("setPerformanceMode",
                                      "(I)Landroid/media/AudioTrack$Builder;",
                                      PERFORMANCE_MODE_LOW_LATENCY);

    trackBuilder.callObjectMethod("setSessionId",
                                  "(I)Landroid/media/AudioTrack$Builder;",
                                  this->m_sessionId);
    this->m_player =
            trackBuilder.callObjectMethod("build",
                                          "()Landroid/media/AudioTrack;");
    this->m_player.callMethod<jboolean>("setPreferredDevice",
                                        "(Landroid/media/AudioDeviceInfo;)Z",
                                        deviceInfo.object());
    this->m_player.callMethod<void>("flush");
    this->m_player.callMethod<void>("play");

    return true;
}

bool AudioDevJNIAudioPrivate::initRecorder(const QString &device,
                                           const AkAudioCaps &caps)
{
    jint encoding = sampleFormatsMap->key(caps.format(), ENCODING_INVALID);

    if (encoding == ENCODING_INVALID)
        return false;

    jint channelMask =
            caps.layout() == AkAudioCaps::Layout_mono?
                CHANNEL_IN_MONO:
            caps.layout() == AkAudioCaps::Layout_stereo?
                CHANNEL_IN_STEREO:
                0;

    if (channelMask == 0)
        return false;

    jint sampleRate = caps.rate();

    if (sampleRate < 1)
        return false;

    // Get device info

    auto deviceInfo = this->deviceInfo(device);

    if (!deviceInfo.isValid())
        return false;

    // Configure format

    auto formatBuilder =
            QAndroidJniObject("android/media/AudioFormat$Builder", "()V");
    formatBuilder.callObjectMethod("setEncoding",
                                   "(I)Landroid/media/AudioFormat$Builder;",
                                   encoding);
    formatBuilder.callObjectMethod("setChannelMask",
                                   "(I)Landroid/media/AudioFormat$Builder;",
                                   channelMask);
    formatBuilder.callObjectMethod("setSampleRate",
                                   "(I)Landroid/media/AudioFormat$Builder;",
                                   sampleRate);
    auto format =
            formatBuilder.callObjectMethod("build",
                                           "()Landroid/media/AudioFormat;");

    // Get mimimum buffer size

    auto minBufferSize =
            QAndroidJniObject::callStaticMethod<jint>("android/media/AudioRecord",
                                                      "getMinBufferSize",
                                                      "(III)I",
                                                      sampleRate,
                                                      channelMask,
                                                      encoding);

    // Calculate buffer size

    auto bufferSize = this->self->latency()
                      * caps.bps()
                      * caps.channels()
                      * caps.rate()
                      / 1000;
    bufferSize = qMax(bufferSize, minBufferSize);

    // Configure recorder

    auto recordBuilder =
            QAndroidJniObject("android/media/AudioRecord$Builder", "()V");
    recordBuilder.callObjectMethod("setAudioFormat",
                                   "(Landroid/media/AudioFormat;)Landroid/media/AudioRecord$Builder;",
                                   format.object());
    recordBuilder.callObjectMethod("setAudioSource",
                                   "(I)Landroid/media/AudioRecord$Builder;",
                                   AUDIOSOURCE_MIC);
    recordBuilder.callObjectMethod("setBufferSizeInBytes",
                                   "(I)Landroid/media/AudioRecord$Builder;",
                                   bufferSize);
    recordBuilder.callObjectMethod("setContext",
                                   "(Landroid/content/Context;)Landroid/media/AudioRecord$Builder;",
                                   this->m_context.object());
    recordBuilder.callObjectMethod("setPreferredDevice",
                                   "(Landroid/media/AudioDeviceInfo;)Landroid/media/AudioRecord$Builder;",
                                   deviceInfo.object());
    this->m_recorder =
            recordBuilder.callObjectMethod("build",
                                           "()Landroid/media/AudioTrack;");
    this->m_recorder.callMethod<void>("startRecording");

    return true;
}

bool AudioDevJNIAudioPrivate::hasAudioCapturePermissions()
{
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

    return true;
}

void AudioDevJNIAudioPrivate::devicesUpdated(JNIEnv *env,
                                             jobject obj,
                                             jlong userPtr)
{
    Q_UNUSED(env)
    Q_UNUSED(obj)

    auto self = reinterpret_cast<AudioDevJNIAudioPrivate *>(intptr_t(userPtr));
    self->updateDevices();
}

void AudioDevJNIAudioPrivate::updateDevices()
{
    decltype(this->m_sources) inputs;
    decltype(this->m_sinks) outputs;
    decltype(this->m_pinDescriptionMap) pinDescriptionMap;
    decltype(this->m_supportedFormats) supportedFormats;
    decltype(this->m_supportedLayouts) supportedChannels;
    decltype(this->m_supportedSampleRates) supportedSampleRates;
    decltype(this->m_preferredCaps) devicePreferredCaps;

    static const QList<AkAudioCaps::SampleFormat> preferredFormats {
        AkAudioCaps::SampleFormat_s16,
        AkAudioCaps::SampleFormat_s32,
        AkAudioCaps::SampleFormat_flt,
        AkAudioCaps::SampleFormat_u8,
    };

    jint deviceTypes = this->hasAudioCapturePermissions()?
               GET_DEVICES_ALL: GET_DEVICES_OUTPUTS;

    auto devices = this->m_audioManager.callObjectMethod("getDevices",
                                                         "(I)[Landroid/media/AudioDeviceInfo;",
                                                         deviceTypes);

    if (devices.isValid()) {
        QAndroidJniEnvironment jniEnv;

        for (jsize i = 0; i < jniEnv->GetArrayLength(static_cast<jobjectArray>(devices.object())); i++) {
            QAndroidJniObject deviceInfo =
                    jniEnv->GetObjectArrayElement(static_cast<jobjectArray>(devices.object()), i);
            auto deviceId = deviceInfo.callMethod<jint>("getId");
            auto deviceType = deviceInfo.callMethod<jint>("getType");
            auto productName =
                    deviceInfo.callObjectMethod("getProductName",
                                                "()Ljava/lang/CharSequence;").toString();
            auto description = QString("%1 %2 (%3)")
                               .arg(deviceTypeMap->value(deviceType, "Unknown"))
                               .arg(deviceId)
                               .arg(productName);

            // Read sample formats

            auto encodings =
                    deviceInfo.callObjectMethod("getEncodings", "()[I");
            QList<AkAudioCaps::SampleFormat> formats;
            auto encodingsCount =
                    jniEnv->GetArrayLength(static_cast<jintArray>(encodings.object()));

            if (encodingsCount == 0) {
                formats = preferredFormats;
            } else {
                jboolean isCopy = JNI_FALSE;
                auto encodingsArray =
                        jniEnv->GetIntArrayElements(static_cast<jintArray>(encodings.object()),
                                                    &isCopy);

                for (jsize j = 0 ; j < encodingsCount; j++) {
                    auto format = sampleFormatsMap->value(encodingsArray[j],
                                                       AkAudioCaps::SampleFormat_none);

                    if (format != AkAudioCaps::SampleFormat_none
                        && !formats.contains(format))
                        formats << format;
                }

                if (isCopy)
                    jniEnv->ReleaseIntArrayElements(static_cast<jintArray>(encodings.object()),
                                                    encodingsArray,
                                                    JNI_ABORT);
            }

            // Read channel layouts

            auto channels = deviceInfo.callObjectMethod("getChannelCounts",
                                                        "()[I");
            QList<AkAudioCaps::ChannelLayout> layouts;
            auto channelsCount =
                    jniEnv->GetArrayLength(static_cast<jintArray>(channels.object()));

            if (channelsCount == 0) {
                layouts = {AkAudioCaps::Layout_mono,
                           AkAudioCaps::Layout_stereo};
            } else {
                jboolean isCopy = JNI_FALSE;
                auto channelsArray =
                        jniEnv->GetIntArrayElements(static_cast<jintArray>(channels.object()),
                                                    &isCopy);

                for (jsize j = 0; j < channelsCount; j++) {
                    switch (channelsArray[j]) {
                    case 1:
                        if (!layouts.contains(AkAudioCaps::Layout_mono))
                            layouts << AkAudioCaps::Layout_mono;

                        break;
                    case 2:
                        if (!layouts.contains(AkAudioCaps::Layout_stereo))
                            layouts << AkAudioCaps::Layout_stereo;

                        break;
                    default:
                        break;
                    }
                }

                if (isCopy)
                    jniEnv->ReleaseIntArrayElements(static_cast<jintArray>(channels.object()),
                                                    channelsArray,
                                                    JNI_ABORT);
            }

            // Read sample rates

            auto sampleRates = deviceInfo.callObjectMethod("getSampleRates",
                                                           "()[I");
            QList<int> rates;
            auto sampleRatesCount =
                    jniEnv->GetArrayLength(static_cast<jintArray>(sampleRates.object()));

            if (sampleRatesCount == 0) {
                rates = this->self->commonSampleRates().toList();
            } else {
                jboolean isCopy = JNI_FALSE;
                auto sampleRatesArray =
                        jniEnv->GetIntArrayElements(static_cast<jintArray>(sampleRates.object()),
                                                    &isCopy);

                for (jsize j = 0; j < sampleRatesCount; j++) {
                    auto rate = sampleRatesArray[j];

                    if (rate > 0 && !rates.contains(rate))
                        rates << rate;
                }

                if (isCopy)
                    jniEnv->ReleaseIntArrayElements(static_cast<jintArray>(sampleRates.object()),
                                                    sampleRatesArray,
                                                    JNI_ABORT);
            }

            std::sort(rates.begin(), rates.end());

            // Add device

            if (!description.isEmpty()
                && !formats.isEmpty()
                && !layouts.isEmpty()
                && !rates.isEmpty()) {
                // Default caps

                AkAudioCaps::SampleFormat preferredFormat =
                        AkAudioCaps::SampleFormat_none;

                for (auto &format: preferredFormats)
                    if (formats.contains(format)) {
                        preferredFormat = format;

                        break;
                    }

                AkAudioCaps::ChannelLayout preferredLayout =
                        layouts.contains(AkAudioCaps::Layout_stereo)?
                            AkAudioCaps::Layout_stereo:
                            AkAudioCaps::Layout_mono;

                static const int wantedSampleRate = 44100;
                int preferredSampleRate = 0;
                int k = std::numeric_limits<int>::max();

                for (auto &rate: rates) {
                    auto diff = qAbs(rate - wantedSampleRate);

                    if (diff < k) {
                        preferredSampleRate = rate;
                        k = diff;
                    }
                }

                AkAudioCaps preferredCaps(preferredFormat,
                                          preferredLayout,
                                          false,
                                          preferredSampleRate);

                // Add device to outputs

                if (deviceInfo.callMethod<jboolean>("isSink")) {
                    auto id = QString("OutputDevice_%2").arg(deviceId);
                    outputs << id;
                    pinDescriptionMap[id] = description;
                    supportedFormats[id] = formats;
                    supportedChannels[id] = layouts;
                    supportedSampleRates[id] = rates;
                    devicePreferredCaps[id] = preferredCaps;
                }

                // Add device to inputs

                if (deviceInfo.callMethod<jboolean>("isSource")) {
                    auto id = QString("InputDevice_%2").arg(deviceId);
                    inputs << id;
                    pinDescriptionMap[id] = description;
                    supportedFormats[id] = formats;
                    supportedChannels[id] = layouts;
                    supportedSampleRates[id] = rates;
                    devicePreferredCaps[id] = preferredCaps;
                }
            }
        }
    }

    // Update devices
    if (this->m_supportedFormats != supportedFormats)
        this->m_supportedFormats = supportedFormats;

    if (this->m_supportedLayouts != supportedChannels)
        this->m_supportedLayouts = supportedChannels;

    if (this->m_supportedSampleRates != supportedSampleRates)
        this->m_supportedSampleRates = supportedSampleRates;

    if (this->m_pinDescriptionMap != pinDescriptionMap)
        this->m_pinDescriptionMap = pinDescriptionMap;

    if (this->m_preferredCaps != devicePreferredCaps)
        this->m_preferredCaps = devicePreferredCaps;

    if (this->m_sources != inputs) {
        this->m_sources = inputs;
        emit self->inputsChanged(inputs);
    }

    if (this->m_sinks != outputs) {
        this->m_sinks = outputs;
        emit self->outputsChanged(outputs);
    }

    QString defaultOutput = outputs.isEmpty()? "": outputs.first();
    QString defaultInput = inputs.isEmpty()? "": inputs.first();

    if (this->m_defaultSource != defaultInput) {
        this->m_defaultSource = defaultInput;
        emit self->defaultInputChanged(defaultInput);
    }

    if (this->m_defaultSink != defaultOutput) {
        this->m_defaultSink = defaultOutput;
        emit self->defaultOutputChanged(defaultOutput);
    }
}

#include "moc_audiodevjniaudio.cpp"
