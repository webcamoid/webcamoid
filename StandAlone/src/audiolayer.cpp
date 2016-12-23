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

#include <QFile>
#include <QThread>
#include <QAbstractEventDispatcher>
#include <QSettings>
#include <QQuickItem>
#include <QQmlProperty>

#include "audiolayer.h"

#define DUMMY_INPUT_DEVICE ":dummyin:"
#define EXTERNAL_MEDIA_INPUT ":externalinput:"

AudioLayer::AudioLayer(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent),
    m_engine(nullptr)
{
    this->m_inputState = AkElement::ElementStateNull;
    this->setQmlEngine(engine);
    this->m_pipeline = AkElement::create("Bin", "pipeline");

    if (this->m_pipeline) {
        QFile jsonFile(":/Webcamoid/share/audiopipeline.json");
        jsonFile.open(QFile::ReadOnly);
        QString description(jsonFile.readAll());
        jsonFile.close();

        this->m_pipeline->setProperty("description", description);

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_audioOut),
                                  Q_ARG(QString, "audioOut"));
        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_audioIn),
                                  Q_ARG(QString, "audioIn"));
        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_audioGenerator),
                                  Q_ARG(QString, "audioGenerator"));
        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_audioSwitch),
                                  Q_ARG(QString, "audioSwitch"));
    }

    if (this->m_audioOut) {
        QString device = this->m_audioOut->property("defaultOutput").toString();
        this->m_audioOut->setProperty("device", device);
        this->m_outputDeviceCaps = this->m_audioOut->property("caps").value<AkCaps>();

        QObject::connect(this->m_audioOut.data(),
                         SIGNAL(deviceChanged(const QString &)),
                         this,
                         SIGNAL(audioOutputChanged(const QString &)));
        QObject::connect(this->m_audioOut.data(),
                         SIGNAL(capsChanged(const AkCaps &)),
                         this,
                         SLOT(setOutputDeviceCaps(const AkCaps &)));
        QObject::connect(this->m_audioOut.data(),
                         SIGNAL(outputsChanged(const QStringList &)),
                         this,
                         SIGNAL(outputsChanged(const QStringList &)));
        QObject::connect(this->m_audioOut.data(),
                         SIGNAL(stateChanged(AkElement::ElementState)),
                         this,
                         SIGNAL(outputStateChanged(AkElement::ElementState)));
    }

    if (this->m_audioIn) {
        QString device = this->m_audioIn->property("defaultInput").toString();
        this->m_audioIn->setProperty("device", device);
        this->m_audioInput = QStringList {device};
        this->m_inputs = QStringList {DUMMY_INPUT_DEVICE, EXTERNAL_MEDIA_INPUT}
                       + this->m_audioIn->property("inputs").toStringList();

        QObject::connect(this->m_audioIn.data(),
                         SIGNAL(inputsChanged(const QStringList &)),
                         this,
                         SLOT(privInputsChanged(const QStringList &)));
    }

    this->m_outputCaps = this->outputCaps();

    if (this->m_audioSwitch) {
        QObject::connect(this->m_audioSwitch.data(),
                         SIGNAL(oStream(const AkPacket &)),
                         this,
                         SIGNAL(oStream(const AkPacket &)),
                         Qt::DirectConnection);
    }

    QObject::connect(this,
                     &AudioLayer::audioInputChanged,
                     this,
                     &AudioLayer::updateInputState);
    QObject::connect(this,
                     &AudioLayer::inputCapsChanged,
                     this,
                     &AudioLayer::updateInputState);
    QObject::connect(this,
                     &AudioLayer::audioOutputChanged,
                     this,
                     &AudioLayer::updateOutputState);
    QObject::connect(this,
                     &AudioLayer::audioInputChanged,
                     this,
                     &AudioLayer::saveAudioInput);
    QObject::connect(this,
                     &AudioLayer::audioOutputChanged,
                     this,
                     &AudioLayer::saveAudioOutput);

    this->loadProperties();
}

AudioLayer::~AudioLayer()
{
    this->resetInputState();
    this->resetOutputState();
    this->saveProperties();

    this->m_mutex.lock();
    this->m_audioOut.clear();
    this->m_audioSwitch.clear();
    this->m_mutex.unlock();
}

QStringList AudioLayer::audioInput() const
{
    return this->m_audioInput;
}

QString AudioLayer::audioOutput() const
{
    if (this->m_audioOut)
        return this->m_audioOut->property("device").toString();

    return QString();
}

QStringList AudioLayer::inputs() const
{
    return this->m_inputs;
}

QStringList AudioLayer::outputs() const
{
    if (this->m_audioOut)
        return this->m_audioOut->property("outputs").toStringList();

    return QStringList();
}

AkCaps AudioLayer::inputCaps() const
{
    return this->m_inputCaps;
}

AkCaps AudioLayer::outputCaps() const
{
    if (this->m_audioInput.contains(EXTERNAL_MEDIA_INPUT)) {
        if (this->m_inputCaps)
            return this->m_inputCaps;
        else {
            AkCaps caps;

            if (this->m_audioGenerator)
                caps = this->m_audioGenerator->property("caps").toString();

            return caps;
        }
    } else if (this->m_audioInput.contains(DUMMY_INPUT_DEVICE)) {
        AkCaps caps;

        if (this->m_audioGenerator)
            caps = this->m_audioGenerator->property("caps").toString();

        return caps;
    }

    return this->m_audioIn?
                this->m_audioIn->property("caps").value<AkCaps>():
                AkCaps();
}

AkCaps AudioLayer::outputDeviceCaps() const
{
    return this->m_outputDeviceCaps;
}

QString AudioLayer::inputDescription() const
{
    return this->m_inputDescription;
}

QString AudioLayer::description(const QString &device) const
{
    QString description;

    if (device == EXTERNAL_MEDIA_INPUT)
        description = this->m_inputDescription;
    else if (device == DUMMY_INPUT_DEVICE)
        description = QString("Dummy Input");
    else if (this->m_audioOut)
        QMetaObject::invokeMethod(this->m_audioOut.data(),
                                  "description",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, device));
    else if (this->m_audioIn)
        QMetaObject::invokeMethod(this->m_audioIn.data(),
                                  "description",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, device));

    return description;
}

AkElement::ElementState AudioLayer::inputState() const
{
    return this->m_inputState;
}

AkElement::ElementState AudioLayer::outputState() const
{
    if (this->m_audioOut)
        return this->m_audioOut->property("state").value<AkElement::ElementState>();

    return AkElement::ElementStateNull;
}

void AudioLayer::setAudioInput(const QStringList &audioInput)
{
    if (this->m_audioInput == audioInput)
        return;

    if (this->m_audioIn
        && !audioInput.contains(EXTERNAL_MEDIA_INPUT)
        && !audioInput.contains(DUMMY_INPUT_DEVICE))
        this->m_audioIn->setProperty("device", audioInput.value(0));

    this->m_mutex.lock();
    this->m_audioInput = audioInput;
    this->m_mutex.unlock();
    emit this->audioInputChanged(audioInput);
}

void AudioLayer::setAudioOutput(const QString &audioOutput)
{
    if (this->m_audioOut) {
        while (!this->m_mutex.tryLock()) {
            auto eventDispatcher = QThread::currentThread()->eventDispatcher();

            if (eventDispatcher)
                eventDispatcher->processEvents(QEventLoop::AllEvents);
        }

        auto state = this->m_audioOut->property("state");
        this->m_audioOut->setProperty("state", AkElement::ElementStateNull);
        this->m_audioOut->setProperty("device", audioOutput);
        this->m_audioOut->setProperty("state", state);

        if (this->m_audioOut->property("state") != state) {
            this->m_audioOut->setProperty("state", AkElement::ElementStateNull);
            this->m_audioOut->setProperty("device", ":dummyout:");
            this->m_audioOut->setProperty("state", state);
        }

        this->m_mutex.unlock();
    }
}

void AudioLayer::setInputCaps(const AkCaps &inputCaps)
{
    if (this->m_inputCaps == inputCaps)
        return;

    this->m_inputCaps = inputCaps;
    emit this->inputCapsChanged(inputCaps);
}

void AudioLayer::setOutputDeviceCaps(const AkCaps &outputDeviceCaps)
{
    if (this->m_outputDeviceCaps == outputDeviceCaps)
        return;

    this->m_outputDeviceCaps = outputDeviceCaps;
    emit this->outputDeviceCapsChanged(outputDeviceCaps);
}

void AudioLayer::setInputDescription(const QString &inputDescription)
{
    if (this->m_inputDescription == inputDescription)
        return;

    this->m_inputDescription = inputDescription;
    emit this->inputDescriptionChanged(inputDescription);
}

void AudioLayer::setInputState(AkElement::ElementState inputState)
{
    if (this->m_inputState == inputState)
        return;

    if (this->m_audioInput.contains(EXTERNAL_MEDIA_INPUT)) {
        if (this->m_inputCaps) {
            if (this->m_audioIn)
                this->m_audioIn->setState(AkElement::ElementStateNull);

            if (this->m_audioGenerator)
                this->m_audioGenerator->setState(AkElement::ElementStateNull);
        } else {
            if (this->m_audioIn)
                this->m_audioIn->setState(AkElement::ElementStateNull);

            if (this->m_audioGenerator)
                this->m_audioGenerator->setState(inputState);
        }
    } else if (this->m_audioInput.contains(DUMMY_INPUT_DEVICE)) {
        if (this->m_audioIn)
            this->m_audioIn->setState(AkElement::ElementStateNull);

        if (this->m_audioGenerator)
            this->m_audioGenerator->setState(inputState);
    } else {
        if (this->m_audioGenerator)
            this->m_audioGenerator->setState(AkElement::ElementStateNull);

        if (this->m_audioIn)
            this->m_audioIn->setState(inputState);
    }

    this->m_inputState = inputState;
    emit this->inputStateChanged(inputState);
}

bool AudioLayer::setOutputState(AkElement::ElementState outputState)
{
    if (this->m_audioOut)
        return this->m_audioOut->setProperty("state", outputState);

    return false;
}

void AudioLayer::resetAudioInput()
{
    QStringList devices;

    if (this->m_audioIn)
        devices << this->m_audioIn->property("defaultInput").toString();

    this->setAudioInput(devices);
}

void AudioLayer::resetAudioOutput()
{
    QString device;

    if (this->m_audioOut)
        device = this->m_audioOut->property("defaultOutput").toString();

    this->setAudioOutput(device);
}

void AudioLayer::resetInputCaps()
{
    this->setInputCaps(AkCaps());
}

void AudioLayer::resetOutputDeviceCaps()
{
    this->setOutputDeviceCaps(AkCaps());
}

void AudioLayer::resetInputDescription()
{
    this->setInputDescription("");
}

void AudioLayer::resetInputState()
{
    this->setInputState(AkElement::ElementStateNull);
}

void AudioLayer::resetOutputState()
{
    this->setOutputState(AkElement::ElementStateNull);
}

AkPacket AudioLayer::iStream(const AkPacket &packet)
{
    if (packet.caps().mimeType() != "audio/x-raw")
        return AkPacket();

    this->m_mutex.lock();

    if (this->m_audioOut)
        (*this->m_audioOut)(packet);

    if (this->m_audioSwitch
        && this->m_audioInput.contains(EXTERNAL_MEDIA_INPUT)) {
        (*this->m_audioSwitch)(packet);
    }

    this->m_mutex.unlock();

    return AkPacket();
}

void AudioLayer::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->m_engine == engine)
        return;

    this->m_engine = engine;

    if (engine)
        engine->rootContext()->setContextProperty("AudioLayer", this);
}

void AudioLayer::privInputsChanged(const QStringList &inputs)
{
    QStringList ins = QStringList {DUMMY_INPUT_DEVICE, EXTERNAL_MEDIA_INPUT}
                         + inputs;

    if (this->m_inputs == ins)
        return;

    this->m_inputs = ins;
    emit this->inputsChanged(ins);

    if (!this->m_audioInput.isEmpty()
        && !this->m_inputs.contains(this->m_audioInput.value(0))) {
            QString device = this->m_audioIn?
                                 this->m_audioIn->property("defaultInput").toString():
                                 QString();
            this->setAudioInput({device});
        }
}

void AudioLayer::updateInputState()
{
    AkCaps outputCaps = this->outputCaps();

    if (this->m_outputCaps != outputCaps) {
        this->m_outputCaps = outputCaps;
        emit this->outputCapsChanged(outputCaps);
    }

    AkElement::ElementState state = this->inputState();
    this->setInputState(AkElement::ElementStateNull);
    this->setInputState(state);
}

void AudioLayer::updateOutputState()
{
    AkCaps outputCaps = this->outputCaps();

    if (this->m_outputCaps != outputCaps) {
        this->m_outputCaps = outputCaps;
        emit this->outputCapsChanged(outputCaps);
    }

    AkElement::ElementState state = this->outputState();
    this->setOutputState(AkElement::ElementStateNull);
    this->setOutputState(state);
}

void AudioLayer::loadProperties()
{
    QSettings config;
    config.beginGroup("AudioConfigs");
    QString confInput = config.value("audioInput").toString();

    if (this->inputs().contains(confInput))
        this->setAudioInput({confInput});

    QString confOutput = config.value("audioOutput").toString();

    if (this->outputs().contains(confOutput))
        this->setAudioOutput(confOutput);

    config.endGroup();
}

void AudioLayer::saveAudioInput(const QStringList &audioInput)
{
    QSettings config;
    config.beginGroup("AudioConfigs");
    config.setValue("audioInput", audioInput);
    config.endGroup();
}

void AudioLayer::saveAudioOutput(const QString &audioOutput)
{
    QSettings config;
    config.beginGroup("AudioConfigs");
    config.setValue("audioOutput", audioOutput);
    config.endGroup();
}

void AudioLayer::saveProperties()
{
    QSettings config;
    config.beginGroup("AudioConfigs");
    config.setValue("audioInput", this->audioInput().value(0));
    config.setValue("audioOutput", this->audioOutput());
    config.endGroup();
}
