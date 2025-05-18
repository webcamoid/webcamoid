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

#include <QAbstractEventDispatcher>
#include <QAudioDevice>
#include <QAudioSink>
#include <QAudioSource>
#include <QMap>
#include <QMediaDevices>
#include <QMutex>
#include <QVector>
#include <QWaitCondition>
#include <QtConcurrent>
#include <QtDebug>
#include <akaudiopacket.h>
#include <akaudioconverter.h>

#if (defined(Q_OS_ANDROID) || defined(Q_OS_OSX)) && QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QPermissions>
#endif

#include "audiodevqt.h"
#include "audiodevicebuffer.h"

#define BUFFER_SIZE 1024 // In samples

using SampleFormatMap = QMap<QAudioFormat::SampleFormat, AkAudioCaps::SampleFormat>;

inline SampleFormatMap initSampleFormatMap()
{
    SampleFormatMap sampleFormat {
        {QAudioFormat::UInt8, AkAudioCaps::SampleFormat_u8 },
        {QAudioFormat::Int16, AkAudioCaps::SampleFormat_s16},
        {QAudioFormat::Int32, AkAudioCaps::SampleFormat_s32},
        {QAudioFormat::Float, AkAudioCaps::SampleFormat_flt},
    };

    return sampleFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatMap, sampleFormats, (initSampleFormatMap()))

using AudioSourcePtr = QSharedPointer<QAudioSource>;
using AudioSinkPtr = QSharedPointer<QAudioSink>;

class AudioDevQtPrivate
{
    public:
        AudioDevQt *self;
        QString m_error;
        QString m_defaultSink;
        QString m_defaultSource;
        QStringList m_sinks;
        QStringList m_sources;
        QMap<QString, QString> m_descriptionMap;
        QMap<QString, QList<AkAudioCaps::SampleFormat>> m_supportedFormats;
        QMap<QString, QList<AkAudioCaps::ChannelLayout>> m_supportedLayouts;
        QMap<QString, QList<int>> m_supportedSampleRates;
        QMap<QString, AkAudioCaps> m_preferredFormat;
        QMediaDevices m_mediaDevices;
        AudioSourcePtr m_audioSource;
        AudioSinkPtr m_audioSink;
        AudioDeviceBuffer m_audioBuffer;
        QMutex m_mutex;
        AkAudioConverter m_audioConvert;
        int m_samples {0};
        size_t m_maxBufferSize {0};
        bool m_isCapture {false};
        bool m_hasAudioCapturePermissions {false};

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        QMicrophonePermission m_microphonePermission;
        bool m_permissionResultReady {false};
#endif

        explicit AudioDevQtPrivate(AudioDevQt *self);
        void updateDevices();
};

AudioDevQt::AudioDevQt(QObject *parent):
    AudioDev(parent)
{
    this->d = new AudioDevQtPrivate(this);

    QObject::connect(&this->d->m_mediaDevices,
                     &QMediaDevices::audioInputsChanged,
                     this,
                     [this] () {
                         this->d->updateDevices();
                     });
    QObject::connect(&this->d->m_mediaDevices,
                     &QMediaDevices::audioOutputsChanged,
                     this,
                     [this] () {
                         this->d->updateDevices();
                     });

#if (defined(Q_OS_ANDROID) || defined(Q_OS_OSX)) && QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    auto permissionStatus =
            qApp->checkPermission(this->d->m_microphonePermission);

    if (permissionStatus == Qt::PermissionStatus::Granted) {
        qInfo() << "Permission granted for audio capture with Qt Audio";
        this->d->m_hasAudioCapturePermissions = true;
    } else {
        this->d->m_permissionResultReady = false;
        qApp->requestPermission(this->d->m_microphonePermission,
                                this,
                                [this] (const QPermission &permission) {
                                    if (permission.status() == Qt::PermissionStatus::Granted) {
                                        qInfo() << "Permission granted for audio capture with Qt Audio";
                                        this->d->m_hasAudioCapturePermissions = true;
                                    } else {
                                        qWarning() << "Permission denied for audio capture with Qt Audio";
                                    }

                                    this->d->m_permissionResultReady = true;
                                });

        while (!this->d->m_permissionResultReady) {
            auto eventDispatcher = QThread::currentThread()->eventDispatcher();

            if (eventDispatcher)
                eventDispatcher->processEvents(QEventLoop::AllEvents);
        }
    }
#endif

    this->d->updateDevices();
}

AudioDevQt::~AudioDevQt()
{
    this->uninit();

    delete this->d;
}

QString AudioDevQt::error() const
{
    return this->d->m_error;
}

QString AudioDevQt::defaultInput()
{
    return this->d->m_defaultSource;
}

QString AudioDevQt::defaultOutput()
{
    return this->d->m_defaultSink;
}

QStringList AudioDevQt::inputs()
{
    return this->d->m_sources;
}

QStringList AudioDevQt::outputs()
{
    return this->d->m_sinks;
}

QString AudioDevQt::description(const QString &device)
{
    return this->d->m_descriptionMap.value(device);
}

AkAudioCaps AudioDevQt::preferredFormat(const QString &device)
{
    return this->d->m_preferredFormat.value(device);
}

QList<AkAudioCaps::SampleFormat> AudioDevQt::supportedFormats(const QString &device)
{
    return this->d->m_supportedFormats.value(device);
}

QList<AkAudioCaps::ChannelLayout> AudioDevQt::supportedChannelLayouts(const QString &device)
{
    return this->d->m_supportedLayouts.value(device);
}

QList<int> AudioDevQt::supportedSampleRates(const QString &device)
{
    return this->d->m_supportedSampleRates.value(device);
}

bool AudioDevQt::init(const QString &device, const AkAudioCaps &caps)
{
    this->d->m_audioConvert.setOutputCaps(caps);
    this->d->m_audioConvert.reset();

    int blockSize = BUFFER_SIZE
                    * caps.channels()
                    * caps.bps()
                    / 8;

    this->d->m_mutex.lock();

    this->d->m_audioBuffer.setBlockSize(blockSize);
    this->d->m_audioBuffer.setMaxBufferSize(4 * blockSize);
    this->d->m_audioBuffer.open(QIODevice::ReadWrite);
    bool ok = false;

    if (this->d->m_sinks.contains(device)) {
        for (auto &audioDevice: QMediaDevices::audioOutputs())
            if (audioDevice.id() == device) {
                QAudioFormat format;
                format.setSampleFormat(sampleFormats->key(caps.format()));
                format.setChannelCount(AkAudioCaps::channelCount(caps.layout()));
                format.setSampleRate(caps.rate());

                this->d->m_audioSink = AudioSinkPtr::create(audioDevice, format, this);
                this->d->m_audioSink->start(&this->d->m_audioBuffer);
                ok = true;

                break;
            }
    } else if (this->d->m_sources.contains(device)) {
        for (auto &audioDevice: QMediaDevices::audioInputs())
            if (audioDevice.id() == device) {
                QAudioFormat format;
                format.setSampleFormat(sampleFormats->key(caps.format()));
                format.setChannelCount(AkAudioCaps::channelCount(caps.layout()));
                format.setSampleRate(caps.rate());

                this->d->m_audioSource = AudioSourcePtr::create(audioDevice, format, this);
                this->d->m_audioSource->start(&this->d->m_audioBuffer);
                ok = true;

                break;
            }
    }

    this->d->m_mutex.unlock();

    return ok;
}

QByteArray AudioDevQt::read()
{
    this->d->m_mutex.lock();
    auto buffer = this->d->m_audioBuffer.readAll();
    this->d->m_mutex.unlock();

    return buffer;
}

bool AudioDevQt::write(const AkAudioPacket &packet)
{
    auto audioPacket = this->d->m_audioConvert.convert(packet);

    if (!audioPacket)
        return false;

    this->d->m_mutex.lock();
    this->d->m_audioBuffer.write(QByteArray(audioPacket.constData(),
                                            audioPacket.size()));
    this->d->m_mutex.unlock();

    return true;
}

bool AudioDevQt::uninit()
{
    this->d->m_mutex.lock();

    if (this->d->m_audioSource) {
        this->d->m_audioSource->stop();
        this->d->m_audioSource = {};
    }

    if (this->d->m_audioSink) {
        this->d->m_audioSink->stop();
        this->d->m_audioSink = {};
    }

    this->d->m_audioBuffer.close();

    this->d->m_mutex.unlock();

    return true;
}

AudioDevQtPrivate::AudioDevQtPrivate(AudioDevQt *self):
    self(self)
{
}

void AudioDevQtPrivate::updateDevices()
{
    decltype(this->m_sources) inputs;
    decltype(this->m_sinks) outputs;
    decltype(this->m_descriptionMap) descriptionMap;
    decltype(this->m_supportedFormats) supportedFormats;
    decltype(this->m_supportedLayouts) supportedChannels;
    decltype(this->m_supportedSampleRates) supportedSampleRates;
    decltype(this->m_preferredFormat) preferredFormats;

    // Update devices

    if (this->m_hasAudioCapturePermissions) {
        for (auto &input: QMediaDevices::audioInputs()) {
            QList<AkAudioCaps::SampleFormat> _supportedFormats;
            QList<AkAudioCaps::ChannelLayout> _supportedLayouts;
            QList<int> _supportedSampleRates;

            for (auto &format: input.supportedSampleFormats()) {
                auto sampleFormat = sampleFormats->value(format, AkAudioCaps::SampleFormat_none);

                if (sampleFormat != AkAudioCaps::SampleFormat_none
                    && !_supportedFormats.contains(sampleFormat))
                    _supportedFormats << sampleFormat;
            }

            if (input.minimumChannelCount() <= 1)
                _supportedLayouts << AkAudioCaps::Layout_mono;

            if (input.maximumChannelCount() >= 2)
                _supportedLayouts << AkAudioCaps::Layout_stereo;

            for (auto &rate: this->self->commonSampleRates())
                if (rate >= input.minimumSampleRate() && rate <= input.maximumSampleRate())
                    _supportedSampleRates << rate;

            if (!_supportedFormats.isEmpty()
                && !_supportedLayouts.isEmpty()
                && !_supportedSampleRates.isEmpty()) {
                inputs << input.id();
                descriptionMap[input.id()] = input.description();
                supportedFormats[input.id()] = _supportedFormats;
                supportedChannels[input.id()] = _supportedLayouts;
                supportedSampleRates[input.id()] = _supportedSampleRates;

                auto format = input.preferredFormat();
                auto sampleFormat =
                    sampleFormats->value(format.sampleFormat(),
                                         AkAudioCaps::SampleFormat_none);

                if (sampleFormat == AkAudioCaps::SampleFormat_none)
                    sampleFormat = _supportedFormats.last();

                auto channels = qBound(1, format.channelCount(), 2);

                preferredFormats[input.id()] = {sampleFormat,
                                                channels == 1?
                                                    AkAudioCaps::Layout_mono:
                                                    AkAudioCaps::Layout_stereo,
                                                false,
                                                format.sampleRate()};
            }
        }
    }

    for (auto &output: QMediaDevices::audioOutputs()) {
        QList<AkAudioCaps::SampleFormat> _supportedFormats;
        QList<AkAudioCaps::ChannelLayout> _supportedLayouts;
        QList<int> _supportedSampleRates;

        for (auto &format: output.supportedSampleFormats()) {
            auto sampleFormat = sampleFormats->value(format, AkAudioCaps::SampleFormat_none);

            if (sampleFormat != AkAudioCaps::SampleFormat_none
                && !_supportedFormats.contains(sampleFormat))
                _supportedFormats << sampleFormat;
        }

        if (output.minimumChannelCount() <= 1)
            _supportedLayouts << AkAudioCaps::Layout_mono;

        if (output.maximumChannelCount() >= 2)
            _supportedLayouts << AkAudioCaps::Layout_stereo;

        for (auto &rate: this->self->commonSampleRates())
            if (rate >= output.minimumSampleRate() && rate <= output.maximumSampleRate())
                _supportedSampleRates << rate;

        if (!_supportedFormats.isEmpty()
            && !_supportedLayouts.isEmpty()
            && !_supportedSampleRates.isEmpty()) {
            outputs << output.id();
            descriptionMap[output.id()] = output.description();
            supportedFormats[output.id()] = _supportedFormats;
            supportedChannels[output.id()] = _supportedLayouts;
            supportedSampleRates[output.id()] = _supportedSampleRates;

            auto format = output.preferredFormat();
            auto sampleFormat =
                sampleFormats->value(format.sampleFormat(),
                                     AkAudioCaps::SampleFormat_none);

            if (sampleFormat == AkAudioCaps::SampleFormat_none)
                sampleFormat = _supportedFormats.last();

            auto channels = qBound(1, format.channelCount(), 2);

            preferredFormats[output.id()] = {sampleFormat,
                                             channels == 1?
                                                 AkAudioCaps::Layout_mono:
                                                 AkAudioCaps::Layout_stereo,
                                             false,
                                             format.sampleRate()};
        }
    }

    if (this->m_supportedFormats != supportedFormats)
        this->m_supportedFormats = supportedFormats;

    if (this->m_supportedLayouts != supportedChannels)
        this->m_supportedLayouts = supportedChannels;

    if (this->m_supportedSampleRates != supportedSampleRates)
        this->m_supportedSampleRates = supportedSampleRates;

    if (this->m_descriptionMap != descriptionMap)
        this->m_descriptionMap = descriptionMap;

    if (this->m_preferredFormat != preferredFormats)
        this->m_preferredFormat = preferredFormats;

    if (this->m_sources != inputs) {
        this->m_sources = inputs;
        emit self->inputsChanged(inputs);
    }

    if (this->m_sinks != outputs) {
        this->m_sinks = outputs;
        emit self->outputsChanged(outputs);
    }

    if (this->m_hasAudioCapturePermissions) {
        auto defaultInput = QMediaDevices::defaultAudioInput().id();

        if (this->m_defaultSource != defaultInput) {
            this->m_defaultSource = defaultInput;
            emit self->defaultInputChanged(defaultInput);
        }
    }

    auto defaultOutput = QMediaDevices::defaultAudioOutput().id();

    if (this->m_defaultSink != defaultOutput) {
        this->m_defaultSink = defaultOutput;
        emit self->defaultOutputChanged(defaultOutput);
    }
}

#include "moc_audiodevqt.cpp"
