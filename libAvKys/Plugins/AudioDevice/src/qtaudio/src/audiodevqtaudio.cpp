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

#include "audiodevqtaudio.h"

#define BUFFER_SIZE 1024 // In samples

AudioDevQtAudio::AudioDevQtAudio(QObject *parent):
    AudioDev(parent),
    m_inputDeviceBuffer(NULL),
    m_input(NULL),
    m_output(NULL)
{
    this->updateDevices();
}

AudioDevQtAudio::~AudioDevQtAudio()
{
    this->uninit();
}

QString AudioDevQtAudio::error() const
{
    return this->m_error;
}

QString AudioDevQtAudio::defaultInput()
{
    return this->m_defaultSource;
}

QString AudioDevQtAudio::defaultOutput()
{
    return this->m_defaultSink;
}

QStringList AudioDevQtAudio::inputs()
{
    return this->m_sources.values();
}

QStringList AudioDevQtAudio::outputs()
{
    return this->m_sinks.values();
}

QString AudioDevQtAudio::description(const QString &device)
{
    return this->m_pinDescriptionMap.value(device);
}

AkAudioCaps AudioDevQtAudio::preferredFormat(const QString &device)
{
    return this->m_pinCapsMap.value(device);
}

QList<AkAudioCaps::SampleFormat> AudioDevQtAudio::supportedFormats(const QString &device)
{
    return this->m_supportedFormats.value(device);
}

QList<int> AudioDevQtAudio::supportedChannels(const QString &device)
{
    return this->m_supportedChannels.value(device);
}

QList<int> AudioDevQtAudio::supportedSampleRates(const QString &device)
{
    return this->m_supportedSampleRates.value(device);
}

bool AudioDevQtAudio::init(const QString &device, const AkAudioCaps &caps)
{
    int blockSize = BUFFER_SIZE
                  * caps.channels()
                  * caps.bps()
                  / 8;

    this->m_mutex.lock();
    this->m_outputDeviceBuffer.setBlockSize(blockSize);
    this->m_outputDeviceBuffer.setMaxBufferSize(4 * blockSize);
    this->m_outputDeviceBuffer.open(QIODevice::ReadWrite);

    if (device.endsWith(":Output")) {
        auto deviceInfo = this->m_sinks.key(device);
        auto format = this->qtFormatFromCaps(caps);
        this->m_output = new QAudioOutput(deviceInfo, format);
        this->m_output->start(&this->m_outputDeviceBuffer);

        if (this->m_output->error() != QAudio::NoError) {
            this->m_mutex.unlock();
            this->uninit();

            return false;
        }
    } else if (device.endsWith(":Input")) {
        auto deviceInfo = this->m_sources.key(device);
        auto format = this->qtFormatFromCaps(caps);
        this->m_input = new QAudioInput(deviceInfo, format);
        this->m_inputDeviceBuffer = this->m_input->start();

        if (!this->m_inputDeviceBuffer
            || this->m_input->error() != QAudio::NoError) {
            this->m_mutex.unlock();
            this->uninit();

            return false;
        }
    } else {
        this->m_mutex.unlock();
        this->uninit();

        return false;
    }

    this->m_mutex.unlock();

    return true;
}

QByteArray AudioDevQtAudio::read(int samples)
{
    QByteArray buffer;

    this->m_mutex.lock();

    if (this->m_inputDeviceBuffer) {
        auto format = this->m_input->format();
        auto bufferSize = format.channelCount()
                        * format.sampleSize()
                        * samples
                        / 8;
        auto readBytes = bufferSize;

        while (buffer.size() < bufferSize) {
            auto data = this->m_inputDeviceBuffer->read(readBytes);
            buffer.append(data);
            readBytes -= data.size();
        }
    }

    this->m_mutex.unlock();

    return buffer;
}

bool AudioDevQtAudio::write(const AkAudioPacket &packet)
{
    this->m_mutex.lock();
    this->m_outputDeviceBuffer.write(packet.buffer());
    this->m_mutex.unlock();

    return true;
}

bool AudioDevQtAudio::uninit()
{
    this->m_mutex.unlock();

    this->m_outputDeviceBuffer.close();

    if (this->m_input) {
        this->m_input->stop();
        delete this->m_input;
        this->m_input = NULL;
    }

    if (this->m_output) {
        this->m_output->stop();
        delete this->m_output;
        this->m_output = NULL;
    }

    this->m_inputDeviceBuffer = NULL;
    this->m_mutex.unlock();

    return true;
}

AkAudioCaps::SampleFormat AudioDevQtAudio::qtFormatToAk(const QAudioFormat &format) const
{
    return AkAudioCaps::sampleFormatFromProperties(
                format.sampleType() == QAudioFormat::SignedInt?
                    AkAudioCaps::SampleType_int:
                format.sampleType() == QAudioFormat::UnSignedInt?
                    AkAudioCaps::SampleType_uint:
                format.sampleType() == QAudioFormat::Float?
                    AkAudioCaps::SampleType_float:
                    AkAudioCaps::SampleType_unknown,
                format.sampleSize(),
                format.byteOrder() == QAudioFormat::LittleEndian?
                    Q_LITTLE_ENDIAN: Q_BIG_ENDIAN,
                false);
}

QAudioFormat AudioDevQtAudio::qtFormatFromCaps(const AkAudioCaps &caps) const
{
    QAudioFormat audioFormat;
    audioFormat.setByteOrder(AkAudioCaps::endianness(caps.format()) == Q_LITTLE_ENDIAN?
                                 QAudioFormat::LittleEndian: QAudioFormat::BigEndian);
    audioFormat.setChannelCount(caps.channels());
    audioFormat.setCodec("audio/pcm");
    audioFormat.setSampleRate(caps.rate());
    audioFormat.setSampleSize(caps.bps());
    auto sampleType = AkAudioCaps::sampleType(caps.format());
    audioFormat.setSampleType(sampleType == AkAudioCaps::SampleType_int?
                                  QAudioFormat::SignedInt:
                              sampleType == AkAudioCaps::SampleType_uint?
                                  QAudioFormat::UnSignedInt:
                              sampleType == AkAudioCaps::SampleType_float?
                                  QAudioFormat::Float:
                                  QAudioFormat::Unknown);

    return audioFormat;
}

void AudioDevQtAudio::updateDevices()
{
    decltype(this->m_defaultSink) defaultSink;
    decltype(this->m_defaultSource) defaultSource;
    decltype(this->m_sinks) sinks;
    decltype(this->m_sources) sources;
    decltype(this->m_pinCapsMap) pinCapsMap;
    decltype(this->m_pinDescriptionMap) pinDescriptionMap;
    decltype(this->m_supportedFormats) supportedFormats;
    decltype(this->m_supportedChannels) supportedChannels;
    decltype(this->m_supportedSampleRates) supportedSampleRates;

    for (auto &mode: QVector<QAudio::Mode> {QAudio::AudioInput,
                                            QAudio::AudioOutput}) {
        for (const auto &device: QAudioDeviceInfo::availableDevices(mode)) {
            auto description = device.deviceName();
            auto deviceName = description;

            if (mode == QAudio::AudioInput) {
                deviceName += ":Input";
                sources[device] = deviceName;
            } else {
                deviceName += ":Output";
                sinks[device] = deviceName;
            }

            auto preferredFormat = device.preferredFormat();

            pinCapsMap[deviceName] =
                    AkAudioCaps(this->qtFormatToAk(preferredFormat),
                                preferredFormat.channelCount(),
                                preferredFormat.sampleRate());
            pinDescriptionMap[deviceName] = description;
            QList<AkAudioCaps::SampleFormat> _supportedFormats;
            QAudioFormat audioFormat;
            audioFormat.setChannelCount(2);
            audioFormat.setCodec("audio/pcm");
            audioFormat.setSampleRate(44100);

            for (auto &endianness: device.supportedByteOrders())
                for (auto &sampleSize: device.supportedSampleSizes())
                    for (auto &sampleType: device.supportedSampleTypes()) {
                        audioFormat.setByteOrder(endianness);
                        audioFormat.setSampleSize(sampleSize);
                        audioFormat.setSampleType(sampleType);
                        auto format = this->qtFormatToAk(audioFormat);

                        if (format != AkAudioCaps::SampleFormat_none)
                            _supportedFormats << format;
                    }

            supportedFormats[deviceName] = _supportedFormats;
            supportedChannels[deviceName] = device.supportedChannelCounts();
            supportedSampleRates[deviceName] = device.supportedSampleRates();
        }
    }

    defaultSource = QAudioDeviceInfo::defaultInputDevice().deviceName() + ":Input";
    defaultSink = QAudioDeviceInfo::defaultOutputDevice().deviceName() + ":Output";

    if (this->m_pinCapsMap != pinCapsMap)
        this->m_pinCapsMap = pinCapsMap;

    if (this->m_supportedFormats != supportedFormats)
        this->m_supportedFormats = supportedFormats;

    if (this->m_supportedChannels != supportedChannels)
        this->m_supportedChannels = supportedChannels;

    if (this->m_supportedSampleRates != supportedSampleRates)
        this->m_supportedSampleRates = supportedSampleRates;

    if (this->m_pinDescriptionMap != pinDescriptionMap)
        this->m_pinDescriptionMap = pinDescriptionMap;

    if (this->m_sources != sources) {
        this->m_sources = sources;
        emit this->inputsChanged(sources.values());
    }

    if (this->m_sinks != sinks) {
        this->m_sinks = sinks;
        emit this->outputsChanged(sinks.values());
    }

    QString defaultOutput = sinks.isEmpty()? "": defaultSink;
    QString defaultInput = sources.isEmpty()? "": defaultSource;

    if (this->m_defaultSource != defaultInput) {
        this->m_defaultSource = defaultInput;
        emit this->defaultInputChanged(defaultInput);
    }

    if (this->m_defaultSink != defaultOutput) {
        this->m_defaultSink = defaultOutput;
        emit this->defaultOutputChanged(defaultOutput);
    }
}
