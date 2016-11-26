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

#include "audiodevcoreaudio.h"

#define OUTPUT_DEVICE 0
#define INPUT_DEVICE  1

AudioDevCoreAudio::AudioDevCoreAudio(QObject *parent):
    AudioDev(parent)
{
    this->m_audioUnit = NULL;
    this->m_bufferSize = 0;
    this->m_bufferList = NULL;
    this->m_isInput = false;

    this->m_inputs  = this->listDevices(true);
    this->m_outputs = this->listDevices(false);

    static const AudioObjectPropertyAddress propDevices = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    AudioObjectAddPropertyListener(kAudioObjectSystemObject,
                                   &propDevices,
                                   this->devicesChangedCallback,
                                   this);

    static const AudioObjectPropertyAddress propDefaultInputDevice = {
        kAudioHardwarePropertyDefaultInputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    AudioObjectAddPropertyListener(kAudioObjectSystemObject,
                                   &propDefaultInputDevice,
                                   this->defaultInputDeviceChangedCallback,
                                   this);

    static const AudioObjectPropertyAddress propDefaultOutputDevice = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    AudioObjectAddPropertyListener(kAudioObjectSystemObject,
                                   &propDefaultOutputDevice,
                                   this->defaultOutputDeviceChangedCallback,
                                   this);
}

AudioDevCoreAudio::~AudioDevCoreAudio()
{
    this->uninit();

    static const AudioObjectPropertyAddress propDevices = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    AudioObjectRemovePropertyListener(kAudioObjectSystemObject,
                                      &propDevices,
                                      this->devicesChangedCallback,
                                      this);

    static const AudioObjectPropertyAddress propDefaultInputDevice = {
        kAudioHardwarePropertyDefaultInputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    AudioObjectRemovePropertyListener(kAudioObjectSystemObject,
                                      &propDefaultInputDevice,
                                      this->defaultInputDeviceChangedCallback,
                                      this);

    static const AudioObjectPropertyAddress propDefaultOutputDevice = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    AudioObjectRemovePropertyListener(kAudioObjectSystemObject,
                                      &propDefaultOutputDevice,
                                      this->defaultOutputDeviceChangedCallback,
                                      this);
}

QString AudioDevCoreAudio::error() const
{
    return this->m_error;
}

QString AudioDevCoreAudio::defaultInput()
{
    bool ok = false;
    QString deviceId = defaultDevice(true, &ok);

    return ok? deviceId: QString();
}

QString AudioDevCoreAudio::defaultOutput()
{
    bool ok = false;
    QString deviceId = defaultDevice(false, &ok);

    return ok? deviceId: QString();
}

QStringList AudioDevCoreAudio::inputs()
{
    return this->m_inputs;
}

QStringList AudioDevCoreAudio::outputs()
{
    return this->m_outputs;
}

QString AudioDevCoreAudio::description(const QString &device)
{
    QStringList deviceParts = device.split(':');

    if (deviceParts.size() < 2)
        return QString();

    bool ok = false;
    AudioDeviceID deviceId = deviceParts[1].toUInt(&ok);

    if (!ok)
        return QString();

    bool input = device.startsWith('1');

    AudioObjectPropertyAddress propName = {
        kAudioObjectPropertyName,
        input?
            kAudioDevicePropertyScopeInput:
            kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMaster
    };

    UInt32 propSize = sizeof(CFStringRef);
    CFStringRef name;

    OSStatus status =
            AudioObjectGetPropertyData(deviceId,
                                       &propName,
                                       0,
                                       NULL,
                                       &propSize,
                                       &name);

    if (status != noErr)
        return QString();

    QString deviceName = this->CFStringToString(name);
    CFRelease(name);

    return deviceName;
}

// Get native format for the default audio device.
AkAudioCaps AudioDevCoreAudio::preferredFormat(const QString &device)
{
    QStringList deviceParts = device.split(':');

    if (deviceParts.size() < 2) {
        this->m_error = "Invalid device ID format";
        emit this->errorChanged(this->m_error);

        return AkAudioCaps();
    }

    bool ok = false;
    AudioDeviceID deviceID = deviceParts[1].toUInt(&ok);

    if (!ok) {
        this->m_error = "Invalid device ID format";
        emit this->errorChanged(this->m_error);

        return AkAudioCaps();
    }

    bool input = device.startsWith('1');

    UInt32 propSize = 0;
    AudioObjectPropertyAddress propStreams = {
        kAudioDevicePropertyStreams,
        input?
            kAudioDevicePropertyScopeInput:
            kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMaster
    };

    OSStatus status = AudioObjectGetPropertyDataSize(deviceID,
                                                     &propStreams,
                                                     0,
                                                     NULL,
                                                     &propSize);

    int nStreams = propSize / sizeof(AudioStreamID);

    if (status != noErr || nStreams < 1) {
        this->m_error = QString("Can't read the number of streams: %1")
                        .arg(this->statusToStr(status));
        emit this->errorChanged(this->m_error);

        return AkAudioCaps();
    }

    QVector<AudioStreamID> streams(nStreams);

    status = AudioObjectGetPropertyData(deviceID,
                                        &propStreams,
                                        0,
                                        NULL,
                                        &propSize,
                                        streams.data());

    if (status != noErr) {
        this->m_error = QString("Can't read streams: %1")
                        .arg(this->statusToStr(status));
        emit this->errorChanged(this->m_error);

        return AkAudioCaps();
    }

    static const AudioObjectPropertyAddress propPhysicalFormat = {
        kAudioStreamPropertyPhysicalFormat,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    AkAudioCaps defaultCaps;
    QVector<AkAudioCaps> supportedCaps;

    static const QVector<AudioObjectPropertySelector> selectors = {
        kAudioStreamPropertyAvailablePhysicalFormats,
        kAudioStreamPropertyAvailableVirtualFormats,
    };

    // These are all common formats supported by the audio converter.
    static const QVector<AkAudioCaps::SampleFormat> recommendedFormats = {
        AkAudioCaps::SampleFormat_flt,
        AkAudioCaps::SampleFormat_s32,
        AkAudioCaps::SampleFormat_s16,
        AkAudioCaps::SampleFormat_u8
//      AkAudioCaps::SampleFormat_fltp,
//      AkAudioCaps::SampleFormat_s32p,
//      AkAudioCaps::SampleFormat_s16p,
//      AkAudioCaps::SampleFormat_u8p,
    };

    for (const AudioStreamID &stream: streams) {
        // Create a list of all supported output formats.
        for (const AudioObjectPropertySelector &selector: selectors)
            for (const AudioStreamRangedDescription &description:
                     this->supportedFormats(stream, selector)) {
                AkAudioCaps audioCaps;
                audioCaps.isValid() = true;
                audioCaps.format() = this->descriptionToSampleFormat(description.mFormat);
                audioCaps.bps() = AkAudioCaps::bitsPerSample(audioCaps.format());
                audioCaps.channels() = description.mFormat.mChannelsPerFrame;
                audioCaps.rate() = int(description.mFormat.mSampleRate);
                audioCaps.layout() = AkAudioCaps::defaultChannelLayout(description.mFormat.mChannelsPerFrame);
                audioCaps.align() = false;

                // Formats not supported by the converter are excluded.
                if (recommendedFormats.contains(audioCaps.format())
                    && !supportedCaps.contains(audioCaps)) {
                    supportedCaps << audioCaps;
                }
            }

        // Find default format.
        status = AudioObjectGetPropertyDataSize(stream,
                                                &propPhysicalFormat,
                                                0,
                                                NULL,
                                                &propSize);

        if (status != noErr)
            continue;

        AudioStreamBasicDescription streamDescription;

        status = AudioObjectGetPropertyData(stream,
                                            &propPhysicalFormat,
                                            0,
                                            NULL,
                                            &propSize,
                                            &streamDescription);

        if (status == noErr && !defaultCaps) {
            AkAudioCaps::SampleFormat sampleFormat = this->descriptionToSampleFormat(streamDescription);

            // If the format is not supported by the converter,
            // find another one.
            if (recommendedFormats.contains(sampleFormat)) {
                defaultCaps.isValid() = true;
                defaultCaps.format() = sampleFormat;
                defaultCaps.bps() = AkAudioCaps::bitsPerSample(defaultCaps.format());
                defaultCaps.channels() = streamDescription.mChannelsPerFrame;
                defaultCaps.rate() = int(streamDescription.mSampleRate);
                defaultCaps.layout() = AkAudioCaps::defaultChannelLayout(streamDescription.mChannelsPerFrame);
                defaultCaps.align() = false;
            }
        }
    }

    if (!defaultCaps) {
        if (supportedCaps.isEmpty()) {
            // There is no suitable output format.
            this->m_error = QString("Can't read default format: %1")
                            .arg(this->statusToStr(status));
            emit this->errorChanged(this->m_error);

            return AkAudioCaps();
        } else for (const AkAudioCaps::SampleFormat &sampleFormat: recommendedFormats) {
            // Select the best output format.
            if (defaultCaps)
                break;

            for (const AkAudioCaps &caps: supportedCaps)
                if (caps.format() == sampleFormat) {
                    defaultCaps = caps;

                    break;
                }
        }
    }

    return defaultCaps;
}

bool AudioDevCoreAudio::init(const QString &device, const AkAudioCaps &caps)
{
    QStringList deviceParts = device.split(':');

    if (deviceParts.size() < 2) {
        this->m_error = "Invalid device ID format";
        emit this->errorChanged(this->m_error);

        return AkAudioCaps();
    }

    bool ok = false;
    AudioDeviceID deviceID = deviceParts[1].toUInt(&ok);

    if (!ok) {
        this->m_error = "Invalid device ID format";
        emit this->errorChanged(this->m_error);

        return AkAudioCaps();
    }

    bool input = device.startsWith('1');

    static const AudioComponentDescription componentDescription = {
        kAudioUnitType_Output,
        kAudioUnitSubType_HALOutput,
        kAudioUnitManufacturer_Apple,
        0,
        0
    };

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
    UInt32 enable = input;

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
    AURenderCallbackStruct callback = {
        AudioDevCoreAudio::audioCallback,
        this
    };

    status = AudioUnitSetProperty(this->m_audioUnit,
                                  input?
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
            AkAudioCaps::sampleType(caps.format()) == AkAudioCaps::SampleType_float?
            kAudioFormatFlagIsFloat:
            AkAudioCaps::sampleType(caps.format()) == AkAudioCaps::SampleType_int?
            kAudioFormatFlagIsSignedInteger:
            0;
    AudioFormatFlags sampleEndianness =
            AkAudioCaps::endianness(caps.format()) == Q_BIG_ENDIAN?
                kAudioFormatFlagIsBigEndian: 0;
    AudioFormatFlags sampleIsPlanar =
            AkAudioCaps::isPlanar(caps.format())?
                kAudioFormatFlagIsNonInterleaved: 0;

    AudioStreamBasicDescription streamDescription;
    streamDescription.mSampleRate = caps.rate();
    streamDescription.mFormatID = kAudioFormatLinearPCM;
    streamDescription.mFormatFlags = kAudioFormatFlagIsPacked
                                   | sampleType
                                   | sampleEndianness
                                   | sampleIsPlanar;
    streamDescription.mFramesPerPacket = 1;
    streamDescription.mChannelsPerFrame = caps.channels();
    streamDescription.mBitsPerChannel = AkAudioCaps::bitsPerSample(caps.format());
    streamDescription.mBytesPerFrame = streamDescription.mChannelsPerFrame
                                     * streamDescription.mBitsPerChannel
                                     / 8;
    streamDescription.mBytesPerPacket = streamDescription.mFramesPerPacket
                                      * streamDescription.mBytesPerFrame;

    status = AudioUnitSetProperty(this->m_audioUnit,
                                  kAudioUnitProperty_StreamFormat,
                                  input?
                                      kAudioUnitScope_Output:
                                      kAudioUnitScope_Input,
                                  input?
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

    this->m_curCaps = caps;
    this->m_isInput = input;
    this->m_maxBufferSize = 2 * caps.bps() * caps.channels() * bufferSize / 8;

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

QByteArray AudioDevCoreAudio::read(int samples)
{
    int bufferSize = this->m_curCaps.bps() * this->m_curCaps.channels() * samples / 8;
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

bool AudioDevCoreAudio::write(const QByteArray &frame)
{
    this->m_mutex.lock();

    if (this->m_buffer.size() >= this->m_maxBufferSize)
        this->m_canWrite.wait(&this->m_mutex);

    this->m_buffer += frame;
    this->m_mutex.unlock();

    return true;
}

bool AudioDevCoreAudio::uninit()
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
    this->m_curCaps = AkAudioCaps();
    this->m_isInput = false;
    this->m_buffer.clear();

    return true;
}

QString AudioDevCoreAudio::statusToStr(OSStatus status)
{
    QString statusStr = QString::fromUtf8(reinterpret_cast<char *>(&status), 4);
    std::reverse(statusStr.begin(), statusStr.end());

    return statusStr;
}

QString AudioDevCoreAudio::CFStringToString(const CFStringRef &cfstr)
{
    CFIndex len = CFStringGetLength(cfstr);
    const UniChar *data = CFStringGetCharactersPtr(cfstr);

    if (data)
        return QString(reinterpret_cast<const QChar *>(data), len);

    UniChar str[len];
    CFStringGetCharacters(cfstr, CFRangeMake(0, len), str);

    return QString(reinterpret_cast<const QChar *>(str), len);
}

QString AudioDevCoreAudio::defaultDevice(bool input, bool *ok)
{
    const AudioObjectPropertyAddress propDefaultDevice = {
        input?
            kAudioHardwarePropertyDefaultInputDevice:
            kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    AudioDeviceID deviceId = 0;
    UInt32 propSize = sizeof(AudioDeviceID);

    OSStatus status =
            AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                       &propDefaultDevice,
                                       0,
                                       NULL,
                                       &propSize,
                                       &deviceId);

    if (status != noErr) {
        if (ok)
            *ok = false;

        return QString();
    }

    if (ok)
        *ok = true;

    return QString("%1:%2").arg(input).arg(deviceId);
}

QStringList AudioDevCoreAudio::listDevices(bool input)
{
    static const AudioObjectPropertyAddress propDevices = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    UInt32 propSize = 0;

    OSStatus status =
            AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,
                                           &propDevices,
                                           0,
                                           NULL,
                                           &propSize);

    if (status != noErr)
        return QStringList();

    int nDevices = propSize / sizeof(AudioDeviceID);

    if (nDevices < 1)
        return QStringList();

    QVector<AudioDeviceID> devices(nDevices);

    status = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                        &propDevices,
                                        0,
                                        NULL,
                                        &propSize,
                                        devices.data());

    if (status != noErr)
        return QStringList();

    AudioObjectPropertyAddress propStreamFormat = {
        kAudioDevicePropertyStreamFormat,
        input?
            kAudioObjectPropertyScopeInput:
            kAudioObjectPropertyScopeOutput,
        kAudioObjectPropertyElementMaster
    };

    QStringList deviceList;

    for (AudioDeviceID &deviceId: devices) {
        propSize = sizeof(AudioStreamBasicDescription);
        AudioStreamBasicDescription streamDescription;

        status = AudioObjectGetPropertyData(deviceId,
                                            &propStreamFormat,
                                            0,
                                            NULL,
                                            &propSize,
                                            &streamDescription);

        if (status != noErr)
            continue;

        deviceList << QString("%1:%2").arg(input).arg(deviceId);
    }

    return deviceList;
}

void AudioDevCoreAudio::clearBuffer()
{
    for (UInt32 i = 0; i < this->m_bufferList->mNumberBuffers; i++) {
        this->m_bufferList->mBuffers[i].mDataByteSize = 0;
        this->m_bufferList->mBuffers[i].mData = 0;
    }
}

QVector<AudioStreamRangedDescription> AudioDevCoreAudio::supportedFormats(AudioStreamID stream,
                                                                          AudioObjectPropertySelector selector)
{
    AudioObjectPropertyAddress physicalPropAddress = {
        selector,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    UInt32 propSize = 0;
    OSStatus status = AudioObjectGetPropertyDataSize(stream,
                                                     &physicalPropAddress,
                                                     0,
                                                     NULL,
                                                     &propSize);

    if (status != noErr || !propSize)
        return QVector<AudioStreamRangedDescription>();

    UInt32 count = propSize / sizeof(AudioStreamRangedDescription);
    QVector<AudioStreamRangedDescription> supportedFormats(count);

    status = AudioObjectGetPropertyData(stream,
                                        &physicalPropAddress,
                                        0,
                                        NULL,
                                        &propSize,
                                        supportedFormats.data());

    if (status != noErr || !propSize)
        return QVector<AudioStreamRangedDescription>();

    return supportedFormats;
}

AkAudioCaps::SampleFormat AudioDevCoreAudio::descriptionToSampleFormat(const AudioStreamBasicDescription &streamDescription)
{
    UInt32 bps = streamDescription.mBitsPerChannel;
    AkAudioCaps::SampleType formatType =
            streamDescription.mFormatFlags & kAudioFormatFlagIsFloat?
            AkAudioCaps::SampleType_float:
            streamDescription.mFormatFlags & kAudioFormatFlagIsSignedInteger?
            AkAudioCaps::SampleType_int:
            AkAudioCaps::SampleType_uint;
    int endian =
            streamDescription.mBitsPerChannel == 8?
            Q_BYTE_ORDER:
            streamDescription.mFormatFlags & kAudioFormatFlagIsBigEndian?
            Q_BIG_ENDIAN: Q_LITTLE_ENDIAN;
    bool planar = streamDescription.mFormatFlags & kAudioFormatFlagIsNonInterleaved;

    return AkAudioCaps::sampleFormatFromProperties(formatType,
                                                   bps,
                                                   endian,
                                                   planar);
}

OSStatus AudioDevCoreAudio::devicesChangedCallback(AudioObjectID objectId,
                                                   UInt32 nProps,
                                                   const AudioObjectPropertyAddress *properties,
                                                   void *audioDev)
{
    Q_UNUSED(objectId)
    Q_UNUSED(nProps)
    Q_UNUSED(properties)

    AudioDevCoreAudio *self = static_cast<AudioDevCoreAudio *>(audioDev);
    QStringList inputs = self->listDevices(true);

    if (self->m_inputs != inputs) {
        self->m_inputs = inputs;
        emit self->inputsChanged(inputs);
    }

    QStringList outputs = self->listDevices(false);

    if (self->m_outputs != outputs) {
        self->m_outputs = outputs;
        emit self->outputsChanged(outputs);
    }

    return noErr;
}

OSStatus AudioDevCoreAudio::defaultInputDeviceChangedCallback(AudioObjectID objectId,
                                                              UInt32 nProps,
                                                              const AudioObjectPropertyAddress *properties,
                                                              void *audioDev)
{
    Q_UNUSED(objectId)
    Q_UNUSED(nProps)
    Q_UNUSED(properties)

    AudioDevCoreAudio *self = static_cast<AudioDevCoreAudio *>(audioDev);

    if (self)
        emit self->defaultInputChanged(self->defaultInput());

    return noErr;
}

OSStatus AudioDevCoreAudio::defaultOutputDeviceChangedCallback(AudioObjectID objectId,
                                                               UInt32 nProps,
                                                               const AudioObjectPropertyAddress *properties,
                                                               void *audioDev)
{
    Q_UNUSED(objectId)
    Q_UNUSED(nProps)
    Q_UNUSED(properties)

    AudioDevCoreAudio *self = static_cast<AudioDevCoreAudio *>(audioDev);

    if (self)
        emit self->defaultOutputChanged(self->defaultOutput());

    return noErr;
}

OSStatus AudioDevCoreAudio::audioCallback(void *audioDev,
                                          AudioUnitRenderActionFlags *actionFlags,
                                          const AudioTimeStamp *timeStamp,
                                          UInt32 busNumber,
                                          UInt32 nFrames,
                                          AudioBufferList *data)
{
    AudioDevCoreAudio *self = static_cast<AudioDevCoreAudio *>(audioDev);

    if (!self)
        return noErr;

    if (self->m_isInput) {
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
