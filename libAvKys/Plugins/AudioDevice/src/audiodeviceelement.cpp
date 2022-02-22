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

#include <QFuture>
#include <QSharedPointer>
#include <QThreadPool>
#include <QTime>
#include <QtConcurrent>
#include <ak.h>
#include <akaudioconverter.h>
#include <akaudiopacket.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>

#include "audiodeviceelement.h"
#include "audiodev.h"

#define PAUSE_TIMEOUT 500
#define DUMMY_OUTPUT_DEVICE ":dummyout:"

#ifdef Q_OS_WIN32
#include <combaseapi.h>
#endif

using AudioDevPtr = QSharedPointer<AudioDev>;

class AudioDeviceElementPrivate
{
    public:
        AudioDeviceElement *self;
        QStringList m_inputs;
        QStringList m_outputs;
        QString m_device;
        AkAudioCaps m_caps;
        AudioDevPtr m_audioDevice;
        QString m_audioDeviceImpl;
        AkAudioConverter m_audioConvert;
        QThreadPool m_threadPool;
        QFuture<void> m_readFramesLoopResult;
        QMutex m_mutex;
        QMutex m_mutexLib;
        bool m_readFramesLoop {false};
        bool m_pause {false};

        explicit AudioDeviceElementPrivate(AudioDeviceElement *self);
        void readFramesLoop();
        void setInputs(const QStringList &inputs);
        void setOutputs(const QStringList &outputs);
        void linksChanged(const AkPluginLinks &links);
};

AudioDeviceElement::AudioDeviceElement():
    AkElement()
{
    this->d = new AudioDeviceElementPrivate(this);

    QObject::connect(akPluginManager,
                     &AkPluginManager::linksChanged,
                     this,
                     [this] (const AkPluginLinks &links) {
                        this->d->linksChanged(links);
                     });

    if (this->d->m_audioDevice) {
        QObject::connect(this->d->m_audioDevice.data(),
                         &AudioDev::defaultInputChanged,
                         this,
                         &AudioDeviceElement::defaultInputChanged);
        QObject::connect(this->d->m_audioDevice.data(),
                         &AudioDev::defaultOutputChanged,
                         this,
                         &AudioDeviceElement::defaultOutputChanged);
        QObject::connect(this->d->m_audioDevice.data(),
                         &AudioDev::latencyChanged,
                         this,
                         &AudioDeviceElement::latencyChanged);
        QObject::connect(this->d->m_audioDevice.data(),
                         &AudioDev::inputsChanged,
                         this,
                         [this] (const QStringList &inputs) {
                            this->d->setInputs(inputs);
                         });
        QObject::connect(this->d->m_audioDevice.data(),
                         &AudioDev::outputsChanged,
                         this,
                         [this] (const QStringList &outputs) {
                            this->d->setOutputs(outputs);
                         });

        this->d->m_inputs = this->d->m_audioDevice->inputs();
        this->d->m_outputs = this->d->m_audioDevice->outputs();
    }
}

AudioDeviceElement::~AudioDeviceElement()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QString AudioDeviceElement::defaultInput()
{
    QString defaultInput;

    this->d->m_mutexLib.lock();

    if (this->d->m_audioDevice)
        defaultInput = this->d->m_audioDevice->defaultInput();

    this->d->m_mutexLib.unlock();

    return defaultInput;
}

QString AudioDeviceElement::defaultOutput()
{
    QString defaultOutput;

    this->d->m_mutexLib.lock();

    if (this->d->m_audioDevice)
        defaultOutput = this->d->m_audioDevice->defaultOutput();

    this->d->m_mutexLib.unlock();

    return defaultOutput;
}

QStringList AudioDeviceElement::inputs()
{
    return this->d->m_inputs;
}

QStringList AudioDeviceElement::outputs()
{
    return this->d->m_outputs + QStringList {DUMMY_OUTPUT_DEVICE};
}

QString AudioDeviceElement::description(const QString &device)
{
    if (device == DUMMY_OUTPUT_DEVICE)
        return QString("Dummy Output");

    QString description;

    this->d->m_mutexLib.lock();

    if (this->d->m_audioDevice)
        description = this->d->m_audioDevice->description(device);

    this->d->m_mutexLib.unlock();

    return description;
}

QString AudioDeviceElement::device() const
{
    return this->d->m_device;
}

int AudioDeviceElement::latency() const
{
    if (this->d->m_audioDevice)
        return  this->d->m_audioDevice->latency();

    return 25;
}

AkAudioCaps AudioDeviceElement::caps() const
{
    return this->d->m_caps;
}

AkAudioCaps AudioDeviceElement::preferredFormat(const QString &device)
{
    if (device == DUMMY_OUTPUT_DEVICE)
        return AkAudioCaps(AkAudioCaps::SampleFormat_s16,
                           AkAudioCaps::Layout_stereo,
                           44100);

    AkAudioCaps preferredFormat;

    this->d->m_mutexLib.lock();

    if (this->d->m_audioDevice)
        preferredFormat = this->d->m_audioDevice->preferredFormat(device);

    this->d->m_mutexLib.unlock();

    return preferredFormat;
}

QList<AkAudioCaps::SampleFormat> AudioDeviceElement::supportedFormats(const QString &device)
{
    if (device == DUMMY_OUTPUT_DEVICE)
        return QList<AkAudioCaps::SampleFormat> {
            AkAudioCaps::SampleFormat_flt,
            AkAudioCaps::SampleFormat_s32,
            AkAudioCaps::SampleFormat_s16,
            AkAudioCaps::SampleFormat_u8
        };

    QList<AkAudioCaps::SampleFormat> supportedFormats;

    this->d->m_mutexLib.lock();

    if (this->d->m_audioDevice)
        supportedFormats = this->d->m_audioDevice->supportedFormats(device);

    this->d->m_mutexLib.unlock();

    return supportedFormats;
}

QList<AkAudioCaps::ChannelLayout> AudioDeviceElement::supportedChannelLayouts(const QString &device)
{
    if (device == DUMMY_OUTPUT_DEVICE)
        return QList<AkAudioCaps::ChannelLayout> {
            AkAudioCaps::Layout_mono,
            AkAudioCaps::Layout_stereo,
        };

    QList<AkAudioCaps::ChannelLayout> supportedChannelLayouts;

    this->d->m_mutexLib.lock();

    if (this->d->m_audioDevice)
        supportedChannelLayouts =
                this->d->m_audioDevice->supportedChannelLayouts(device);

    this->d->m_mutexLib.unlock();

    return supportedChannelLayouts;
}

QList<int> AudioDeviceElement::supportedSampleRates(const QString &device)
{
    if (device == DUMMY_OUTPUT_DEVICE)
        return this->d->m_audioDevice->commonSampleRates().toList();

    QList<int> supportedSampleRates;

    this->d->m_mutexLib.lock();

    if (this->d->m_audioDevice)
        supportedSampleRates = this->d->m_audioDevice->supportedSampleRates(device);

    this->d->m_mutexLib.unlock();

    return supportedSampleRates;
}

AudioDeviceElementPrivate::AudioDeviceElementPrivate(AudioDeviceElement *self):
    self(self)
{
    this->m_audioDevice = akPluginManager->create<AudioDev>("AudioSource/AudioDevice/Impl/*");
    this->m_audioDeviceImpl = akPluginManager->defaultPlugin("AudioSource/AudioDevice/Impl/*",
                                                             {"AudioDeviceImpl"}).id();

    if (this->m_audioDevice) {
        this->m_inputs = this->m_audioDevice->inputs();
        this->m_outputs = this->m_audioDevice->outputs();
    }
}

void AudioDeviceElementPrivate::readFramesLoop()
{
    if (!this->m_audioDevice)
        return;

#ifdef Q_OS_WIN32
    // Initialize the COM library in multithread mode.
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

    QString device = this->m_device;
    AkAudioCaps caps(this->m_caps);
    qint64 streamId = Ak::id();
    AkFrac timeBase(1, caps.rate());

    if (this->m_audioDevice->init(device, caps)) {
        while (this->m_readFramesLoop) {
            if (this->m_pause) {
                QThread::msleep(PAUSE_TIMEOUT);

                continue;
            }

            auto buffer = this->m_audioDevice->read();

            if (buffer.isEmpty())
                return;

            QByteArray oBuffer(buffer.size(), 0);
            memcpy(oBuffer.data(), buffer.constData(), size_t(buffer.size()));
            auto pts = qint64(QTime::currentTime().msecsSinceStartOfDay()
                              / timeBase.value() / 1e3);
            caps.setSamples(8 * buffer.size() / (caps.channels() * caps.bps()));
            AkAudioPacket packet;
            packet.caps() = caps;
            packet.buffer() = oBuffer;
            packet.setPts(pts);
            packet.setTimeBase(timeBase);
            packet.setIndex(0);
            packet.setId(streamId);

            emit self->oStream(packet);
        }

        this->m_audioDevice->uninit();
    }

#ifdef Q_OS_WIN32
    // Close COM library.
    CoUninitialize();
#endif
}

void AudioDeviceElementPrivate::setInputs(const QStringList &inputs)
{
    if (this->m_inputs == inputs)
        return;

    this->m_inputs = inputs;
    emit self->inputsChanged(inputs);
}

void AudioDeviceElementPrivate::setOutputs(const QStringList &outputs)
{
    if (this->m_outputs == outputs)
        return;

    this->m_outputs = outputs;
    emit self->outputsChanged(outputs);
}

void AudioDeviceElementPrivate::linksChanged(const AkPluginLinks &links)
{
    if (!links.contains("AudioSource/AudioDevice/Impl/*")
        || links["AudioSource/AudioDevice/Impl/*"] == this->m_audioDeviceImpl)
        return;

    auto state = self->state();
    self->setState(AkElement::ElementStateNull);

    bool isInput = this->m_inputs.contains(this->m_device);

    this->m_mutexLib.lock();
    int latency = 25;

    if (this->m_audioDevice)
        latency = this->m_audioDevice->latency();

    this->m_audioDevice =
            akPluginManager->create<AudioDev>("AudioSource/AudioDevice/Impl/*");
    this->m_mutexLib.unlock();

    this->m_audioDeviceImpl = links["AudioSource/AudioDevice/Impl/*"];

    if (!this->m_audioDevice)
        return;

    QObject::connect(this->m_audioDevice.data(),
                     &AudioDev::defaultInputChanged,
                     self,
                     &AudioDeviceElement::defaultInputChanged);
    QObject::connect(this->m_audioDevice.data(),
                     &AudioDev::defaultOutputChanged,
                     self,
                     &AudioDeviceElement::defaultOutputChanged);
    QObject::connect(this->m_audioDevice.data(),
                     &AudioDev::latencyChanged,
                     self,
                     &AudioDeviceElement::latencyChanged);
    QObject::connect(this->m_audioDevice.data(),
                     &AudioDev::inputsChanged,
                     self,
                     [this] (const QStringList &inputs) {
                        this->setInputs(inputs);
                     });
    QObject::connect(this->m_audioDevice.data(),
                     &AudioDev::outputsChanged,
                     self,
                     [this] (const QStringList &outputs) {
                        this->setOutputs(outputs);
                     });

    this->m_audioDevice->setLatency(latency);
    this->setInputs(this->m_audioDevice->inputs());
    this->setOutputs(this->m_audioDevice->outputs());
    emit self->defaultInputChanged(this->m_audioDevice->defaultInput());
    emit self->defaultOutputChanged(this->m_audioDevice->defaultOutput());

    if (this->m_device != DUMMY_OUTPUT_DEVICE) {
        self->setDevice(isInput?
                            this->m_audioDevice->defaultInput():
                            this->m_audioDevice->defaultOutput());
        auto preferredFormat =
                this->m_audioDevice->preferredFormat(this->m_device);
        self->setCaps(preferredFormat);
    }

    self->setState(state);
}

AkPacket AudioDeviceElement::iAudioStream(const AkAudioPacket &packet)
{
    if (!this->d->m_audioDevice)
        return AkPacket();

    this->d->m_mutex.lock();

    if (this->state() != ElementStatePlaying) {
        this->d->m_mutex.unlock();

        return AkPacket();
    }

    auto device = this->d->m_device;
    this->d->m_mutex.unlock();

    if (device == DUMMY_OUTPUT_DEVICE)
        QThread::usleep(ulong(1e6
                              * packet.caps().samples()
                              / packet.caps().rate()));
    else {
        AkPacket iPacket;
        this->d->m_mutex.lock();
        iPacket = this->d->m_audioConvert.convert(packet);
        this->d->m_mutex.unlock();

        if (iPacket) {
            this->d->m_mutexLib.lock();
            this->d->m_audioDevice->write(iPacket);
            this->d->m_mutexLib.unlock();
        }
    }

    return AkPacket();
}

void AudioDeviceElement::setDevice(const QString &device)
{
    if (this->d->m_device == device)
        return;

    this->d->m_device = device;
    emit this->deviceChanged(device);
    AkAudioCaps preferredFormat;

    if (this->d->m_audioDevice)
        preferredFormat = this->d->m_audioDevice->preferredFormat(device);

    this->setCaps(preferredFormat);
}

void AudioDeviceElement::setLatency(int latency)
{
    if (this->d->m_audioDevice)
        this->d->m_audioDevice->setLatency(latency);
}

void AudioDeviceElement::setCaps(const AkAudioCaps &caps)
{
    if (this->d->m_caps == caps)
        return;

    this->d->m_caps = caps;
    this->d->m_audioConvert.setOutputCaps(caps);
    emit this->capsChanged(caps);
}

void AudioDeviceElement::resetDevice()
{
    this->setDevice("");
}

void AudioDeviceElement::resetLatency()
{
    if (this->d->m_audioDevice)
        this->d->m_audioDevice->resetLatency();
}

void AudioDeviceElement::resetCaps()
{
    this->d->m_mutexLib.lock();
    auto preferredFormat = this->preferredFormat(this->d->m_device);
    this->d->m_mutexLib.unlock();

    this->setCaps(preferredFormat);
}

bool AudioDeviceElement::setState(AkElement::ElementState state)
{
    if (!this->d->m_audioDevice)
        return false;

    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused: {
            if (this->d->m_inputs.contains(this->d->m_device)) {
                this->d->m_audioConvert.reset();
                this->d->m_pause = true;
                this->d->m_readFramesLoop = true;
                this->d->m_readFramesLoopResult =
                        QtConcurrent::run(&this->d->m_threadPool,
                                          &AudioDeviceElementPrivate::readFramesLoop,
                                          this->d);
            }

            return AkElement::setState(state);
        }
        case AkElement::ElementStatePlaying: {
            if (this->d->m_inputs.contains(this->d->m_device)) {
                this->d->m_audioConvert.reset();
                this->d->m_pause = false;
                this->d->m_readFramesLoop = true;
                this->d->m_readFramesLoopResult =
                        QtConcurrent::run(&this->d->m_threadPool,
                                          &AudioDeviceElementPrivate::readFramesLoop,
                                          this->d);
            } else if (this->d->m_device != DUMMY_OUTPUT_DEVICE
                       && this->d->m_outputs.contains(this->d->m_device)) {
                this->d->m_audioConvert.reset();
                QString device = this->d->m_device;
                AkAudioCaps caps(this->d->m_caps);

                this->d->m_mutexLib.lock();
                auto isInit = this->d->m_audioDevice->init(device, caps);
                this->d->m_mutexLib.unlock();

                if (!isInit)
                    return false;
            }

            return AkElement::setState(state);
        }
        case AkElement::ElementStateNull:
            break;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull:
            if (this->d->m_inputs.contains(this->d->m_device)) {
                this->d->m_pause = false;
                this->d->m_readFramesLoop = false;
                this->d->m_readFramesLoopResult.waitForFinished();
            } else if (this->d->m_device != DUMMY_OUTPUT_DEVICE
                       && this->d->m_outputs.contains(this->d->m_device)) {
                this->d->m_mutexLib.lock();
                this->d->m_audioDevice->uninit();
                this->d->m_mutexLib.unlock();
            }

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            if (this->d->m_inputs.contains(this->d->m_device)) {
                this->d->m_pause = false;
            } else if (this->d->m_device != DUMMY_OUTPUT_DEVICE
                       && this->d->m_outputs.contains(this->d->m_device)) {
                QString device = this->d->m_device;
                AkAudioCaps caps(this->d->m_caps);

                this->d->m_mutexLib.lock();
                auto isInit = this->d->m_audioDevice->init(device, caps);
                this->d->m_mutexLib.unlock();

                if (!isInit)
                    return false;
            }

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull:
            if (this->d->m_inputs.contains(this->d->m_device)) {
                this->d->m_pause = false;
                this->d->m_readFramesLoop = false;
                this->d->m_readFramesLoopResult.waitForFinished();
            } else if (this->d->m_device != DUMMY_OUTPUT_DEVICE
                       && this->d->m_outputs.contains(this->d->m_device)) {
                this->d->m_mutexLib.lock();
                this->d->m_audioDevice->uninit();
                this->d->m_mutexLib.unlock();
            }

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            if (this->d->m_inputs.contains(this->d->m_device)) {
                this->d->m_pause = true;
            } else if (this->d->m_device != DUMMY_OUTPUT_DEVICE
                       && this->d->m_outputs.contains(this->d->m_device)) {
                this->d->m_mutexLib.lock();
                this->d->m_audioDevice->uninit();
                this->d->m_mutexLib.unlock();
            }

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
}

#include "moc_audiodeviceelement.cpp"
