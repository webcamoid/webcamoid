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
#define MAX_SAMPLE_RATE 512e3

using ObjectPtr = QSharedPointer<QObject>;

class AudioLayerPrivate
{
    public:
        AudioLayer *self;
        QQmlApplicationEngine *m_engine {nullptr};
        QStringList m_audioInput;
        QStringList m_inputs;
        QString m_input;
        QString m_inputDescription;
        AkAudioCaps m_inputCaps;
        AkAudioCaps m_outputCaps;
        AkElementPtr m_audioOut {akPluginManager->create<AkElement>("AudioSource/AudioDevice")};
        AkElementPtr m_audioIn {akPluginManager->create<AkElement>("AudioSource/AudioDevice")};
        AkElementPtr m_audioGenerator {akPluginManager->create<AkElement>("AudioSource/AudioGenerator")};
        QMutex m_mutex;
        QVector<int> m_commonSampleRates;
        AkElement::ElementState m_inputState {AkElement::ElementStateNull};

        explicit AudioLayerPrivate(AudioLayer *self);
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

    this->d->m_inputState = AkElement::ElementStateNull;
    this->setQmlEngine(engine);

    if (this->d->m_audioOut) {
        auto device = this->d->m_audioOut->property("defaultOutput").toString();
        this->d->m_audioOut->setProperty("device", device);

        QObject::connect(this->d->m_audioOut.data(),
                         SIGNAL(deviceChanged(QString)),
                         this,
                         SIGNAL(audioOutputChanged(QString)));
        QObject::connect(this->d->m_audioOut.data(),
                         SIGNAL(capsChanged(AkAudioCaps)),
                         this,
                         SIGNAL(outputDeviceCapsChanged(AkAudioCaps)));
        QObject::connect(this->d->m_audioOut.data(),
                         SIGNAL(outputsChanged(QStringList)),
                         this,
                         SIGNAL(outputsChanged(QStringList)));
        QObject::connect(this->d->m_audioOut.data(),
                         SIGNAL(stateChanged(AkElement::ElementState)),
                         this,
                         SIGNAL(outputStateChanged(AkElement::ElementState)));
        QObject::connect(this->d->m_audioOut.data(),
                         SIGNAL(latencyChanged(int)),
                         this,
                         SIGNAL(outputLatencyChanged(int)));
    }

    if (this->d->m_audioIn) {
        auto device = this->d->m_audioIn->property("defaultInput").toString();
        this->d->m_audioIn->setProperty("device", device);
        this->d->m_audioInput = QStringList {device};
        this->d->m_inputs = QStringList {DUMMY_INPUT_DEVICE}
                          + this->d->m_audioIn->property("inputs").toStringList();
        QObject::connect(this->d->m_audioIn.data(),
                         SIGNAL(oStream(AkPacket)),
                         this,
                         SLOT(sendPacket(AkPacket)),
                         Qt::DirectConnection);

        QObject::connect(this->d->m_audioIn.data(),
                         SIGNAL(inputsChanged(QStringList)),
                         this,
                         SLOT(privInputsChanged(QStringList)));
        QObject::connect(this->d->m_audioIn.data(),
                         SIGNAL(latencyChanged(int)),
                         this,
                         SIGNAL(inputLatencyChanged(int)));
    }

    this->d->m_outputCaps = this->outputCaps();

    if (this->d->m_audioGenerator) {
        this->d->m_audioGenerator->setProperty("waveType", "silence");
        QObject::connect(this->d->m_audioGenerator.data(),
                         SIGNAL(oStream(AkPacket)),
                         this,
                         SLOT(sendPacket(AkPacket)),
                         Qt::DirectConnection);
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
    if (this->d->m_audioOut)
        return this->d->m_audioOut->property("device").toString();

    return QString();
}

QStringList AudioLayer::inputs() const
{
    return this->d->m_inputs;
}

QStringList AudioLayer::outputs() const
{
    QStringList outputs;

    if (this->d->m_audioOut) {
        auto devices =
                this->d->m_audioOut->property("outputs").toStringList();

        if (devices.contains(":dummyout:"))
            outputs << ":dummyout:";

        for (auto &device: devices)
            if (device != ":dummyout:")
                outputs << device;
    }

    return outputs;
}

AkAudioCaps AudioLayer::outputCaps() const
{
    if (this->d->m_audioInput.contains(this->d->m_input)) {
        if (this->d->m_inputCaps)
            return this->d->m_inputCaps;

        if (this->d->m_audioGenerator)
            return this->d->m_audioGenerator->property("caps").value<AkAudioCaps>();

        return {};
    }

    if (this->d->m_audioInput.contains(DUMMY_INPUT_DEVICE)) {
        if (this->d->m_audioGenerator)
            return this->d->m_audioGenerator->property("caps").value<AkAudioCaps>();

        return {};
    }

    if (this->d->m_audioIn)
        return this->d->m_audioIn->property("caps").value<AkAudioCaps>();

    return {};
}

AkAudioCaps AudioLayer::inputDeviceCaps() const
{
    return this->outputCaps();
}

AkAudioCaps AudioLayer::outputDeviceCaps() const
{
    if (this->d->m_audioOut)
        return this->d->m_audioOut->property("caps").value<AkAudioCaps>();

    return {};
}

QString AudioLayer::description(const QString &device) const
{
    QString description;

    if (device == this->d->m_input)
        description = this->d->m_inputDescription;
    else if (device == DUMMY_INPUT_DEVICE || device == ":dummyout:")
        description = tr("Silence");
    else if (this->d->m_audioOut)
        QMetaObject::invokeMethod(this->d->m_audioOut.data(),
                                  "description",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, device));
    else if (this->d->m_audioIn)
        QMetaObject::invokeMethod(this->d->m_audioIn.data(),
                                  "description",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, device));

    return description;
}

AkElement::ElementState AudioLayer::inputState() const
{
    return this->d->m_inputState;
}

AkElement::ElementState AudioLayer::outputState() const
{
    if (this->d->m_audioOut)
        return this->d->m_audioOut->property("state").value<AkElement::ElementState>();

    return AkElement::ElementStateNull;
}

int AudioLayer::inputLatency() const
{
    if (this->d->m_audioIn)
        return this->d->m_audioIn->property("latency").toInt();

    return 1;
}

int AudioLayer::outputLatency() const
{
    if (this->d->m_audioOut)
        return this->d->m_audioOut->property("latency").toInt();

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

        if (this->d->m_audioGenerator)
            caps = this->d->m_audioGenerator->property("caps").value<AkAudioCaps>();

        return caps;
    }

    if (this->d->m_audioOut) {
        AkAudioCaps preferredFormat;
        QMetaObject::invokeMethod(this->d->m_audioOut.data(),
                                  "preferredFormat",
                                  Q_RETURN_ARG(AkAudioCaps, preferredFormat),
                                  Q_ARG(QString, device));

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

        if (this->d->m_inputCaps)
            caps = this->d->m_inputCaps;
        else if (this->d->m_audioGenerator)
            caps = this->d->m_audioGenerator->property("caps").value<AkAudioCaps>();

        if (caps)
            return {caps.format()};
    } else if (this->d->m_audioOut) {
        QList<AkAudioCaps::SampleFormat> supportedFormats;
        QMetaObject::invokeMethod(this->d->m_audioOut.data(),
                                  "supportedFormats",
                                  Q_RETURN_ARG(QList<AkAudioCaps::SampleFormat>,
                                               supportedFormats),
                                  Q_ARG(QString, device));

        return supportedFormats;
    }

    return {};
}

QList<AkAudioCaps::ChannelLayout> AudioLayer::supportedChannelLayouts(const QString &device) const
{
    if (device == DUMMY_INPUT_DEVICE)
        return {AkAudioCaps::Layout_mono, AkAudioCaps::Layout_stereo};

    if (device == this->d->m_input) {
        AkAudioCaps caps;

        if (this->d->m_inputCaps)
            caps = this->d->m_inputCaps;
        else if (this->d->m_audioGenerator)
            caps = this->d->m_audioGenerator->property("caps").value<AkAudioCaps>();

        if (caps)
            return {caps.layout()};
    } else if (this->d->m_audioOut) {
        QList<AkAudioCaps::ChannelLayout> supportedChannelLayouts;
        QMetaObject::invokeMethod(this->d->m_audioOut.data(),
                                  "supportedChannelLayouts",
                                  Q_RETURN_ARG(QList<AkAudioCaps::ChannelLayout>,
                                               supportedChannelLayouts),
                                  Q_ARG(QString, device));

        return supportedChannelLayouts;
    }

    return {};
}

QList<int> AudioLayer::supportedSampleRates(const QString &device) const
{
    if (device == DUMMY_INPUT_DEVICE)
        return this->d->m_commonSampleRates.toList();

    if (device == this->d->m_input) {
        AkAudioCaps caps;

        if (this->d->m_inputCaps)
            caps = this->d->m_inputCaps;
        else if (this->d->m_audioGenerator)
            caps = this->d->m_audioGenerator->property("caps").value<AkAudioCaps>();

        if (caps)
            return QList<int> {caps.rate()};
    } else if (this->d->m_audioOut) {
        QList<int> supportedSampleRates;
        QMetaObject::invokeMethod(this->d->m_audioOut.data(),
                                  "supportedSampleRates",
                                  Q_RETURN_ARG(QList<int>, supportedSampleRates),
                                  Q_ARG(QString, device));

        return supportedSampleRates;
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

void AudioLayer::setAudioInput(const QStringList &audioInput)
{
    if (this->d->m_audioInput == audioInput)
        return;

    auto state = this->inputState();
    this->setInputState(AkElement::ElementStateNull);

    if (this->d->m_audioIn
        && !audioInput.contains(this->d->m_input)
        && !audioInput.contains(DUMMY_INPUT_DEVICE))
        this->d->m_audioIn->setProperty("device", audioInput.value(0));

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

    auto state = this->d->m_audioOut->property("state");
    this->d->m_audioOut->setProperty("state", AkElement::ElementStateNull);
    this->d->m_audioOut->setProperty("device", audioOutput);
    this->d->m_audioOut->setProperty("state", state);

    if (this->d->m_audioOut->property("state") != state) {
        this->d->m_audioOut->setProperty("state", AkElement::ElementStateNull);
        this->d->m_audioOut->setProperty("device", ":dummyout:");
        this->d->m_audioOut->setProperty("state", state);
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
            this->d->m_inputs =
                    QStringList {DUMMY_INPUT_DEVICE}
                    + this->d->m_audioIn->property("inputs").toStringList();
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
    this->d->m_inputs =
            QStringList {DUMMY_INPUT_DEVICE, device}
            + this->d->m_audioIn->property("inputs").toStringList();
    emit this->inputsChanged(this->d->m_inputs);
}

void AudioLayer::setInputDeviceCaps(const AkAudioCaps &inputDeviceCaps)
{
    if (this->d->m_audioInput.contains(this->d->m_input)
        || this->inputDeviceCaps() == inputDeviceCaps)
        return;

    if (this->d->m_audioInput.contains(DUMMY_INPUT_DEVICE)) {
        if (this->d->m_audioGenerator)
            this->d->m_audioGenerator->setProperty("caps", QVariant::fromValue(inputDeviceCaps));
    } else if (this->d->m_audioIn) {
        this->d->m_audioIn->setProperty("caps", QVariant::fromValue(inputDeviceCaps));
    }

    emit inputDeviceCapsChanged(inputDeviceCaps);
}

void AudioLayer::setOutputDeviceCaps(const AkAudioCaps &outputDeviceCaps)
{
    if (this->d->m_audioOut)
        this->d->m_audioOut->setProperty("caps",
                                         QVariant::fromValue(outputDeviceCaps));
}

void AudioLayer::setInputState(AkElement::ElementState inputState)
{
    if (this->d->m_inputState == inputState)
        return;

    if (this->d->m_audioInput.contains(this->d->m_input)) {
        if (this->d->m_inputCaps) {
            if (this->d->m_audioIn)
                this->d->m_audioIn->setState(AkElement::ElementStateNull);

            if (this->d->m_audioGenerator)
                this->d->m_audioGenerator->setState(AkElement::ElementStateNull);
        } else {
            if (this->d->m_audioIn)
                this->d->m_audioIn->setState(AkElement::ElementStateNull);

            if (this->d->m_audioGenerator)
                this->d->m_audioGenerator->setState(inputState);
        }
    } else if (this->d->m_audioInput.contains(DUMMY_INPUT_DEVICE)) {
        if (this->d->m_audioIn)
            this->d->m_audioIn->setState(AkElement::ElementStateNull);

        if (this->d->m_audioGenerator)
            this->d->m_audioGenerator->setState(inputState);
    } else {
        if (this->d->m_audioGenerator)
            this->d->m_audioGenerator->setState(AkElement::ElementStateNull);

        if (this->d->m_audioIn)
            this->d->m_audioIn->setState(inputState);
    }

    this->d->m_inputState = inputState;
    emit this->inputStateChanged(inputState);
}

bool AudioLayer::setOutputState(AkElement::ElementState outputState)
{
    if (this->d->m_audioOut)
        return this->d->m_audioOut->setProperty("state", outputState);

    return false;
}

void AudioLayer::setInputLatency(int inputLatency)
{
    if (this->d->m_audioIn)
        this->d->m_audioIn->setProperty("latency", inputLatency);
}

void AudioLayer::setOutputLatency(int outputLatency)
{
    if (this->d->m_audioOut)
        this->d->m_audioOut->setProperty("latency", outputLatency);
}

void AudioLayer::resetAudioInput()
{
    QStringList devices;

    if (this->d->m_audioIn)
        devices << this->d->m_audioIn->property("defaultInput").toString();

    this->setAudioInput(devices);
}

void AudioLayer::resetAudioOutput()
{
    QString device;

    if (this->d->m_audioOut)
        device = this->d->m_audioOut->property("defaultOutput").toString();

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
        if (this->d->m_audioGenerator)
            QMetaObject::invokeMethod(this->d->m_audioGenerator.data(),
                                      "resetCaps");
    } else if (this->d->m_audioIn) {
        QMetaObject::invokeMethod(this->d->m_audioIn.data(), "resetCaps");
    }
}

void AudioLayer::resetOutputDeviceCaps()
{
    if (this->d->m_audioOut)
        QMetaObject::invokeMethod(this->d->m_audioOut.data(), "resetCaps");
}

void AudioLayer::resetInputState()
{
    this->setInputState(AkElement::ElementStateNull);
}

void AudioLayer::resetOutputState()
{
    this->setOutputState(AkElement::ElementStateNull);
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

void AudioLayer::privInputsChanged(const QStringList &inputs)
{
    QStringList ins {DUMMY_INPUT_DEVICE};

    if (!this->d->m_input.isEmpty())
        ins << this->d->m_input;

    ins += inputs;

    if (this->d->m_inputs == ins)
        return;

    this->d->m_inputs = ins;
    emit this->inputsChanged(ins);

    if (!this->d->m_audioInput.isEmpty()
        && !this->d->m_inputs.contains(this->d->m_audioInput.value(0))) {
            QString device =
                    this->d->m_audioIn?
                        this->d->m_audioIn->property("defaultInput").toString():
                        QString();
            this->setAudioInput({device});
        }
}

AudioLayerPrivate::AudioLayerPrivate(AudioLayer *self):
    self(self)
{

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
