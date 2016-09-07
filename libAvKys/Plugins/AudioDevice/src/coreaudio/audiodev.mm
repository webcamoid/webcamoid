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

#include "audiodev.h"

#define OUTPUT_DEVICE 0
#define INPUT_DEVICE  1

#define MKKEY(bps, type, endianness) \
    QString("%1_%2_%3").arg(bps).arg(type).arg(endianness)

typedef QMap<QString, AkAudioCaps::SampleFormat> SampleFormatsMap;

inline SampleFormatsMap initSampleFormatsMap()
{
    SampleFormatsMap sampleFormats = {
        {MKKEY( 8, AkAudioCaps::SampleType_int  , Q_BYTE_ORDER   ), AkAudioCaps::SampleFormat_s8   },
        {MKKEY(16, AkAudioCaps::SampleType_int  , Q_BYTE_ORDER   ), AkAudioCaps::SampleFormat_s16  },
        {MKKEY(16, AkAudioCaps::SampleType_int  , Q_LITTLE_ENDIAN), AkAudioCaps::SampleFormat_s16le},
        {MKKEY(16, AkAudioCaps::SampleType_int  , Q_BIG_ENDIAN   ), AkAudioCaps::SampleFormat_s16be},
        {MKKEY(24, AkAudioCaps::SampleType_int  , Q_BYTE_ORDER   ), AkAudioCaps::SampleFormat_s24  },
        {MKKEY(24, AkAudioCaps::SampleType_int  , Q_LITTLE_ENDIAN), AkAudioCaps::SampleFormat_s24le},
        {MKKEY(24, AkAudioCaps::SampleType_int  , Q_BIG_ENDIAN   ), AkAudioCaps::SampleFormat_s24be},
        {MKKEY(32, AkAudioCaps::SampleType_int  , Q_BYTE_ORDER   ), AkAudioCaps::SampleFormat_s32  },
        {MKKEY(32, AkAudioCaps::SampleType_int  , Q_LITTLE_ENDIAN), AkAudioCaps::SampleFormat_s32le},
        {MKKEY(32, AkAudioCaps::SampleType_int  , Q_BIG_ENDIAN   ), AkAudioCaps::SampleFormat_s32be},
        {MKKEY(32, AkAudioCaps::SampleType_float, Q_BYTE_ORDER   ), AkAudioCaps::SampleFormat_flt  },
        {MKKEY(32, AkAudioCaps::SampleType_float, Q_LITTLE_ENDIAN), AkAudioCaps::SampleFormat_fltle},
        {MKKEY(32, AkAudioCaps::SampleType_float, Q_BIG_ENDIAN   ), AkAudioCaps::SampleFormat_fltbe}
    };

    return sampleFormats;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatsMap, sampleFormats, (initSampleFormatsMap()))

AudioDev::AudioDev(QObject *parent):
    QObject(parent)
{
    this->m_audioUnit = NULL;
    this->m_bufferSize = 0;
    this->m_bufferList = NULL;
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
    AudioDeviceID deviceID = this->defaultDevice(mode, &ok);

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
                        .arg(this->statusToStr(status));
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
                        .arg(this->statusToStr(status));
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
                        .arg(this->statusToStr(status));
        emit this->errorChanged(this->m_error);

        return false;
    }

    UInt32 bps = streamDescription.mBitsPerChannel;
    AkAudioCaps::SampleType formatType =
            streamDescription.mFormatFlags & kAudioFormatFlagIsSignedInteger?
            AkAudioCaps::SampleType_int:
            streamDescription.mFormatFlags & kAudioFormatFlagIsFloat?
            AkAudioCaps::SampleType_float: AkAudioCaps::SampleType_unknown;
    int endian =
            streamDescription.mBitsPerChannel == 8?
            Q_BYTE_ORDER:
            streamDescription.mFormatFlags & kAudioFormatFlagIsBigEndian?
            Q_BIG_ENDIAN: Q_LITTLE_ENDIAN;

    *sampleFormat = sampleFormats->value(MKKEY(bps, formatType, endian),
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
    bool ok = false;
    AudioDeviceID deviceID = this->defaultDevice(mode, &ok);

    if (!ok) {
        this->m_error = "Can't get default device";
        emit this->errorChanged(this->m_error);

        return false;
    }

    AudioComponentDescription componentDescription =
        {kAudioUnitType_Output,
         kAudioUnitSubType_HALOutput,
         kAudioUnitManufacturer_Apple,
         0,
         0};
    AudioComponent component = AudioComponentFindNext(NULL,
                                                      &componentDescription);

    if (!component) {
        this->m_error = "Device not found";
        emit this->errorChanged(this->m_error);

        return false;
    }

    OSStatus status = AudioComponentInstanceNew(component,
                                                &this->m_audioUnit);

    if (status != noErr) {
        this->m_error = QString("Can't create component instance: %1")
                        .arg(this->statusToStr(status));
        emit this->errorChanged(this->m_error);

        return false;
    }

    // Set device mode.
    UInt32 enable = mode == AudioDev::DeviceModeCapture;

    status = AudioUnitSetProperty(this->m_audioUnit,
                                  kAudioOutputUnitProperty_EnableIO,
                                  kAudioUnitScope_Input,
                                  INPUT_DEVICE,
                                  &enable,
                                  sizeof(UInt32));

    if (status != noErr) {
        this->m_error = QString("Can't set device as input: %1")
                        .arg(this->statusToStr(status));
        emit this->errorChanged(this->m_error);

        return false;
    }

    enable = !enable;

    status = AudioUnitSetProperty(this->m_audioUnit,
                                  kAudioOutputUnitProperty_EnableIO,
                                  kAudioUnitScope_Output,
                                  OUTPUT_DEVICE,
                                  &enable,
                                  sizeof(UInt32));

    if (status != noErr) {
        this->m_error = QString("Can't set device as output: %1")
                        .arg(this->statusToStr(status));
        emit this->errorChanged(this->m_error);

        return false;
    }

    // Set callback.
    AURenderCallbackStruct callback =
        {AudioDev::audioCallback,
         this};

    status = AudioUnitSetProperty(this->m_audioUnit,
                                  mode == AudioDev::DeviceModeCapture?
                                      kAudioOutputUnitProperty_SetInputCallback:
                                      kAudioUnitProperty_SetRenderCallback,
                                  kAudioUnitScope_Global,
                                  0,
                                  &callback,
                                  sizeof(AURenderCallbackStruct));

    if (status != noErr) {
        this->m_error = QString("Error setting audio callback: %1")
                        .arg(this->statusToStr(status));
        emit this->errorChanged(this->m_error);

        return false;
    }

    status = AudioUnitSetProperty(this->m_audioUnit,
                                  kAudioOutputUnitProperty_CurrentDevice,
                                  kAudioUnitScope_Global,
                                  0,
                                  &deviceID,
                                  sizeof(AudioDeviceID));

    if (status != noErr) {
        this->m_error = QString("Can't set device: %1")
                        .arg(this->statusToStr(status));
        emit this->errorChanged(this->m_error);

        return false;
    }

    AudioFormatFlags sampleType =
            AkAudioCaps::sampleType(sampleFormat) == AkAudioCaps::SampleType_float?
                kAudioFormatFlagIsFloat
            : AkAudioCaps::sampleType(sampleFormat) == AkAudioCaps::SampleType_int?
                kAudioFormatFlagIsSignedInteger
            : 0;

    AudioFormatFlags sampleEndianness =
            AkAudioCaps::endianness(sampleFormat)?
                kAudioFormatFlagIsBigEndian: 0;

    AudioStreamBasicDescription streamDescription;
    streamDescription.mSampleRate = sampleRate;
    streamDescription.mFormatID = kAudioFormatLinearPCM;
    streamDescription.mFormatFlags = kAudioFormatFlagIsPacked
                                   | sampleType
                                   | sampleEndianness;
    streamDescription.mFramesPerPacket = 1;
    streamDescription.mChannelsPerFrame = channels;
    streamDescription.mBitsPerChannel = AkAudioCaps::bitsPerSample(sampleFormat);
    streamDescription.mBytesPerFrame = streamDescription.mChannelsPerFrame
                                     * streamDescription.mBitsPerChannel
                                     / 8;
    streamDescription.mBytesPerPacket = streamDescription.mFramesPerPacket
                                      * streamDescription.mBytesPerFrame;

    status = AudioUnitSetProperty(this->m_audioUnit,
                                  kAudioUnitProperty_StreamFormat,
                                  mode == AudioDev::DeviceModeCapture?
                                      kAudioUnitScope_Output:
                                      kAudioUnitScope_Input,
                                  mode == AudioDev::DeviceModeCapture?
                                      INPUT_DEVICE:
                                      OUTPUT_DEVICE,
                                  &streamDescription,
                                  sizeof(AudioStreamBasicDescription));

    if (status != noErr) {
        this->m_error = QString("Can't set stream format: %1")
                        .arg(this->statusToStr(status));
        emit this->errorChanged(this->m_error);

        return false;
    }

    UInt32 bufferSize = 0; // In N° of frames
    UInt32 vSize = sizeof(UInt32);

    status = AudioUnitGetProperty(this->m_audioUnit,
                                  kAudioDevicePropertyBufferFrameSize,
                                  kAudioUnitScope_Global,
                                  0,
                                  &bufferSize,
                                  &vSize);

    if (status != noErr) {
        this->m_error = QString("Can't read buffer size: %1")
                        .arg(this->statusToStr(status));
        emit this->errorChanged(this->m_error);

        return false;
    }

    status = AudioUnitInitialize(this->m_audioUnit);

    if (status != noErr) {
        this->m_error = QString("Can't initialize device: %1")
                        .arg(this->statusToStr(status));
        emit this->errorChanged(this->m_error);

        return false;
    }

    status = AudioOutputUnitStart(this->m_audioUnit);

    if (status != noErr) {
        this->m_error = QString("Can't start device: %1")
                        .arg(this->statusToStr(status));
        emit this->errorChanged(this->m_error);

        return false;
    }

    this->m_curBps = streamDescription.mBitsPerChannel;
    this->m_curChannels = streamDescription.mChannelsPerFrame;
    this->m_curMode = mode;
    this->m_maxBufferSize =
            2 * this->m_curBps * this->m_curChannels * bufferSize / 8;


    int nBuffers = streamDescription.mFormatFlags
                   & kAudioFormatFlagIsNonInterleaved?
                    streamDescription.mChannelsPerFrame: 1;

    this->m_bufferSize = bufferSize;
    this->m_bufferList =
            reinterpret_cast<AudioBufferList *>(malloc(sizeof(AudioBufferList)
                                                       + sizeof(AudioBuffer)
                                                       * nBuffers));
    this->m_bufferList->mNumberBuffers = nBuffers;

    for (int i = 0; i < nBuffers; i++) {
        this->m_bufferList->mBuffers[i].mNumberChannels =
                streamDescription.mFormatFlags
                & kAudioFormatFlagIsNonInterleaved?
                    1: streamDescription.mChannelsPerFrame;
        this->m_bufferList->mBuffers[i].mDataByteSize = 0;
        this->m_bufferList->mBuffers[i].mData = 0;
    }

    return true;
}

QByteArray AudioDev::read(int samples)
{
    int bufferSize = this->m_curBps * this->m_curChannels * samples / 8;
    QByteArray audioData;

    this->m_mutex.lock();

    while (audioData.size() < bufferSize) {
        if (this->m_buffer.isEmpty())
            this->m_samplesAvailable.wait(&this->m_mutex);

        int copyBytes = qMin(this->m_buffer.size(),
                             bufferSize - audioData.size());
        audioData += this->m_buffer.mid(0, copyBytes);
        this->m_buffer.remove(0, copyBytes);
    }

    this->m_mutex.unlock();

    return audioData;
}

bool AudioDev::write(const QByteArray &frame)
{
    this->m_mutex.lock();

    if (this->m_buffer.size() >= this->m_maxBufferSize)
        this->m_canWrite.wait(&this->m_mutex);

    this->m_buffer += frame;
    this->m_mutex.unlock();

    return true;
}

bool AudioDev::uninit()
{
    if (this->m_bufferList) {
        free(this->m_bufferList);
        this->m_bufferList = NULL;
    }

    if (this->m_audioUnit) {
        AudioOutputUnitStop(this->m_audioUnit);
        AudioUnitUninitialize(this->m_audioUnit);
        AudioComponentInstanceDispose(this->m_audioUnit);
        this->m_audioUnit = NULL;
    }

    this->m_bufferSize = 0;
    this->m_curBps = 0;
    this->m_curChannels = 0;
    this->m_buffer.clear();

    return true;
}

QString AudioDev::statusToStr(OSStatus status)
{
    QString statusStr = QString::fromUtf8(reinterpret_cast<char *>(&status), 4);
    std::reverse(statusStr.begin(), statusStr.end());

    return statusStr;
}

AudioDeviceID AudioDev::defaultDevice(AudioDev::DeviceMode mode, bool *ok)
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

void AudioDev::clearBuffer()
{
    for (UInt32 i = 0; i < this->m_bufferList->mNumberBuffers; i++) {
        this->m_bufferList->mBuffers[i].mDataByteSize = 0;
        this->m_bufferList->mBuffers[i].mData = 0;
    }
}

OSStatus AudioDev::audioCallback(void *audioDev,
                                 AudioUnitRenderActionFlags *actionFlags,
                                 const AudioTimeStamp *timeStamp,
                                 UInt32 busNumber,
                                 UInt32 nFrames,
                                 AudioBufferList *data)
{
    AudioDev *self = static_cast<AudioDev *>(audioDev);

    if (self->m_curMode == AudioDev::DeviceModeCapture) {
        self->clearBuffer();

        OSStatus status = AudioUnitRender(self->m_audioUnit,
                                          actionFlags,
                                          timeStamp,
                                          busNumber,
                                          nFrames,
                                          self->m_bufferList);

        if (status != noErr)
            return status;

        self->m_mutex.lock();

        // FIXME: This assumees that all samples are interleaved, so appending it to
        // the buffer is ok. It must be fixed for planar sample formats.
        for (UInt32 i = 0; i < self->m_bufferList->mNumberBuffers; i++)
            self->m_buffer +=
                QByteArray::fromRawData(static_cast<const char *>(self->m_bufferList->mBuffers[i].mData),
                                        self->m_bufferList->mBuffers[i].mDataByteSize);

        // We use a ring buffer and all old samples are discarded.
        if (self->m_buffer.size() > self->m_maxBufferSize)
            self->m_buffer =
                self->m_buffer.mid(self->m_buffer.size() - self->m_maxBufferSize,
                                   self->m_maxBufferSize);

        self->m_samplesAvailable.wakeAll();
        self->m_mutex.unlock();
    } else {
        // FIXME: Same as above.
        if (data->mNumberBuffers == 1) {
            int i = 0; // Write just the first buffer.

            memset(data->mBuffers[i].mData,
                   0,
                   data->mBuffers[i].mDataByteSize);

            self->m_mutex.lock();
            int copyBytes = qMin<int>(data->mBuffers[i].mDataByteSize,
                                      self->m_buffer.size());

            if (copyBytes > 0) {
                memcpy(data->mBuffers[i].mData,
                       self->m_buffer.constData(),
                       copyBytes);
                self->m_buffer.remove(0, copyBytes);
            }

            if (self->m_buffer.size() <= self->m_maxBufferSize)
                self->m_canWrite.wakeAll();

            self->m_mutex.unlock();
        }
    }

    return noErr;
}
