/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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
#include <akaudiocaps.h>
#include <akpacket.h>

#include "audiolayer.h"

#define DUMMY_INPUT_DEVICE ":dummyin:"
#define EXTERNAL_MEDIA_INPUT ":externalinput:"
#define MAX_SAMPLE_RATE 512e3

class AudioLayerPrivate
{
    public:
        QQmlApplicationEngine *m_engine;
        QStringList m_audioInput;
        QStringList m_inputs;
        AkCaps m_inputCaps;
        AkCaps m_outputCaps;
        QString m_inputDescription;
        AkElement::ElementState m_inputState;
        AkElementPtr m_pipeline;
        AkElementPtr m_audioOut;
        AkElementPtr m_audioIn;
        AkElementPtr m_audioConvert;
        AkElementPtr m_audioGenerator;
        AkElementPtr m_audioSwitch;
        QMutex m_mutex;
        QVector<int> m_commonSampleRates;

        AudioLayerPrivate():
            m_engine(nullptr),
            m_inputState(AkElement::ElementStateNull)
        {
        }
};

AudioLayer::AudioLayer(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new AudioLayerPrivate;

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
    this->d->m_pipeline = AkElement::create("Bin", "pipeline");

    if (this->d->m_pipeline) {
        QFile jsonFile(":/Webcamoid/share/audiopipeline.json");
        jsonFile.open(QFile::ReadOnly);
        QString description(jsonFile.readAll());
        jsonFile.close();

        this->d->m_pipeline->setProperty("description", description);

        QMetaObject::invokeMethod(this->d->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->d->m_audioOut),
                                  Q_ARG(QString, "audioOut"));
        QMetaObject::invokeMethod(this->d->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->d->m_audioIn),
                                  Q_ARG(QString, "audioIn"));
        QMetaObject::invokeMethod(this->d->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->d->m_audioGenerator),
                                  Q_ARG(QString, "audioGenerator"));
        QMetaObject::invokeMethod(this->d->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->d->m_audioSwitch),
                                  Q_ARG(QString, "audioSwitch"));
        QMetaObject::invokeMethod(this->d->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->d->m_audioConvert),
                                  Q_ARG(QString, "audioConvert"));
    }

    if (this->d->m_audioOut) {
        QString device = this->d->m_audioOut->property("defaultOutput").toString();
        this->d->m_audioOut->setProperty("device", device);

        QObject::connect(this->d->m_audioOut.data(),
                         SIGNAL(deviceChanged(const QString &)),
                         this,
                         SIGNAL(audioOutputChanged(const QString &)));
        QObject::connect(this->d->m_audioOut.data(),
                         SIGNAL(capsChanged(const AkCaps &)),
                         this,
                         SIGNAL(outputDeviceCapsChanged(const AkCaps &)));
        QObject::connect(this->d->m_audioOut.data(),
                         SIGNAL(outputsChanged(const QStringList &)),
                         this,
                         SIGNAL(outputsChanged(const QStringList &)));
        QObject::connect(this->d->m_audioOut.data(),
                         SIGNAL(stateChanged(AkElement::ElementState)),
                         this,
                         SIGNAL(outputStateChanged(AkElement::ElementState)));
    }

    if (this->d->m_audioIn) {
        QString device = this->d->m_audioIn->property("defaultInput").toString();
        this->d->m_audioIn->setProperty("device", device);
        this->d->m_audioInput = QStringList {device};
        this->d->m_inputs = QStringList {DUMMY_INPUT_DEVICE, EXTERNAL_MEDIA_INPUT}
                          + this->d->m_audioIn->property("inputs").toStringList();

        QObject::connect(this->d->m_audioIn.data(),
                         SIGNAL(inputsChanged(const QStringList &)),
                         this,
                         SLOT(privInputsChanged(const QStringList &)));
    }

    if (this->d->m_audioOut) {
        QObject::connect(this->d->m_audioOut.data(),
                         SIGNAL(audioLibChanged(const QString &)),
                         this,
                         SLOT(saveAudioDeviceAudioLib(const QString &)));
    } else if (this->d->m_audioIn) {
        QObject::connect(this->d->m_audioIn.data(),
                         SIGNAL(audioLibChanged(const QString &)),
                         this,
                         SLOT(saveAudioDeviceAudioLib(const QString &)));
    }

    if (this->d->m_audioConvert) {
        QObject::connect(this->d->m_audioConvert.data(),
                         SIGNAL(convertLibChanged(const QString &)),
                         this,
                         SLOT(saveAudioConvertConvertLib(const QString &)));
    }

    this->d->m_outputCaps = this->outputCaps();

    if (this->d->m_audioSwitch) {
        QObject::connect(this->d->m_audioSwitch.data(),
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

    this->d->m_mutex.lock();
    this->d->m_audioOut.clear();
    this->d->m_audioSwitch.clear();
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
    if (this->d->m_audioOut)
        return this->d->m_audioOut->property("outputs").toStringList();

    return QStringList();
}

AkCaps AudioLayer::inputCaps() const
{
    return this->d->m_inputCaps;
}

AkCaps AudioLayer::outputCaps() const
{
    if (this->d->m_audioInput.contains(EXTERNAL_MEDIA_INPUT)) {
        if (this->d->m_inputCaps)
            return this->d->m_inputCaps;
        else {
            AkCaps caps;

            if (this->d->m_audioGenerator)
                caps = this->d->m_audioGenerator->property("caps").toString();

            return caps;
        }
    } else if (this->d->m_audioInput.contains(DUMMY_INPUT_DEVICE)) {
        AkCaps caps;

        if (this->d->m_audioGenerator)
            caps = this->d->m_audioGenerator->property("caps").toString();

        return caps;
    }

    return this->d->m_audioIn?
                this->d->m_audioIn->property("caps").value<AkCaps>():
                AkCaps();
}

AkCaps AudioLayer::inputDeviceCaps() const
{
    return this->outputCaps();
}

AkCaps AudioLayer::outputDeviceCaps() const
{
    if (this->d->m_audioOut)
        return this->d->m_audioOut->property("caps").value<AkCaps>();

    return AkCaps();
}

QString AudioLayer::inputDescription() const
{
    return this->d->m_inputDescription;
}

QString AudioLayer::description(const QString &device) const
{
    QString description;

    if (device == EXTERNAL_MEDIA_INPUT)
        description = this->d->m_inputDescription;
    else if (device == DUMMY_INPUT_DEVICE)
        description = QString("Dummy Input");
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

AkAudioCaps AudioLayer::preferredFormat(const QString &device)
{
    if (device == DUMMY_INPUT_DEVICE) {
        return AkAudioCaps(AkAudioCaps::SampleFormat_s16, 1, 8000);
    } else if (device == EXTERNAL_MEDIA_INPUT) {
        if (this->d->m_inputCaps)
            return AkAudioCaps(this->d->m_inputCaps);

        AkAudioCaps caps;

        if (this->d->m_audioGenerator)
            caps = this->d->m_audioGenerator->property("caps").toString();

        return caps;
    } else if (this->d->m_audioOut) {
        AkAudioCaps preferredFormat;
        QMetaObject::invokeMethod(this->d->m_audioOut.data(),
                                  "preferredFormat",
                                  Q_RETURN_ARG(AkAudioCaps, preferredFormat),
                                  Q_ARG(QString, device));

        return preferredFormat;
    }

    return AkAudioCaps();
}

QStringList AudioLayer::supportedFormats(const QString &device)
{
    if (device == DUMMY_INPUT_DEVICE) {
        return QStringList {"flt", "s32", "s16", "u8"};
    } else if (device == EXTERNAL_MEDIA_INPUT) {
        AkAudioCaps caps;

        if (this->d->m_inputCaps)
            caps = this->d->m_inputCaps;
        else if (this->d->m_audioGenerator)
            caps = this->d->m_audioGenerator->property("caps").toString();

        if (caps)
            return QStringList {AkAudioCaps::sampleFormatToString(caps.format())};
    } else if (this->d->m_audioOut) {
        QList<AkAudioCaps::SampleFormat> supportedFormats;
        QMetaObject::invokeMethod(this->d->m_audioOut.data(),
                                  "supportedFormats",
                                  Q_RETURN_ARG(QList<AkAudioCaps::SampleFormat>, supportedFormats),
                                  Q_ARG(QString, device));

        QStringList supportedFormatsStr;

        for (auto &format: supportedFormats)
            supportedFormatsStr << AkAudioCaps::sampleFormatToString(format);

        return supportedFormatsStr;
    }

    return QStringList();
}

QList<int> AudioLayer::supportedChannels(const QString &device)
{
    if (device == DUMMY_INPUT_DEVICE) {
        return QList<int> {1, 2};
    } else if (device == EXTERNAL_MEDIA_INPUT) {
        AkAudioCaps caps;

        if (this->d->m_inputCaps)
            caps = this->d->m_inputCaps;
        else if (this->d->m_audioGenerator)
            caps = this->d->m_audioGenerator->property("caps").toString();

        if (caps)
            return QList<int> {caps.channels()};
    } else if (this->d->m_audioOut) {
        QList<int> supportedChannels;
        QMetaObject::invokeMethod(this->d->m_audioOut.data(),
                                  "supportedChannels",
                                  Q_RETURN_ARG(QList<int>, supportedChannels),
                                  Q_ARG(QString, device));

        return supportedChannels;
    }

    return QList<int>();
}

QList<int> AudioLayer::supportedSampleRates(const QString &device)
{
    if (device == DUMMY_INPUT_DEVICE) {
        return this->d->m_commonSampleRates.toList();
    } else if (device == EXTERNAL_MEDIA_INPUT) {
        AkAudioCaps caps;

        if (this->d->m_inputCaps)
            caps = this->d->m_inputCaps;
        else if (this->d->m_audioGenerator)
            caps = this->d->m_audioGenerator->property("caps").toString();

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

    return QList<int>();
}

void AudioLayer::setAudioInput(const QStringList &audioInput)
{
    if (this->d->m_audioInput == audioInput)
        return;

    if (this->d->m_audioIn
        && !audioInput.contains(EXTERNAL_MEDIA_INPUT)
        && !audioInput.contains(DUMMY_INPUT_DEVICE))
        this->d->m_audioIn->setProperty("device", audioInput.value(0));

    this->d->m_mutex.lock();
    this->d->m_audioInput = audioInput;
    this->d->m_mutex.unlock();
    emit this->audioInputChanged(audioInput);
}

void AudioLayer::setAudioOutput(const QString &audioOutput)
{
    if (this->d->m_audioOut) {
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
    }
}

void AudioLayer::setInputCaps(const AkCaps &inputCaps)
{
    if (this->d->m_inputCaps == inputCaps)
        return;

    this->d->m_inputCaps = inputCaps;
    emit this->inputCapsChanged(inputCaps);
}

void AudioLayer::setInputDeviceCaps(const AkCaps &inputDeviceCaps)
{
    if (this->d->m_audioInput.contains(EXTERNAL_MEDIA_INPUT)
        || this->inputDeviceCaps() == inputDeviceCaps)
        return;

    if (this->d->m_audioInput.contains(DUMMY_INPUT_DEVICE)) {
        if (this->d->m_audioGenerator)
            this->d->m_audioGenerator->setProperty("caps", inputDeviceCaps.toString());
    } else if (this->d->m_audioIn)
        this->d->m_audioIn->setProperty("caps", QVariant::fromValue(inputDeviceCaps));

    emit inputDeviceCapsChanged(inputDeviceCaps);
}

void AudioLayer::setOutputDeviceCaps(const AkCaps &outputDeviceCaps)
{
    if (this->d->m_audioOut)
        this->d->m_audioOut->setProperty("caps", QVariant::fromValue(outputDeviceCaps));
}

void AudioLayer::setInputDescription(const QString &inputDescription)
{
    if (this->d->m_inputDescription == inputDescription)
        return;

    this->d->m_inputDescription = inputDescription;
    emit this->inputDescriptionChanged(inputDescription);
}

void AudioLayer::setInputState(AkElement::ElementState inputState)
{
    if (this->d->m_inputState == inputState)
        return;

    if (this->d->m_audioInput.contains(EXTERNAL_MEDIA_INPUT)) {
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

void AudioLayer::resetInputCaps()
{
    this->setInputCaps(AkCaps());
}

void AudioLayer::resetInputDeviceCaps()
{
    if (this->d->m_audioInput.contains(EXTERNAL_MEDIA_INPUT)) {
    } else if (this->d->m_audioInput.contains(DUMMY_INPUT_DEVICE)) {
        if (this->d->m_audioGenerator)
            QMetaObject::invokeMethod(this->d->m_audioGenerator.data(),
                                      "resetCaps");
    } else if (this->d->m_audioIn)
        QMetaObject::invokeMethod(this->d->m_audioIn.data(), "resetCaps");
}

void AudioLayer::resetOutputDeviceCaps()
{
    if (this->d->m_audioOut)
        QMetaObject::invokeMethod(this->d->m_audioOut.data(), "resetCaps");
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

    this->d->m_mutex.lock();

    if (this->d->m_audioOut)
        (*this->d->m_audioOut)(packet);

    if (this->d->m_audioSwitch
        && this->d->m_audioInput.contains(EXTERNAL_MEDIA_INPUT)) {
        (*this->d->m_audioSwitch)(packet);
    }

    this->d->m_mutex.unlock();

    return AkPacket();
}

void AudioLayer::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->d->m_engine == engine)
        return;

    this->d->m_engine = engine;

    if (engine)
        engine->rootContext()->setContextProperty("AudioLayer", this);
}

void AudioLayer::privInputsChanged(const QStringList &inputs)
{
    QStringList ins = QStringList {DUMMY_INPUT_DEVICE, EXTERNAL_MEDIA_INPUT}
                         + inputs;

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

void AudioLayer::updateInputState()
{
    AkCaps outputCaps = this->outputCaps();

    if (this->d->m_outputCaps != outputCaps) {
        this->d->m_outputCaps = outputCaps;
        emit this->outputCapsChanged(outputCaps);
    }

    AkElement::ElementState state = this->inputState();
    this->setInputState(AkElement::ElementStateNull);
    this->setInputState(state);
}

void AudioLayer::updateOutputState()
{
    AkCaps outputCaps = this->outputCaps();

    if (this->d->m_outputCaps != outputCaps) {
        this->d->m_outputCaps = outputCaps;
        emit this->outputCapsChanged(outputCaps);
    }

    AkElement::ElementState state = this->outputState();
    this->setOutputState(AkElement::ElementStateNull);
    this->setOutputState(state);
}

void AudioLayer::loadProperties()
{
    QSettings config;

    config.beginGroup("Libraries");
    auto audioDev = this->d->m_audioOut?
                        this->d->m_audioOut: this->d->m_audioIn;

    if (audioDev)
        audioDev->setProperty("audioLib",
                              config.value("AudioDevice.audioLib",
                                           audioDev->property("audioLib")));

    if (this->d->m_audioConvert)
        this->d->m_audioConvert->setProperty("convertLib",
                                             config.value("AudioConvert.convertLib",
                                                          this->d->m_audioConvert->property("convertLib")));

    config.endGroup();

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

void AudioLayer::saveAudioDeviceAudioLib(const QString &audioLib)
{
    QSettings config;
    config.beginGroup("Libraries");
    config.setValue("AudioDevice.audioLib", audioLib);
    config.endGroup();
}

void AudioLayer::saveAudioConvertConvertLib(const QString &convertLib)
{
    QSettings config;
    config.beginGroup("Libraries");
    config.setValue("AudioConvert.convertLib", convertLib);
    config.endGroup();
}

void AudioLayer::saveProperties()
{
    QSettings config;
    config.beginGroup("AudioConfigs");
    config.setValue("audioInput", this->audioInput().value(0));
    config.setValue("audioOutput", this->audioOutput());
    config.endGroup();

    config.beginGroup("Libraries");
    auto audioDev = this->d->m_audioOut?
                        this->d->m_audioOut: this->d->m_audioIn;

    if (audioDev)
        config.setValue("AudioDevice.audioLib", audioDev->property("audioLib"));

    if (this->d->m_audioConvert)
        config.setValue("AudioConvert.convertLib",
                        this->d->m_audioConvert->property("convertLib"));

    config.endGroup();
}

#include "moc_audiolayer.cpp"
