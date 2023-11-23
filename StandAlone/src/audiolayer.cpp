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

#include <QMutex>
#include <QFile>
#include <QThread>
#include <QAbstractEventDispatcher>
#include <QSettings>
#include <QQuickItem>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQmlApplicationEngine>
#include <akcaps.h>
#include <akpacket.h>
#include <akplugin.h>
#include <akpluginmanager.h>

#include "audiolayer.h"

#define DUMMY_INPUT_DEVICE ":dummyin:"
#define DUMMY_OUtPUT_DEVICE ":dummyout:"
#define MAX_SAMPLE_RATE 512e3

class AudioLayerPrivate;

class OutputDeviceCapsChanged:
    public IAkStreamFormatCallbacks
{
    public:
        OutputDeviceCapsChanged(AudioLayerPrivate *self);
        void formatChanged(const AkCaps &caps);

    private:
        AudioLayerPrivate *self;
};

class OutputStateChanged:
    public IAkElementStateCallbacks
{
    public:
        OutputStateChanged(AudioLayerPrivate *self);
        void stateChanged(AkElementState state);

    private:
        AudioLayerPrivate *self;
};

class AudioLayerPrivate:
    public IAkDeviceProviderCallbacks
{
    public:
        AudioLayer *self;
        QQmlApplicationEngine *m_engine {nullptr};
        QStringList m_audioInput;
        QString m_audioOutput;
        QStringList m_inputs;
        QStringList m_outputs;
        QString m_input;
        QString m_inputDescription;
        AkAudioCaps m_inputCaps;
        AkAudioCaps m_outputCaps;
        IAkAudioDeviceProviderPtr m_audioDeviceProvider {akPluginManager->create<IAkAudioDeviceProvider>("AudioSource/AudioDevice")};
        IAkAudioDeviceSinkPtr m_audioOut;
        IAkAudioDeviceSourcePtr m_audioIn;
        IAkAudioDeviceSourcePtr m_audioGenerator {akPluginManager->create<IAkAudioDeviceSource>("AudioSource/AudioGenerator")};
        QMutex m_mutex;
        QVector<int> m_commonSampleRates;
        AkElementState m_inputState {AkElementState_Stopped};
        OutputDeviceCapsChanged *m_outputDeviceCapsChanged {nullptr};
        OutputStateChanged *m_outputStateChanged {nullptr};

        explicit AudioLayerPrivate(AudioLayer *self);
        ~AudioLayerPrivate();
        void devicesUpdated(const QString &devices={}) override;
        void loadProperties();
        void saveAudioInput(const QStringList &audioInput);
        void saveAudioOutput(const QString &audioOutput);
};

AudioLayer::AudioLayer(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new AudioLayerPrivate(this);

    // Multiples of 8k sample rates
    for (int rate = 4000; rate < MAX_SAMPLE_RATE; rate *= 2)
        this->d->m_commonSampleRates << rate;

    // Multiples of 48k sample rates
    for (int rate = 6000; rate < MAX_SAMPLE_RATE; rate *= 2)
        this->d->m_commonSampleRates << rate;

    // Multiples of 44.1k sample rates
    for (int rate = 11025; rate < MAX_SAMPLE_RATE; rate *= 2)
        this->d->m_commonSampleRates << rate;

    std::sort(this->d->m_commonSampleRates.begin(),
              this->d->m_commonSampleRates.end());

    this->d->m_inputState = AkElementState_Stopped;
    this->setQmlEngine(engine);

    if (this->d->m_audioDeviceProvider)
        this->d->m_audioDeviceProvider->subscribe(this->d);

    this->d->devicesUpdated();
    this->d->m_outputCaps = this->outputCaps();

    if (this->d->m_audioGenerator) {
        auto menu = this->d->m_audioGenerator->property<IAkPropertyIntMenu>("waveType");
        menu->setValue(menu->optionById("silence").value());
        this->d->m_audioGenerator->link(this);
    }

    this->d->loadProperties();
}

AudioLayer::~AudioLayer()
{
    this->resetInputState();
    this->resetOutputState();

    this->d->m_mutex.lock();
    this->d->m_audioOut.clear();
    this->d->m_mutex.unlock();

    delete this->d;
}

QStringList AudioLayer::audioInput() const
{
    return this->d->m_audioInput;
}

QString AudioLayer::audioOutput() const
{
    return this->d->m_audioOutput;
}

QStringList AudioLayer::inputs() const
{
    return this->d->m_inputs;
}

QStringList AudioLayer::outputs() const
{
    QStringList outputs;

    if (this->d->m_audioDeviceProvider) {
        outputs = this->d->m_audioDeviceProvider->sinks();

        if (outputs.contains(DUMMY_OUtPUT_DEVICE))
            outputs << DUMMY_OUtPUT_DEVICE;

        for (auto &device: outputs)
            if (device != DUMMY_OUtPUT_DEVICE)
                outputs << device;
    }

    return outputs;
}

AkAudioCaps AudioLayer::outputCaps() const
{
    if (this->d->m_audioInput.contains(this->d->m_input)) {
        if (this->d->m_inputCaps)
            return this->d->m_inputCaps;

        if (this->d->m_audioGenerator) {
            auto streamFormat = this->d->m_audioGenerator->sourceStreamFormat();

            if (streamFormat)
                return streamFormat->format();

            return {};
        }

        return {};
    }

    if (this->d->m_audioInput.contains(DUMMY_INPUT_DEVICE)) {
        if (this->d->m_audioGenerator){
            auto streamFormat = this->d->m_audioGenerator->sourceStreamFormat();

            if (streamFormat)
                return streamFormat->format();

            return {};
        }

        return {};
    }

    if (this->d->m_audioIn) {
        auto streamFormat = this->d->m_audioIn->sourceStreamFormat();

        if (streamFormat)
            return streamFormat->format();

        return {};
    }

    return {};
}

AkAudioCaps AudioLayer::inputDeviceCaps() const
{
    return this->outputCaps();
}

AkAudioCaps AudioLayer::outputDeviceCaps() const
{
    if (this->d->m_audioOut) {
        auto streamFormat = this->d->m_audioOut->sinkStreamFormat();

        if (streamFormat)
            return streamFormat->format();

        return {};
    }

    return {};
}

QString AudioLayer::description(const QString &device) const
{
    if (device == this->d->m_input)
        return this->d->m_inputDescription;
    else if (device == DUMMY_INPUT_DEVICE || device == DUMMY_OUtPUT_DEVICE)
        return tr("Silence");
    else if (this->d->m_audioOut)
        return this->d->m_audioOut->description();
    else if (this->d->m_audioIn)
        return this->d->m_audioIn->description();

    return {};
}

AkElementState AudioLayer::inputState() const
{
    return this->d->m_inputState;
}

AkElementState AudioLayer::outputState() const
{
    if (this->d->m_audioOut)
        return this->d->m_audioOut->state();

    return AkElementState_Stopped;
}

int AudioLayer::inputLatency() const
{
    if (this->d->m_audioIn)
        return this->d->m_audioIn->latency();

    return 1;
}

int AudioLayer::outputLatency() const
{
    if (this->d->m_audioOut)
        return this->d->m_audioOut->latency();

    return 1;
}

AkAudioCaps AudioLayer::preferredFormat(const QString &device)
{
    if (device == DUMMY_INPUT_DEVICE)
        return AkAudioCaps(AkAudioCaps::SampleFormat_s16,
                           AkAudioCaps::Layout_mono,
                           false,
                           8000);

    if (device == this->d->m_input) {
        if (this->d->m_inputCaps)
            return this->d->m_inputCaps;

        AkAudioCaps caps;

        if (this->d->m_audioGenerator) {
            auto streamFormat = this->d->m_audioGenerator->sourceStreamFormat();

            if (streamFormat)
                caps = streamFormat->format();
        }

        return caps;
    }

    if (this->d->m_audioOut) {
        AkAudioCaps preferredFormat;
        auto streamFormat = this->d->m_audioOut->sinkStreamFormat();

        if (streamFormat)
            preferredFormat = streamFormat->preferredFormat();

        return preferredFormat;
    }

    return AkAudioCaps();
}

QList<AkAudioCaps::SampleFormat> AudioLayer::supportedFormats(const QString &device) const
{
    if (device == DUMMY_INPUT_DEVICE)
        return {
            AkAudioCaps::SampleFormat_s32,
            AkAudioCaps::SampleFormat_s16,
            AkAudioCaps::SampleFormat_flt,
            AkAudioCaps::SampleFormat_u8,
        };

    if (device == this->d->m_input) {
        AkAudioCaps caps;

        if (this->d->m_inputCaps) {
            caps = this->d->m_inputCaps;
        } else if (this->d->m_audioGenerator) {
            auto streamFormat = this->d->m_audioGenerator->sourceStreamFormat();

            if (streamFormat)
                caps = streamFormat->format();
        }

        if (caps)
            return {caps.format()};
    } else if (this->d->m_audioOut) {
        return this->d->m_audioOut->supportedFormats();
    }

    return {};
}

QList<AkAudioCaps::ChannelLayout> AudioLayer::supportedChannelLayouts(const QString &device) const
{
    if (device == DUMMY_INPUT_DEVICE)
        return {AkAudioCaps::Layout_mono, AkAudioCaps::Layout_stereo};

    if (device == this->d->m_input) {
        AkAudioCaps caps;

        if (this->d->m_inputCaps) {
            caps = this->d->m_inputCaps;
        } else if (this->d->m_audioGenerator) {
            auto streamFormat = this->d->m_audioGenerator->sourceStreamFormat();

            if (streamFormat)
                caps = streamFormat->format();
        }

        if (caps)
            return {caps.layout()};
    } else if (this->d->m_audioOut) {
        return this->d->m_audioOut->supportedChannelLayouts();
    }

    return {};
}

QList<int> AudioLayer::supportedSampleRates(const QString &device) const
{
    if (device == DUMMY_INPUT_DEVICE)
        return this->d->m_commonSampleRates.toList();

    if (device == this->d->m_input) {
        AkAudioCaps caps;

        if (this->d->m_inputCaps) {
            caps = this->d->m_inputCaps;
        } else if (this->d->m_audioGenerator) {
            auto streamFormat = this->d->m_audioGenerator->sourceStreamFormat();

            if (streamFormat)
                caps = streamFormat->format();
        }

        if (caps)
            return QList<int> {caps.rate()};
    } else if (this->d->m_audioOut) {
        return this->d->m_audioOut->supportedSampleRates();
    }

    return {};
}

QVariantList AudioLayer::supportedFormatsVariant(const QString &device) const
{
    QVariantList list;

    for (auto &format: this->supportedFormats(device))
        list << format;

    return list;
}

QVariantList AudioLayer::supportedChannelLayoutsVariant(const QString &device) const
{
    QVariantList list;

    for (auto &format: this->supportedChannelLayouts(device))
        list << format;

    return list;
}

AkPacket AudioLayer::iAudioStream(const AkAudioPacket &packet)
{
    emit this->sendPacket(packet);

    return packet;
}

void AudioLayer::setAudioInput(const QStringList &audioInput)
{
    if (this->d->m_audioInput == audioInput)
        return;

    auto state = this->inputState();
    this->setInputState(AkElementState_Stopped);

    if (this->d->m_audioDeviceProvider
        && !audioInput.contains(this->d->m_input)
        && !audioInput.contains(DUMMY_INPUT_DEVICE)) {
        this->d->m_audioIn =
            this->d->m_audioDeviceProvider->create<IAkAudioDeviceSource>(audioInput.value(0));
        this->d->m_audioIn->link(this);
    }

    this->d->m_mutex.lock();
    this->d->m_audioInput = audioInput;
    this->d->m_mutex.unlock();

    emit this->audioInputChanged(audioInput);
    this->d->saveAudioInput(audioInput);
    auto outputCaps = this->outputCaps();

    if (this->d->m_outputCaps != outputCaps) {
        this->d->m_outputCaps = outputCaps;
        emit this->outputCapsChanged(outputCaps);
    }

    this->setInputState(state);
}

void AudioLayer::setAudioOutput(const QString &audioOutput)
{
    if (!this->d->m_audioOut)
        return;

    while (!this->d->m_mutex.tryLock()) {
        auto eventDispatcher = QThread::currentThread()->eventDispatcher();

        if (eventDispatcher)
            eventDispatcher->processEvents(QEventLoop::AllEvents);
    }

    auto state = this->outputState();
    this->setOutputState(AkElementState_Stopped);
    this->d->m_audioOut =
        this->d->m_audioDeviceProvider->create<IAkAudioDeviceSink>(audioOutput);

    if (this->d->m_audioOut) {
        auto streamFormat = this->d->m_audioOut->sinkStreamFormat();

        if (streamFormat)
            streamFormat->subscribe(this->d->m_outputDeviceCapsChanged);

        this->d->m_audioOut->IAkElementState::subscribe(this->d->m_outputStateChanged);
    }

    this->setOutputState(state);

    if (this->outputState() != state) {
        this->setOutputState(AkElementState_Stopped);
        this->d->m_audioOut =
            this->d->m_audioDeviceProvider->create<IAkAudioDeviceSink>(DUMMY_OUtPUT_DEVICE);

        if (this->d->m_audioOut) {
            auto streamFormat = this->d->m_audioOut->sinkStreamFormat();

            if (streamFormat)
                streamFormat->subscribe(this->d->m_outputDeviceCapsChanged);

            this->d->m_audioOut->IAkElementState::subscribe(this->d->m_outputStateChanged);
        }

        this->setOutputState(state);
    }

    this->d->m_mutex.unlock();

    this->d->saveAudioOutput(audioOutput);
    auto outputCaps = this->outputCaps();

    if (this->d->m_outputCaps != outputCaps) {
        this->d->m_outputCaps = outputCaps;
        emit this->outputCapsChanged(outputCaps);
    }
}

void AudioLayer::setInput(const QString &device,
                          const QString &description,
                          const AkAudioCaps &inputCaps)
{
    if (device.isEmpty() || description.isEmpty() || !inputCaps) {
        this->d->m_inputDescription.clear();
        this->d->m_inputCaps = AkAudioCaps();

        if (!this->d->m_input.isEmpty()) {
            this->d->m_input.clear();
            this->d->m_inputs = QStringList {DUMMY_INPUT_DEVICE};

            if (this->d->m_audioDeviceProvider)
                this->d->m_inputs << this->d->m_audioDeviceProvider->sources();

            emit this->inputsChanged(this->d->m_inputs);
        }

        return;
    }

    if (this->d->m_input == device
        && this->d->m_inputDescription == description
        && this->d->m_inputCaps == inputCaps) {
        return;
    }

    this->d->m_input = device;
    this->d->m_inputDescription = description;
    this->d->m_inputCaps = inputCaps;
    this->d->m_inputs = QStringList {DUMMY_INPUT_DEVICE, device};

    if (this->d->m_audioDeviceProvider)
        this->d->m_inputs << this->d->m_audioDeviceProvider->sources();

    emit this->inputsChanged(this->d->m_inputs);
}

void AudioLayer::setInputDeviceCaps(const AkAudioCaps &inputDeviceCaps)
{
    if (this->d->m_audioInput.contains(this->d->m_input)
        || this->inputDeviceCaps() == inputDeviceCaps)
        return;

    if (this->d->m_audioInput.contains(DUMMY_INPUT_DEVICE)) {
        if (this->d->m_audioGenerator) {
            auto streamFormat = this->d->m_audioGenerator->sourceStreamFormat();

            if (streamFormat)
                streamFormat->setFormat(inputDeviceCaps);
        }
    } else if (this->d->m_audioIn) {
        auto streamFormat = this->d->m_audioIn->sourceStreamFormat();

        if (streamFormat)
            streamFormat->setFormat(inputDeviceCaps);
    }

    emit this->inputDeviceCapsChanged(inputDeviceCaps);
}

void AudioLayer::setOutputDeviceCaps(const AkAudioCaps &outputDeviceCaps)
{
    if (this->d->m_audioOut) {
        auto streamFormat = this->d->m_audioOut->sinkStreamFormat();

        if (streamFormat)
            streamFormat->setFormat(outputDeviceCaps);
    }
}

void AudioLayer::setInputState(AkElementState inputState)
{
    if (this->d->m_inputState == inputState)
        return;

    if (this->d->m_audioInput.contains(this->d->m_input)) {
        if (this->d->m_inputCaps) {
            if (this->d->m_audioIn)
                this->d->m_audioIn->setState(AkElementState_Stopped);

            if (this->d->m_audioGenerator)
                this->d->m_audioGenerator->setState(AkElementState_Stopped);
        } else {
            if (this->d->m_audioIn)
                this->d->m_audioIn->setState(AkElementState_Stopped);

            if (this->d->m_audioGenerator)
                this->d->m_audioGenerator->setState(inputState);
        }
    } else if (this->d->m_audioInput.contains(DUMMY_INPUT_DEVICE)) {
        if (this->d->m_audioIn)
            this->d->m_audioIn->setState(AkElementState_Stopped);

        if (this->d->m_audioGenerator)
            this->d->m_audioGenerator->setState(inputState);
    } else {
        if (this->d->m_audioGenerator)
            this->d->m_audioGenerator->setState(AkElementState_Stopped);

        if (this->d->m_audioIn)
            this->d->m_audioIn->setState(inputState);
    }

    this->d->m_inputState = inputState;
    emit this->inputStateChanged(inputState);
}

bool AudioLayer::setOutputState(AkElementState outputState)
{
    if (this->d->m_audioOut)
        this->d->m_audioOut->setState(outputState);

    return false;
}

void AudioLayer::setInputLatency(int inputLatency)
{
    if (this->d->m_audioIn) {
        auto latency = this->d->m_audioIn->latency();
        this->d->m_audioIn->setLatency(inputLatency);

        if (latency != inputLatency)
            emit this->inputLatencyChanged(inputLatency);
    }
}

void AudioLayer::setOutputLatency(int outputLatency)
{
    if (this->d->m_audioOut) {
        auto latency = this->d->m_audioIn->latency();
        this->d->m_audioOut->setLatency(outputLatency);

        if (latency != outputLatency)
            emit this->outputLatencyChanged(outputLatency);
    }
}

void AudioLayer::resetAudioInput()
{
    QStringList devices;

    if (this->d->m_audioDeviceProvider)
        devices <<
            this->d->m_audioDeviceProvider->defaultDevice(AkDeviceType_Source);

    this->setAudioInput(devices);
}

void AudioLayer::resetAudioOutput()
{
    QString device;

    if (this->d->m_audioDeviceProvider)
        device =
            this->d->m_audioDeviceProvider->defaultDevice(AkDeviceType_Sink);

    this->setAudioOutput(device);
}

void AudioLayer::resetInput()
{
    this->setInput({}, {}, {});
}

void AudioLayer::resetInputDeviceCaps()
{
    if (this->d->m_audioInput.contains(this->d->m_input)) {
    } else if (this->d->m_audioInput.contains(DUMMY_INPUT_DEVICE)) {
        if (this->d->m_audioGenerator) {
            auto streamFormat = this->d->m_audioGenerator->sourceStreamFormat();

            if (streamFormat) {
                auto emitSignal = streamFormat->format() != streamFormat->preferredFormat();
                streamFormat->resetFormat();

                if (emitSignal)
                    emit this->inputDeviceCapsChanged(streamFormat->format());
            }
        }
    } else if (this->d->m_audioIn) {
        auto streamFormat = this->d->m_audioIn->sourceStreamFormat();

        if (streamFormat) {
            auto emitSignal = streamFormat->format() != streamFormat->preferredFormat();
            streamFormat->resetFormat();

            if (emitSignal)
                emit this->inputDeviceCapsChanged(streamFormat->format());
        }
    }
}

void AudioLayer::resetOutputDeviceCaps()
{
    if (this->d->m_audioOut) {
        auto streamFormat = this->d->m_audioOut->sinkStreamFormat();

        if (streamFormat)
            streamFormat->resetFormat();
    }
}

void AudioLayer::resetInputState()
{
    this->setInputState(AkElementState_Stopped);
}

void AudioLayer::resetOutputState()
{
    this->setOutputState(AkElementState_Stopped);
}

void AudioLayer::resetInputLatency()
{
    this->setInputLatency(1);
}

void AudioLayer::resetOutputLatency()
{
    this->setOutputLatency(1);
}

AkPacket AudioLayer::iStream(const AkPacket &packet)
{
    if (packet.caps().type() != AkCaps::CapsAudio)
        return {};

    this->d->m_mutex.lock();

    if (this->d->m_audioOut)
        this->d->m_audioOut->iStream(packet);

    if (this->d->m_audioInput.contains(this->d->m_input))
        this->sendPacket(packet);

    this->d->m_mutex.unlock();

    return {};
}

void AudioLayer::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->d->m_engine == engine)
        return;

    this->d->m_engine = engine;

    if (engine)
        engine->rootContext()->setContextProperty("audioLayer", this);
}

void AudioLayer::sendPacket(const AkPacket &packet)
{
    auto _packet = packet;
    _packet.setIndex(1);
    emit this->oStream(_packet);
}

OutputDeviceCapsChanged::OutputDeviceCapsChanged(AudioLayerPrivate *self):
      self(self)
{

}

void OutputDeviceCapsChanged::formatChanged(const AkCaps &caps)
{
    emit self->self->outputDeviceCapsChanged(caps);
}

OutputStateChanged::OutputStateChanged(AudioLayerPrivate *self):
      self(self)
{

}

void OutputStateChanged::stateChanged(AkElementState state)
{
    emit self->self->outputStateChanged(state);
}

AudioLayerPrivate::AudioLayerPrivate(AudioLayer *self):
    self(self)
{
    this->m_outputDeviceCapsChanged = new OutputDeviceCapsChanged(this);
    this->m_outputStateChanged = new OutputStateChanged(this);
}

AudioLayerPrivate::~AudioLayerPrivate()
{
    delete this->m_outputDeviceCapsChanged;
    delete this->m_outputStateChanged;
}

void AudioLayerPrivate::devicesUpdated(const QString &devices)
{
    Q_UNUSED(devices)

    QStringList inputs {DUMMY_INPUT_DEVICE};

    if (!this->m_input.isEmpty())
        inputs << this->m_input;

    if (this->m_audioDeviceProvider)
        inputs << this->m_audioDeviceProvider->sources();

    if (inputs != this->m_inputs) {
        this->m_inputs = inputs;
        emit self->inputsChanged(inputs);
        QString device;

        if (this->m_inputs.contains(this->m_audioInput.value(0)))
            device = this->m_audioInput.value(0);

        if (device.isEmpty() && this->m_audioDeviceProvider)
            device =
                this->m_audioDeviceProvider->defaultDevice(AkDeviceType_Source);

        if (device.isEmpty())
            self->setAudioInput({DUMMY_INPUT_DEVICE});
        else
            self->setAudioInput({device});
    }

    QStringList outputs;
    QStringList outputDevices;

    if (this->m_audioDeviceProvider) {
        outputDevices = this->m_audioDeviceProvider->sinks();

        if (outputDevices.contains(DUMMY_OUtPUT_DEVICE))
            outputs << DUMMY_OUtPUT_DEVICE;

        for (auto &device: outputDevices)
            if (device != DUMMY_OUtPUT_DEVICE)
                outputs << device;
    }

    if (outputs != this->m_outputs) {
        this->m_outputs = outputs;
        emit self->outputsChanged(outputs);
        QString device;

        if (this->m_outputs.contains(this->m_audioOutput))
            device = this->m_audioOutput;

        if (device.isEmpty() && this->m_audioDeviceProvider)
            device =
                this->m_audioDeviceProvider->defaultDevice(AkDeviceType_Sink);

        if (device.isEmpty()) {
            if (outputDevices.contains(DUMMY_OUtPUT_DEVICE))
                self->setAudioOutput(DUMMY_INPUT_DEVICE);
            else
                self->setAudioOutput({});
        } else {
            self->setAudioOutput(device);
        }
    }
}

void AudioLayerPrivate::loadProperties()
{
    QSettings config;

    config.beginGroup("AudioConfigs");
    QString confInput = config.value("audioInput").toString();

    if (self->inputs().contains(confInput))
        self->setAudioInput({confInput});

    QString confOutput = config.value("audioOutput").toString();

    if (self->outputs().contains(confOutput))
        self->setAudioOutput(confOutput);

    config.endGroup();
}

void AudioLayerPrivate::saveAudioInput(const QStringList &audioInput)
{
    QSettings config;
    config.beginGroup("AudioConfigs");
    config.setValue("audioInput", audioInput);
    config.endGroup();
}

void AudioLayerPrivate::saveAudioOutput(const QString &audioOutput)
{
    QSettings config;
    config.beginGroup("AudioConfigs");
    config.setValue("audioOutput", audioOutput);
    config.endGroup();
}

#include "moc_audiolayer.cpp"
