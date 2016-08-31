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
#include <CoreAudio/CoreAudio.h>

#include "audiodev.h"

enum CAFormatType
{
    CAFormatType_Unknown,
    CAFormatType_Int,
    CAFormatType_Float
};

typedef QMap<QString, AkAudioCaps::SampleFormat> SampleFormatsMap;

inline SampleFormatsMap initSampleFormatsMap()
{
    SampleFormatsMap sampleFormats =
        {{QString("%1_%2_%3").arg(8).arg(CAFormatType_Int).arg(Q_BYTE_ORDER), AkAudioCaps::SampleFormat_s8},
         {QString("%1_%2_%3").arg(16).arg(CAFormatType_Int).arg(Q_BYTE_ORDER), AkAudioCaps::SampleFormat_s16},
         {QString("%1_%2_%3").arg(16).arg(CAFormatType_Int).arg(Q_LITTLE_ENDIAN), AkAudioCaps::SampleFormat_s16le},
         {QString("%1_%2_%3").arg(16).arg(CAFormatType_Int).arg(Q_BIG_ENDIAN), AkAudioCaps::SampleFormat_s16be},
         {QString("%1_%2_%3").arg(24).arg(CAFormatType_Int).arg(Q_BYTE_ORDER), AkAudioCaps::SampleFormat_s24},
         {QString("%1_%2_%3").arg(24).arg(CAFormatType_Int).arg(Q_LITTLE_ENDIAN), AkAudioCaps::SampleFormat_s24le},
         {QString("%1_%2_%3").arg(24).arg(CAFormatType_Int).arg(Q_BIG_ENDIAN), AkAudioCaps::SampleFormat_s24be},
         {QString("%1_%2_%3").arg(32).arg(CAFormatType_Int).arg(Q_BYTE_ORDER), AkAudioCaps::SampleFormat_s32},
         {QString("%1_%2_%3").arg(32).arg(CAFormatType_Int).arg(Q_LITTLE_ENDIAN), AkAudioCaps::SampleFormat_s32le},
         {QString("%1_%2_%3").arg(32).arg(CAFormatType_Int).arg(Q_BIG_ENDIAN), AkAudioCaps::SampleFormat_s32be},
         {QString("%1_%2_%3").arg(32).arg(CAFormatType_Float).arg(Q_BYTE_ORDER), AkAudioCaps::SampleFormat_flt},
         {QString("%1_%2_%3").arg(32).arg(CAFormatType_Float).arg(Q_LITTLE_ENDIAN), AkAudioCaps::SampleFormat_fltle},
         {QString("%1_%2_%3").arg(32).arg(CAFormatType_Float).arg(Q_BIG_ENDIAN), AkAudioCaps::SampleFormat_fltbe}};

    return sampleFormats;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatsMap, sampleFormats, (initSampleFormatsMap()))

class AudioDevPrivate
{
    public:
        inline static QString statusToStr(OSStatus status)
        {
            QString statusStr = QString::fromUtf8(reinterpret_cast<char *>(&status), 4);
            std::reverse(statusStr.begin(), statusStr.end());

            return statusStr;
        }

        inline static AudioDeviceID defaultDevice(AudioDev::DeviceMode mode,
                                                  bool *ok=NULL)
        {
            AudioDeviceID deviceId = 0;
            UInt32 deviceIdSize = sizeof(AudioDeviceID);
            AudioObjectPropertyAddress propAddress =
                {mode == AudioDev::DeviceModeCapture?
                    kAudioHardwarePropertyDefaultInputDevice:
                    kAudioHardwarePropertyDefaultOutputDevice,
                 kAudioObjectPropertyScopeGlobal,
                 kAudioObjectPropertyElementMaster};

            OSStatus status =
                    AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                               &propAddress,
                                               0,
                                               NULL,
                                               &deviceIdSize,
                                               &deviceId);

            if (status != noErr) {
                if (ok)
                    *ok = false;

                return deviceId;
            }

            if (ok)
                *ok = true;

            return deviceId;
        }
};

AudioDev::AudioDev(QObject *parent):
    QObject(parent)
{
    this->m_defaultChannels = 0;
    this->m_defaultRate = 0;
    this->m_curBps = 0;
    this->m_curChannels = 0;
}

AudioDev::~AudioDev()
{
    this->uninit();
}

QString AudioDev::error() const
{
    return this->m_error;
}

// Get native format for the default audio device.
bool AudioDev::preferredFormat(DeviceMode mode,
                               AkAudioCaps::SampleFormat *sampleFormat,
                               int *channels,
                               int *sampleRate)
{
    bool ok = false;
    AudioDeviceID deviceID = AudioDevPrivate::defaultDevice(mode, &ok);

    if (!ok) {
        *sampleFormat = AkAudioCaps::SampleFormat_none;
        *channels = 0;
        *sampleRate = 0;
        this->m_error = "Can't get default device";
        emit this->errorChanged(this->m_error);

        return false;
    }

    UInt32 propSize = 0;
    AudioObjectPropertyAddress propAddress =
        {kAudioDevicePropertyStreams,
         mode == DeviceModeCapture?
            kAudioDevicePropertyScopeInput: kAudioDevicePropertyScopeOutput,
         kAudioObjectPropertyElementMaster};

    OSStatus status = AudioObjectGetPropertyDataSize(deviceID,
                                                     &propAddress,
                                                     0,
                                                     NULL,
                                                     &propSize);

    int nStreams = propSize / sizeof(AudioStreamID);

    if (status != noErr || nStreams < 1) {
        *sampleFormat = AkAudioCaps::SampleFormat_none;
        *channels = 0;
        *sampleRate = 0;
        this->m_error = QString("Can't read the number of streams: %1")
                        .arg(AudioDevPrivate::statusToStr(status));
        emit this->errorChanged(this->m_error);

        return false;
    }

    QVector<AudioStreamID> streams(nStreams);

    status = AudioObjectGetPropertyData(deviceID,
                                        &propAddress,
                                        0,
                                        NULL,
                                        &propSize,
                                        streams.data());

    if (status != noErr) {
        *sampleFormat = AkAudioCaps::SampleFormat_none;
        *channels = 0;
        *sampleRate = 0;
        this->m_error = QString("Can't read streams: %1")
                        .arg(AudioDevPrivate::statusToStr(status));
        emit this->errorChanged(this->m_error);

        return false;
    }

    AudioObjectPropertyAddress physicalPropAddress =
        {kAudioStreamPropertyPhysicalFormat,
         kAudioObjectPropertyScopeGlobal,
         kAudioObjectPropertyElementMaster};

    ok = false;
    AudioStreamBasicDescription streamDescription;

    foreach (AudioStreamID stream, streams) {
        status = AudioObjectGetPropertyDataSize(stream,
                                                &physicalPropAddress,
                                                0,
                                                NULL,
                                                &propSize);

        if (status != noErr)
            continue;

        status = AudioObjectGetPropertyData(stream,
                                            &physicalPropAddress,
                                            0,
                                            NULL,
                                            &propSize,
                                            &streamDescription);

        if (status == noErr) {
            ok = true;

            break;
        }
    }

    if (!ok) {
        *sampleFormat = AkAudioCaps::SampleFormat_none;
        *channels = 0;
        *sampleRate = 0;
        this->m_error = QString("Can't read default format: %1")
                        .arg(AudioDevPrivate::statusToStr(status));
        emit this->errorChanged(this->m_error);

        return false;
    }

    UInt32 bps = streamDescription.mBitsPerChannel;
    CAFormatType formatType =
            streamDescription.mFormatFlags & kAudioFormatFlagIsSignedInteger?
            CAFormatType_Int:
            streamDescription.mFormatFlags & kAudioFormatFlagIsFloat?
            CAFormatType_Float: CAFormatType_Unknown;
    int endian =
            streamDescription.mBitsPerChannel == 8?
            Q_BYTE_ORDER:
            streamDescription.mFormatFlags & kAudioFormatFlagIsBigEndian?
            Q_BIG_ENDIAN: Q_LITTLE_ENDIAN;

    *sampleFormat = sampleFormats->value(QString("%1_%2_%3").arg(bps).arg(formatType).arg(endian),
                                         AkAudioCaps::SampleFormat_none);
    *channels = streamDescription.mChannelsPerFrame;
    *sampleRate = streamDescription.mSampleRate;

    return true;
}

bool AudioDev::init(DeviceMode mode,
                    AkAudioCaps::SampleFormat sampleFormat,
                    int channels,
                    int sampleRate)
{
    return false;
}

QByteArray AudioDev::read(int samples)
{
    return QByteArray();
}

bool AudioDev::write(const QByteArray &frame)
{
    return false;
}

bool AudioDev::uninit()
{
    return true;
}
