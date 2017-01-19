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

    this->updateDevices();
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
    return this->m_defaultSource;
}

QString AudioDevCoreAudio::defaultOutput()
{
    return this->m_defaultSink;
}

QStringList AudioDevCoreAudio::inputs()
{
    return this->m_sources;
}

QStringList AudioDevCoreAudio::outputs()
{
    return this->m_sinks;
}

QString AudioDevCoreAudio::description(const QString &device)
{
    return this->m_descriptionMap.value(device);
}

AkAudioCaps AudioDevCoreAudio::preferredFormat(const QString &device)
{
    return this->m_defaultCaps.value(device);
}

QList<AkAudioCaps::SampleFormat> AudioDevCoreAudio::supportedFormats(const QString &device)
{
    return this->m_supportedFormats.value(device);
}

QList<int> AudioDevCoreAudio::supportedChannels(const QString &device)
{
    return this->m_supportedChannels.value(device);
}

QList<int> AudioDevCoreAudio::supportedSampleRates(const QString &device)
{
    return this->m_supportedSampleRates.value(device);
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
    streamDescription.mChannelsPerFrame = UInt32(caps.channels());
    streamDescription.mBitsPerChannel = UInt32(AkAudioCaps::bitsPerSample(caps.format()));
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
    this->m_maxBufferSize = 2
                          * caps.bps()
                          * caps.channels()
                          * int(bufferSize) / 8;

    UInt32 nBuffers = streamDescription.mFormatFlags
                    & kAudioFormatFlagIsNonInterleaved?
                        streamDescription.mChannelsPerFrame: 1;

    this->m_bufferSize = bufferSize;
    this->m_bufferList =
            reinterpret_cast<AudioBufferList *>(malloc(sizeof(AudioBufferList)
                                                       + sizeof(AudioBuffer)
                                                       * nBuffers));
    this->m_bufferList->mNumberBuffers = nBuffers;

    for (UInt32 i = 0; i < nBuffers; i++) {
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

bool AudioDevCoreAudio::write(const AkAudioPacket &packet)
{
    this->m_mutex.lock();

    if (this->m_buffer.size() >= this->m_maxBufferSize)
        this->m_canWrite.wait(&this->m_mutex);

    this->m_buffer += packet.buffer();
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
        return QString(reinterpret_cast<const QChar *>(data), int(len));

    QVector<UniChar> str((int(len)));
    CFStringGetCharacters(cfstr, CFRangeMake(0, len), str.data());

    return QString(reinterpret_cast<const QChar *>(str.data()), str.size());
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

void AudioDevCoreAudio::clearBuffer()
{
    for (UInt32 i = 0; i < this->m_bufferList->mNumberBuffers; i++) {
        this->m_bufferList->mBuffers[i].mDataByteSize = 0;
        this->m_bufferList->mBuffers[i].mData = 0;
    }
}

QList<AkAudioCaps::SampleFormat> AudioDevCoreAudio::supportedCAFormats(AudioDeviceID deviceId,
                                                                       AudioObjectPropertyScope scope)
{
    // Read stream properties of the device.
    UInt32 propSize = 0;

    AudioObjectPropertyAddress propStreams = {
        kAudioDevicePropertyStreams,
        scope,
        kAudioObjectPropertyElementMaster
    };

    auto status = AudioObjectGetPropertyDataSize(deviceId,
                                                 &propStreams,
                                                 0,
                                                 NULL,
                                                 &propSize);

    int nStreams = propSize / sizeof(AudioStreamID);

    if (status != noErr || nStreams < 1)
        return QList<AkAudioCaps::SampleFormat>();

    QVector<AudioStreamID> streams(nStreams);

    status = AudioObjectGetPropertyData(deviceId,
                                        &propStreams,
                                        0,
                                        NULL,
                                        &propSize,
                                        streams.data());

    if (status != noErr)
        return QList<AkAudioCaps::SampleFormat>();

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

    static const QVector<AudioObjectPropertySelector> selectorType {
        kAudioStreamPropertyAvailablePhysicalFormats,
//        kAudioStreamPropertyAvailableVirtualFormats,
    };

    QList<AkAudioCaps::SampleFormat> supportedFormats;

    // Enumerate all formats supported by each stream.
    for (const AudioStreamID &stream: streams) {
        for (auto &selector: selectorType) {
            AudioObjectPropertyAddress availableFormats {
                selector,
                kAudioObjectPropertyScopeGlobal,
                kAudioObjectPropertyElementMaster
            };

            UInt32 propSize = 0;
            OSStatus status = AudioObjectGetPropertyDataSize(stream,
                                                             &availableFormats,
                                                             0,
                                                             NULL,
                                                             &propSize);

            if (status != noErr || !propSize)
                continue;

            UInt32 count = propSize / sizeof(AudioStreamRangedDescription);
            QVector<AudioStreamRangedDescription> streamDescriptions((int(count)));

            status = AudioObjectGetPropertyData(stream,
                                                &availableFormats,
                                                0,
                                                NULL,
                                                &propSize,
                                                streamDescriptions.data());

            if (status != noErr || !propSize)
                continue;

            for (auto &description: streamDescriptions) {
                auto format = this->descriptionToSampleFormat(description.mFormat);

                // Formats not supported by the converter are excluded.
                if (recommendedFormats.contains(format)
                    && !supportedFormats.contains(format)) {
                    supportedFormats << format;
                }
            }
        }
    }

    return supportedFormats;
}

QList<int> AudioDevCoreAudio::supportedCAChannels(AudioDeviceID deviceId,
                                                  AudioObjectPropertyScope scope)
{
    UInt32 propSize = 0;
    AudioObjectPropertyAddress streamConfiguration = {
        kAudioDevicePropertyStreamConfiguration,
        scope,
        kAudioObjectPropertyElementMaster
    };

    auto status = AudioObjectGetPropertyDataSize(deviceId,
                                                 &streamConfiguration,
                                                 0,
                                                 NULL,
                                                 &propSize);

    if (status != noErr)
        return QList<int>();

    int nBuffers = propSize / sizeof(AudioBufferList);

    if (nBuffers < 1)
        return QList<int>();

    QVector<AudioBufferList> buffers(nBuffers);

    status = AudioObjectGetPropertyData(deviceId,
                                        &streamConfiguration,
                                        0,
                                        NULL,
                                        &propSize,
                                        buffers.data());

    if (status != noErr)
        return QList<int>();

    QList<int> supportedCAChannels;

    for (auto &buffer: buffers) {
        int channels = 0;

        for (UInt32 i = 0; i < buffer.mNumberBuffers; i++)
            channels += buffer.mBuffers[i].mNumberChannels;

        if (!supportedCAChannels.contains(channels))
            supportedCAChannels << channels;
    }

    return supportedCAChannels;
}

QList<int> AudioDevCoreAudio::supportedCASampleRates(AudioDeviceID deviceId,
                                                     AudioObjectPropertyScope scope)
{
    UInt32 propSize = 0;
    AudioObjectPropertyAddress nominalSampleRates = {
        kAudioDevicePropertyAvailableNominalSampleRates,
        scope,
        kAudioObjectPropertyElementMaster
    };

    auto status = AudioObjectGetPropertyDataSize(deviceId,
                                                 &nominalSampleRates,
                                                 0,
                                                 NULL,
                                                 &propSize);

    if (status != noErr)
        return QList<int>();

    int nSampleRates = propSize / sizeof(AudioValueRange);

    if (nSampleRates < 1)
        return QList<int>();

    QVector<AudioValueRange> sampleRates(nSampleRates);

    status = AudioObjectGetPropertyData(deviceId,
                                        &nominalSampleRates,
                                        0,
                                        NULL,
                                        &propSize,
                                        sampleRates.data());

    if (status != noErr)
        return QList<int>();

    QList<int> supportedSampleRates;

    for (auto &rate: sampleRates) {
        auto minRate = qRound(rate.mMinimum);

        if (!supportedSampleRates.contains(minRate))
            supportedSampleRates << minRate;

        auto maxRate = qRound(rate.mMaximum);

        if (!supportedSampleRates.contains(maxRate))
            supportedSampleRates << maxRate;
    }

    qSort(supportedSampleRates);

    return supportedSampleRates;
}

AkAudioCaps::SampleFormat AudioDevCoreAudio::descriptionToSampleFormat(const AudioStreamBasicDescription &streamDescription)
{
    UInt32 bps = streamDescription.mBitsPerChannel;
    auto formatType =
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
                                                   int(bps),
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

    auto self = static_cast<AudioDevCoreAudio *>(audioDev);
    self->updateDevices();

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

    auto self = static_cast<AudioDevCoreAudio *>(audioDev);

    if (self) {
        auto defaultInput = self->defaultDevice(true, NULL);

        if (defaultInput.isEmpty() && !self->m_sources.isEmpty())
            defaultInput = self->m_sources.first();

        if (self->m_defaultSource != defaultInput) {
            self->m_defaultSource = defaultInput;
            emit self->defaultInputChanged(defaultInput);
        }
    }

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

    auto self = static_cast<AudioDevCoreAudio *>(audioDev);

    if (self) {
        auto defaultOutput = self->defaultDevice(false, NULL);

        if (defaultOutput.isEmpty() && !self->m_sinks.isEmpty())
            defaultOutput = self->m_sinks.first();

        if (self->m_defaultSink != defaultOutput) {
            self->m_defaultSink = defaultOutput;
            emit self->defaultOutputChanged(defaultOutput);
        }
    }

    return noErr;
}

OSStatus AudioDevCoreAudio::audioCallback(void *audioDev,
                                          AudioUnitRenderActionFlags *actionFlags,
                                          const AudioTimeStamp *timeStamp,
                                          UInt32 busNumber,
                                          UInt32 nFrames,
                                          AudioBufferList *data)
{
    auto self = static_cast<AudioDevCoreAudio *>(audioDev);

    if (!self)
        return noErr;

    if (self->m_isInput) {
        self->clearBuffer();

        auto status =
                AudioUnitRender(self->m_audioUnit,
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
                                        int(self->m_bufferList->mBuffers[i].mDataByteSize));

        // We use a ring buffer and all old samples are discarded.
        if (self->m_buffer.size() > self->m_maxBufferSize) {
            int k = self->m_curCaps.bps()
                    * self->m_curCaps.channels();
            int bufferSize = k * int(self->m_maxBufferSize / k);

            self->m_buffer =
                self->m_buffer.mid(self->m_buffer.size() - bufferSize,
                                   bufferSize);
        }

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
            int copyBytes = qMin(int(data->mBuffers[i].mDataByteSize),
                                  self->m_buffer.size());

            if (copyBytes > 0) {
                memcpy(data->mBuffers[i].mData,
                       self->m_buffer.constData(),
                       size_t(copyBytes));
                self->m_buffer.remove(0, copyBytes);
            }

            if (self->m_buffer.size() <= self->m_maxBufferSize)
                self->m_canWrite.wakeAll();

            self->m_mutex.unlock();
        }
    }

    return noErr;
}

void AudioDevCoreAudio::updateDevices()
{    
    decltype(this->m_defaultSink) defaultOutput;
    decltype(this->m_defaultSource) defaultInput;
    decltype(this->m_sources) inputs;
    decltype(this->m_sinks) outputs;
    decltype(this->m_descriptionMap) descriptionMap;
    decltype(this->m_defaultCaps) defaultCaps;
    decltype(this->m_supportedFormats) supportedFormats;
    decltype(this->m_supportedChannels) supportedChannels;
    decltype(this->m_supportedSampleRates) supportedSampleRates;

    // List default devices
    defaultInput = this->defaultDevice(true, NULL);
    defaultOutput = this->defaultDevice(false, NULL);

    // List all devices
    static const AudioObjectPropertyAddress propDevices = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    UInt32 propSize = 0;

    if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,
                                       &propDevices,
                                       0,
                                       NULL,
                                       &propSize) == noErr) {
        int nDevices = propSize / sizeof(AudioDeviceID);

        if (nDevices > 0) {
            QVector<AudioDeviceID> devices(nDevices);

            if (AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                                &propDevices,
                                                0,
                                                NULL,
                                                &propSize,
                                                devices.data()) == noErr) {
                for (auto &deviceType: QVector<AudioObjectPropertyScope> {
                                           kAudioObjectPropertyScopeInput,
                                           kAudioObjectPropertyScopeOutput
                                       }) {
                    AudioObjectPropertyAddress propStreamFormat = {
                        kAudioDevicePropertyStreamFormat,
                        deviceType,
                        kAudioObjectPropertyElementMaster
                    };

                    for (auto &deviceId: devices) {
                        // Check if we can use this device.
                        propSize = sizeof(AudioStreamBasicDescription);
                        AudioStreamBasicDescription streamDescription;

                        auto status =
                                AudioObjectGetPropertyData(deviceId,
                                                           &propStreamFormat,
                                                           0,
                                                           NULL,
                                                           &propSize,
                                                           &streamDescription);

                        if (status != noErr)
                            continue;

                        // Get device description.
                        AudioObjectPropertyAddress propName = {
                            kAudioObjectPropertyName,
                            deviceType == kAudioObjectPropertyScopeInput?
                                kAudioDevicePropertyScopeInput:
                                kAudioDevicePropertyScopeInput,
                            kAudioObjectPropertyElementMaster
                        };

                        UInt32 propSize = sizeof(CFStringRef);
                        CFStringRef name;

                        status = AudioObjectGetPropertyData(deviceId,
                                                            &propName,
                                                            0,
                                                            NULL,
                                                            &propSize,
                                                            &name);

                        if (status != noErr)
                            continue;

                        auto description = this->CFStringToString(name);
                        CFRelease(name);
                        auto formats = this->supportedCAFormats(deviceId,
                                                                deviceType);
                        auto channels = this->supportedCAChannels(deviceId,
                                                                  deviceType);
                        auto sampleRates = this->supportedCASampleRates(deviceId,
                                                                        deviceType);

                        if (formats.isEmpty()
                            || channels.isEmpty()
                            || sampleRates.isEmpty())
                            continue;

                        QString devId;

                        // Append device to the list.
                        if (deviceType == kAudioObjectPropertyScopeInput) {
                            devId = QString("%1:%2").arg(INPUT_DEVICE).arg(deviceId);
                            inputs << devId;
                        } else {
                            devId = QString("%1:%2").arg(OUTPUT_DEVICE).arg(deviceId);
                            outputs << devId;
                        }

                        descriptionMap[devId] = description;
                        supportedFormats[devId] = formats;
                        supportedChannels[devId] = channels;
                        supportedSampleRates[devId] = sampleRates;
                        defaultCaps[devId] = AkAudioCaps(formats.first(),
                                                         channels.first(),
                                                         sampleRates.first());
                    }
                }
            }
        }
    }

    if (this->m_defaultCaps != defaultCaps)
        this->m_defaultCaps = defaultCaps;

    if (this->m_supportedFormats != supportedFormats)
        this->m_supportedFormats = supportedFormats;

    if (this->m_supportedChannels != supportedChannels)
        this->m_supportedChannels = supportedChannels;

    if (this->m_supportedSampleRates != supportedSampleRates)
        this->m_supportedSampleRates = supportedSampleRates;

    if (this->m_descriptionMap != descriptionMap)
        this->m_descriptionMap = descriptionMap;

    if (this->m_sources != inputs) {
        this->m_sources = inputs;
        emit this->inputsChanged(inputs);
    }

    if (this->m_sinks != outputs) {
        this->m_sinks = outputs;
        emit this->outputsChanged(outputs);
    }

    if (defaultInput.isEmpty() && !inputs.isEmpty())
        defaultInput = inputs.first();

    if (defaultOutput.isEmpty() && !outputs.isEmpty())
        defaultOutput = outputs.first();

    if (this->m_defaultSource != defaultInput) {
        this->m_defaultSource = defaultInput;
        emit this->defaultInputChanged(defaultInput);
    }

    if (this->m_defaultSink != defaultOutput) {
        this->m_defaultSink = defaultOutput;
        emit this->defaultOutputChanged(defaultOutput);
    }
}
