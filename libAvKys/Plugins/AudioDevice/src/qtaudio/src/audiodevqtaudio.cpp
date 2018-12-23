/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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
#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioDeviceInfo>
#include <akaudiopacket.h>

#include "audiodevqtaudio.h"
#include "audiodevicebuffer.h"

#define BUFFER_SIZE 1024 // In samples

inline bool operator <(const QAudioDeviceInfo &info1,
                       const QAudioDeviceInfo &info2)
{
    return info1.deviceName() < info2.deviceName();
}

class AudioDevQtAudioPrivate
{
    public:
        QString m_error;
        QString m_defaultSink;
        QString m_defaultSource;
        QMap<QAudioDeviceInfo, QString> m_sinks;
        QMap<QAudioDeviceInfo, QString> m_sources;
        QMap<QString, AkAudioCaps> m_pinCapsMap;
        QMap<QString, QString> m_pinDescriptionMap;
        QMap<QString, QList<AkAudioCaps::SampleFormat>> m_supportedFormats;
        QMap<QString, QList<int>> m_supportedChannels;
        QMap<QString, QList<int>> m_supportedSampleRates;
        AudioDeviceBuffer m_outputDeviceBuffer;
        QIODevice *m_inputDeviceBuffer {nullptr};
        QAudioInput *m_input {nullptr};
        QAudioOutput *m_output {nullptr};
        QMutex m_mutex;

        AkAudioCaps::SampleFormat qtFormatToAk(const QAudioFormat &format) const;
        QAudioFormat qtFormatFromCaps(const AkAudioCaps &caps) const;
};

AudioDevQtAudio::AudioDevQtAudio(QObject *parent):
    AudioDev(parent)
{
    this->d = new AudioDevQtAudioPrivate;
    this->updateDevices();
}

AudioDevQtAudio::~AudioDevQtAudio()
{
    this->uninit();
    delete this->d;
}

QString AudioDevQtAudio::error() const
{
    return this->d->m_error;
}

QString AudioDevQtAudio::defaultInput()
{
    return this->d->m_defaultSource;
}

QString AudioDevQtAudio::defaultOutput()
{
    return this->d->m_defaultSink;
}

QStringList AudioDevQtAudio::inputs()
{
    return this->d->m_sources.values();
}

QStringList AudioDevQtAudio::outputs()
{
    return this->d->m_sinks.values();
}

QString AudioDevQtAudio::description(const QString &device)
{
    return this->d->m_pinDescriptionMap.value(device);
}

AkAudioCaps AudioDevQtAudio::preferredFormat(const QString &device)
{
    return this->d->m_pinCapsMap.value(device);
}

QList<AkAudioCaps::SampleFormat> AudioDevQtAudio::supportedFormats(const QString &device)
{
    return this->d->m_supportedFormats.value(device);
}

QList<int> AudioDevQtAudio::supportedChannels(const QString &device)
{
    return this->d->m_supportedChannels.value(device);
}

QList<int> AudioDevQtAudio::supportedSampleRates(const QString &device)
{
    return this->d->m_supportedSampleRates.value(device);
}

bool AudioDevQtAudio::init(const QString &device, const AkAudioCaps &caps)
{
    int blockSize = BUFFER_SIZE
                  * caps.channels()
                  * caps.bps()
                  / 8;

    this->d->m_mutex.lock();
    this->d->m_outputDeviceBuffer.setBlockSize(blockSize);
    this->d->m_outputDeviceBuffer.setMaxBufferSize(4 * blockSize);
    this->d->m_outputDeviceBuffer.open(QIODevice::ReadWrite);

    if (device.endsWith(":Output")) {
        auto deviceInfo = this->d->m_sinks.key(device);
        auto format = this->d->qtFormatFromCaps(caps);
        this->d->m_output = new QAudioOutput(deviceInfo, format);
        this->d->m_output->start(&this->d->m_outputDeviceBuffer);

        if (this->d->m_output->error() != QAudio::NoError) {
            this->d->m_mutex.unlock();
            this->uninit();

            return false;
        }
    } else if (device.endsWith(":Input")) {
        auto deviceInfo = this->d->m_sources.key(device);
        auto format = this->d->qtFormatFromCaps(caps);
        this->d->m_input = new QAudioInput(deviceInfo, format);
        this->d->m_inputDeviceBuffer = this->d->m_input->start();

        if (!this->d->m_inputDeviceBuffer
            || this->d->m_input->error() != QAudio::NoError) {
            this->d->m_mutex.unlock();
            this->uninit();

            return false;
        }
    } else {
        this->d->m_mutex.unlock();
        this->uninit();

        return false;
    }

    this->d->m_mutex.unlock();

    return true;
}

QByteArray AudioDevQtAudio::read(int samples)
{
    QByteArray buffer;

    this->d->m_mutex.lock();

    if (this->d->m_inputDeviceBuffer) {
        auto format = this->d->m_input->format();
        auto bufferSize = format.channelCount()
                        * format.sampleSize()
                        * samples
                        / 8;
        auto readBytes = bufferSize;

        while (buffer.size() < bufferSize) {
            auto data = this->d->m_inputDeviceBuffer->read(readBytes);
            buffer.append(data);
            readBytes -= data.size();
        }
    }

    this->d->m_mutex.unlock();

    return buffer;
}

bool AudioDevQtAudio::write(const AkAudioPacket &packet)
{
    this->d->m_mutex.lock();
    this->d->m_outputDeviceBuffer.write(packet.buffer());
    this->d->m_mutex.unlock();

    return true;
}

bool AudioDevQtAudio::uninit()
{
    this->d->m_mutex.lock();

    this->d->m_outputDeviceBuffer.close();

    if (this->d->m_input) {
        this->d->m_input->stop();
        delete this->d->m_input;
        this->d->m_input = nullptr;
    }

    if (this->d->m_output) {
        this->d->m_output->stop();
        delete this->d->m_output;
        this->d->m_output = nullptr;
    }

    this->d->m_inputDeviceBuffer = nullptr;
    this->d->m_mutex.unlock();

    return true;
}

AkAudioCaps::SampleFormat AudioDevQtAudioPrivate::qtFormatToAk(const QAudioFormat &format) const
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

QAudioFormat AudioDevQtAudioPrivate::qtFormatFromCaps(const AkAudioCaps &caps) const
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
    decltype(this->d->m_defaultSink) defaultSink;
    decltype(this->d->m_defaultSource) defaultSource;
    decltype(this->d->m_sinks) sinks;
    decltype(this->d->m_sources) sources;
    decltype(this->d->m_pinCapsMap) pinCapsMap;
    decltype(this->d->m_pinDescriptionMap) pinDescriptionMap;
    decltype(this->d->m_supportedFormats) supportedFormats;
    decltype(this->d->m_supportedChannels) supportedChannels;
    decltype(this->d->m_supportedSampleRates) supportedSampleRates;

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
                    AkAudioCaps(this->d->qtFormatToAk(preferredFormat),
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
                        auto format = this->d->qtFormatToAk(audioFormat);

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

    if (this->d->m_pinCapsMap != pinCapsMap)
        this->d->m_pinCapsMap = pinCapsMap;

    if (this->d->m_supportedFormats != supportedFormats)
        this->d->m_supportedFormats = supportedFormats;

    if (this->d->m_supportedChannels != supportedChannels)
        this->d->m_supportedChannels = supportedChannels;

    if (this->d->m_supportedSampleRates != supportedSampleRates)
        this->d->m_supportedSampleRates = supportedSampleRates;

    if (this->d->m_pinDescriptionMap != pinDescriptionMap)
        this->d->m_pinDescriptionMap = pinDescriptionMap;

    if (this->d->m_sources != sources) {
        this->d->m_sources = sources;
        emit this->inputsChanged(sources.values());
    }

    if (this->d->m_sinks != sinks) {
        this->d->m_sinks = sinks;
        emit this->outputsChanged(sinks.values());
    }

    QString defaultOutput = sinks.isEmpty()? "": defaultSink;
    QString defaultInput = sources.isEmpty()? "": defaultSource;

    if (this->d->m_defaultSource != defaultInput) {
        this->d->m_defaultSource = defaultInput;
        emit this->defaultInputChanged(defaultInput);
    }

    if (this->d->m_defaultSink != defaultOutput) {
        this->d->m_defaultSink = defaultOutput;
        emit this->defaultOutputChanged(defaultOutput);
    }
}

#include "moc_audiodevqtaudio.cpp"
