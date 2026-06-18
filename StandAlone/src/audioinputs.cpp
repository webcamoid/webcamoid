/* Webcamoid, camera capture application.
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
#include <QMutex>
#include <QSettings>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <ak.h>
#include <akaudiopacket.h>
#include <akcaps.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <iak/akelement.h>

#include "audioinputs.h"
#include "akaudiomixer.h"

// Represents one active source slot in the mixer.
struct AudioSource
{
    QString      device;
    qreal        volume {1.0};
    AkElementPtr element;
};

// Video sources registered via addInput().
struct VideoSource
{
    QString media;
    QString description;
    qreal   volume {1.0};
    qint64  streamId {-1};
};

class AudioInputsPrivate
{
    public:
        AudioInputs *self;
        QQmlApplicationEngine *m_engine {nullptr};
        AkElementPtr m_audioInput {akPluginManager->create<AkElement>("AudioSource/AudioDevice")};

        QVector<AudioSource> m_audioSources;
        QVector<VideoSource> m_videoSources;

        AkAudioMixer m_mixer;
        QMutex m_mutex;
        AkElement::ElementState m_inputState  {AkElement::ElementStateNull};
        AkElement::ElementState m_outputState {AkElement::ElementStateNull};
        int m_inputRefs {0};
        int m_outputRefs {0};
        AkAudioCaps m_deviceCaps {AkAudioCaps::SampleFormat_s16,
                                  AkAudioCaps::Layout_stereo,
                                  false,
                                  44100};
        int m_latency {25};

        explicit AudioInputsPrivate(AudioInputs *self);

        void rebuildMixer();
        void loadProperties();
        void saveProperties();
        AkAudioCaps closestCaps(const QString &device) const;
};

AudioInputs::AudioInputs(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new AudioInputsPrivate(this);

    this->setQmlEngine(engine);

    // Seed the available inputs list from the AudioDevice element.
    if (this->d->m_audioInput)
        QObject::connect(this->d->m_audioInput.data(),
                         SIGNAL(inputsChanged(QStringList)),
                         this,
                         SLOT(privInputsChanged(QStringList)));

    // Connect mixer output to our oStream.
    QObject::connect(&this->d->m_mixer,
                     &AkAudioMixer::oStream,
                     this,
                     &AudioInputs::sendPacket,
                     Qt::DirectConnection);

    this->d->loadProperties();
}

AudioInputs::~AudioInputs()
{
    this->resetInputState();
    this->resetOutputState();
    delete this->d;
}

QStringList AudioInputs::activeInputs() const
{
    QStringList activeInputs;

    for (auto &src: this->d->m_audioSources)
        activeInputs << src.device;

    for (auto &src: this->d->m_videoSources)
        activeInputs << src.media;

    return activeInputs;
}

QStringList AudioInputs::inputs() const
{
    if (this->d->m_audioInput)
        return this->d->m_audioInput->property("inputs").toStringList();

    return {};
}

QString AudioInputs::description(const QString &device) const
{
    for (auto &src: this->d->m_videoSources)
        if (src.media == device)
            return src.description;

    QString description;

    if (this->d->m_audioInput)
        QMetaObject::invokeMethod(this->d->m_audioInput.data(),
                                  "description",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, device));

    return description;
}

AkElement::ElementState AudioInputs::inputState() const
{
    return this->d->m_inputState;
}

AkElement::ElementState AudioInputs::outputState() const
{
    return this->d->m_outputState;
}

AkAudioCaps AudioInputs::deviceCaps() const
{
    return this->d->m_deviceCaps;
}

int AudioInputs::latency() const
{
    return this->d->m_latency;
}

qreal AudioInputs::volume(const QString &device) const
{
    for (auto &src: this->d->m_audioSources)
        if (src.device == device)
            return src.volume;

    for (auto &src: this->d->m_videoSources)
        if (src.media == device)
            return src.volume;

    return 0.0;
}

bool AudioInputs::setVolume(const QString &device, qreal volume)
{
    size_t i = 0;

    for (auto &src: this->d->m_audioSources) {
        if (src.device == device) {
            src.volume = volume;

            if (src.element)
                QMetaObject::invokeMethod(src.element.data(), "setVolume",
                                          Q_ARG(qreal, volume));

            this->d->saveProperties();

            return true;
        }

        i++;
    }

    for (auto &src: this->d->m_videoSources) {
        if (src.media == device) {
            src.volume = volume;
            this->d->saveProperties();

            return true;
        }

        i++;
    }

    return false;
}

AkAudioCaps AudioInputs::preferredFormat() const
{
    return {AkAudioCaps::SampleFormat_s16,
            AkAudioCaps::Layout_stereo,
            false,
            44100};
}

QList<AkAudioCaps::SampleFormat> AudioInputs::supportedFormats() const
{
    return {AkAudioCaps::SampleFormat_s16,
            AkAudioCaps::SampleFormat_s32,
            AkAudioCaps::SampleFormat_flt};
}

QList<AkAudioCaps::ChannelLayout> AudioInputs::supportedChannelLayouts() const
{
    return {AkAudioCaps::Layout_mono, AkAudioCaps::Layout_stereo};
}

QList<int> AudioInputs::supportedSampleRates() const
{
    return {44100, 48000};
}

QVariantList AudioInputs::supportedFormatsVariant() const
{
    QVariantList list;

    for (auto &fmt: this->supportedFormats())
        list << fmt;

    return list;
}

QVariantList AudioInputs::supportedChannelLayoutsVariant() const
{
    QVariantList list;

    for (auto &layout: this->supportedChannelLayouts())
        list << layout;

    return list;
}

bool AudioInputs::addInput(const QString &device)
{
    if (device.isEmpty())
        return false;

    // Prevent duplicates.
    for (auto &src: this->d->m_audioSources)
        if (src.device == device)
            return false;

    // Only allow devices that are in the available inputs list.
    QStringList audioInputs;

    if (this->d->m_audioInput)
        audioInputs = this->d->m_audioInput->property("inputs").toStringList();

    if (!audioInputs.contains(device))
        return false;

    AudioSource src;
    src.device = device;
    src.volume = 1.0;

    this->d->m_audioSources << src;
    this->d->rebuildMixer();

    this->d->saveProperties();
    emit this->activeInputsChanged(this->activeInputs());

    return true;
}

qint64 AudioInputs::addInput(const QString &device, const QString &description)
{
    if (device.isEmpty() || description.isEmpty())
        return -1;

    for (auto &src: this->d->m_audioSources)
        if (src.device == device)
            return -1;

    // Update caps/description if already registered.
    for (auto &vs: this->d->m_videoSources)
        if (vs.media == device) {
            vs.description = description;

            return vs.streamId;
        }

    VideoSource vs;
    vs.media       = device;
    vs.description = description;
    vs.streamId    = Ak::id();
    this->d->m_videoSources << vs;
    this->d->rebuildMixer();
    this->d->saveProperties();
    emit this->activeInputsChanged(this->activeInputs());

    return vs.streamId;
}

bool AudioInputs::removeInput(const QString &device)
{
    for (int i = 0; i < this->d->m_audioSources.size(); ++i) {
        if (this->d->m_audioSources[i].device == device) {
            this->d->m_audioSources.removeAt(i);
            this->d->rebuildMixer();
            this->d->saveProperties();

            emit this->activeInputsChanged(this->activeInputs());

            return true;
        }
    }

    for (int i = 0; i < this->d->m_videoSources.size(); ++i) {
        if (this->d->m_videoSources[i].media == device) {
            this->d->m_videoSources.removeAt(i);
            this->d->rebuildMixer();
            this->d->saveProperties();

            emit this->activeInputsChanged(this->activeInputs());

            return true;
        }
    }

    return false;
}

void AudioInputs::setDeviceCaps(const AkAudioCaps &deviceCaps)
{
    if (this->d->m_deviceCaps == deviceCaps)
        return;

    this->d->m_deviceCaps = deviceCaps;
    this->d->m_mixer.setOutputCaps(deviceCaps);
    this->d->saveProperties();
    emit this->deviceCapsChanged(deviceCaps);
}

void AudioInputs::resetDeviceCaps()
{
    this->setDeviceCaps(this->preferredFormat());
}

void AudioInputs::setInputState(AkElement::ElementState state)
{
    if (state == AkElement::ElementStatePlaying) {
        ++this->d->m_inputRefs;

        // Already running, only refcount changed.
        if (this->d->m_inputRefs > 1)
            return;

        // Start all hardware capture elements.
        for (auto &src: this->d->m_audioSources) {
            if (!src.element)
                continue;

            src.element->setProperty("caps", this->d->closestCaps(src.device).toVariant());
            src.element->setProperty("latency", this->d->m_latency);
            src.element->setState(AkElement::ElementStatePlaying);
        }

        this->d->m_inputState = AkElement::ElementStatePlaying;
        emit this->inputStateChanged(this->d->m_inputState);
        qDebug() << "Audio inputs started";
    } else {
        // Treat non-Playing as release.
        if (this->d->m_inputRefs <= 0) {
            this->d->m_inputRefs = 0;

            return;
        }

        --this->d->m_inputRefs;

        // Still in use.
        if (this->d->m_inputRefs > 0)
            return;

        // Stop all hardware source elements.
        for (auto &src: this->d->m_audioSources)
            if (src.element)
                src.element->setState(AkElement::ElementStateNull);

        this->d->m_inputState = AkElement::ElementStateNull;
        emit this->inputStateChanged(this->d->m_inputState);
        qDebug() << "Audio inputs stopped";
    }
}

void AudioInputs::setOutputState(AkElement::ElementState state)
{
    if (state == AkElement::ElementStatePlaying) {
        ++this->d->m_outputRefs;

        // Already running, only refcount changed.
        if (this->d->m_outputRefs > 1)
            return;

        // Start mixer output.
        if (this->d->m_mixer.inputs() > 0) {
            qDebug() << "Mixer output caps" << this->d->m_deviceCaps;
            this->d->m_mixer.setOutputCaps(this->d->m_deviceCaps);
            this->d->m_mixer.setState(AkElement::ElementStatePlaying);
        }

        this->d->m_outputState = AkElement::ElementStatePlaying;
        emit this->outputStateChanged(this->d->m_outputState);
        qDebug() << "Audio output started";
    } else {
        // Treat non-Playing as release.
        if (this->d->m_outputRefs <= 0) {
            this->d->m_outputRefs = 0;
            return;
        }

        --this->d->m_outputRefs;

        // Still in use.
        if (this->d->m_outputRefs > 0)
            return;

        // Stop mixer output.
        this->d->m_mixer.setState(AkElement::ElementStateNull);
        this->d->m_outputState = AkElement::ElementStateNull;
        emit this->outputStateChanged(this->d->m_outputState);

        qDebug() << "Audio output stopped";
    }
}

void AudioInputs::setLatency(int latency)
{
    if (this->d->m_latency == latency)
        return;

    this->d->m_latency = latency;
    emit this->latencyChanged(latency);
}

void AudioInputs::resetActiveInputs()
{
    for (int i = 0; i < this->d->m_audioSources.size(); ++i)
        this->d->m_audioSources.removeAt(i);

    for (int i = 0; i < this->d->m_videoSources.size(); ++i)
        this->d->m_videoSources.removeAt(i);

    this->d->rebuildMixer();
    this->d->saveProperties();

    emit this->activeInputsChanged(this->activeInputs());
}

void AudioInputs::resetInputState()
{
    this->setInputState(AkElement::ElementStateNull);
}

void AudioInputs::resetOutputState()
{
    this->setOutputState(AkElement::ElementStateNull);
}

void AudioInputs::resetLatency()
{
    this->setLatency(25);
}

void AudioInputs::updateDevices()
{
    if (this->d->m_audioInput)
        QMetaObject::invokeMethod(this->d->m_audioInput.data(),
                                  "updateDevices");
}

AkPacket AudioInputs::iStream(const AkPacket &packet)
{
    // Receives audio from a registered video source.
    if (packet.caps().type() != AkCaps::CapsAudio)
        return {};

    AkAudioPacket pkt;

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_videoSources.isEmpty())
            return {};

        auto volume = this->d->m_videoSources.first().volume;
        pkt = AkAudioPacket(packet).adjustVolume(volume);
        pkt.setId(this->d->m_audioSources.size());
    }

    this->d->m_mixer.iStream(pkt);

    if (this->d->m_inputState == AkElement::ElementStatePlaying)
        emit this->vumeter(this->d->m_videoSources.first().media, pkt.volume());

    return {};
}

void AudioInputs::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->d->m_engine == engine)
        return;

    this->d->m_engine = engine;

    if (engine)
        engine->rootContext()->setContextProperty("audioInputs", this);
}

void AudioInputs::sendPacket(const AkPacket &packet)
{
    auto pkt = packet;
    pkt.setIndex(1);
    emit this->oStream(pkt);
}

void AudioInputs::privInputsChanged(const QStringList &inputs)
{
    emit this->inputsChanged(inputs);

    // Remove any active source that is no longer available.
    bool changed = false;

    for (int i = this->d->m_audioSources.size() - 1; i >= 0; --i)
        if (!inputs.contains(this->d->m_audioSources[i].device)) {
            this->d->m_audioSources.removeAt(i);
            changed = true;
        }

    if (changed) {
        this->d->rebuildMixer();
        emit this->activeInputsChanged(this->activeInputs());
    }
}

AudioInputsPrivate::AudioInputsPrivate(AudioInputs *self):
    self(self)
{
}

// Rebuild mixer slots from m_activeSources and restart according to the
// current inputState / outputState.
void AudioInputsPrivate::rebuildMixer()
{
    QMutexLocker locker(&this->m_mutex);

    // Tear down existing elements and mixer.
    for (auto &src: this->m_audioSources)
        if (src.element)
            src.element->setState(AkElement::ElementStateNull);

    this->m_mixer.setState(AkElement::ElementStateNull);
    this->m_mixer.setInputs(this->m_audioSources.size()
                            + this->m_videoSources.size());
    int streamIndex = 0;

    // Reconnect each element to its mixer slot.
    for (auto &src: this->m_audioSources) {
        src.element =
                akPluginManager->create<AkElement>("AudioSource/AudioDevice");

        if (src.element) {
            src.element->setProperty("device", src.device);
            QMetaObject::invokeMethod(src.element.data(), "setVolume",
                                      Q_ARG(qreal, src.volume));

            auto device = src.device;
            QObject::connect(src.element.data(),
                             &AkElement::oStream,
                             this->self,
                             [this, streamIndex, device](const AkPacket &pkt) {
                                 auto p = pkt;
                                 p.setId(streamIndex);
                                 this->m_mixer.iStream(p);

                                 if (this->m_inputState == AkElement::ElementStatePlaying)
                                     emit this->self->vumeter(device, AkAudioPacket(pkt).volume());
                             },
                             Qt::DirectConnection);
        }

        streamIndex++;
    }

    if (this->m_mixer.inputs() < 1)
        return;

    // Restore mixer output if it was running.
    if (this->m_outputState == AkElement::ElementStatePlaying) {
        this->m_mixer.setOutputCaps(this->m_deviceCaps);
        this->m_mixer.setState(AkElement::ElementStatePlaying);
    }

    // Restore hardware element capture if inputs were running.
    if (this->m_inputState == AkElement::ElementStatePlaying)
        for (auto &src: this->m_audioSources)
            if (src.element) {
                src.element->setProperty("caps", this->closestCaps(src.device).toVariant());
                src.element->setProperty("latency", this->m_latency);
                src.element->setState(AkElement::ElementStatePlaying);
            }
}

void AudioInputsPrivate::loadProperties()
{
    QSettings config;
    config.beginGroup("AudioInputsConfigs");

    // Restore global caps and latency.
    auto savedFormat = config.value("format",
                                    int(AkAudioCaps::SampleFormat_s16)).toInt();
    auto savedLayout = config.value("layout",
                                    int(AkAudioCaps::Layout_stereo)).toInt();
    auto savedRate   = config.value("rate", 44100).toInt();
    this->m_deviceCaps = {AkAudioCaps::SampleFormat(savedFormat),
                          AkAudioCaps::ChannelLayout(savedLayout),
                          false,
                          savedRate};
    this->m_latency = config.value("latency", 25).toInt();

    int n = config.beginReadArray("audioSources");

    for (int i = 0; i < n; ++i) {
        config.setArrayIndex(i);
        QString device = config.value("device").toString();
        qreal volume   = config.value("volume", 1.0).toReal();

        if (self->inputs().contains(device)) {
            AudioSource src;
            src.device = device;
            src.volume = volume;
            this->m_audioSources << src;
        }
    }

    config.endArray();
    config.endGroup();

    if (!this->m_audioSources.isEmpty())
        this->rebuildMixer();
}

void AudioInputsPrivate::saveProperties()
{
    QSettings config;
    config.beginGroup("AudioInputsConfigs");

    config.setValue("format", int(this->m_deviceCaps.format()));
    config.setValue("layout", int(this->m_deviceCaps.layout()));
    config.setValue("rate",   this->m_deviceCaps.rate());
    config.setValue("latency", this->m_latency);

    config.beginWriteArray("audioSources", this->m_audioSources.size());

    for (int i = 0; i < this->m_audioSources.size(); ++i) {
        config.setArrayIndex(i);
        config.setValue("device", this->m_audioSources[i].device);
        config.setValue("volume", this->m_audioSources[i].volume);
    }

    config.endArray();
    config.endGroup();
}

AkAudioCaps AudioInputsPrivate::closestCaps(const QString &device) const
{
    if (device.isEmpty() || !this->m_audioInput)
        return {};

    // Query supported lists from the device element.
    QList<AkAudioCaps::SampleFormat> formats;
    QList<AkAudioCaps::ChannelLayout> layouts;
    QList<int> rates;

    QMetaObject::invokeMethod(this->m_audioInput.data(), "supportedFormats",
                              Q_RETURN_ARG(QList<AkAudioCaps::SampleFormat>, formats),
                              Q_ARG(QString, device));
    QMetaObject::invokeMethod(this->m_audioInput.data(), "supportedChannelLayouts",
                              Q_RETURN_ARG(QList<AkAudioCaps::ChannelLayout>, layouts),
                              Q_ARG(QString, device));
    QMetaObject::invokeMethod(this->m_audioInput.data(), "supportedSampleRates",
                              Q_RETURN_ARG(QList<int>, rates),
                              Q_ARG(QString, device));

    // Closest format: match bits per sample.
    auto format = this->m_deviceCaps.format();

    if (!formats.contains(format)) {
        int targetBps = AkAudioCaps::bitsPerSample(format);
        int bestDiff = INT_MAX;

        for (auto &fmt: formats) {
            int diff = qAbs(AkAudioCaps::bitsPerSample(fmt) - targetBps);

            if (diff < bestDiff) {
                bestDiff = diff;
                format = fmt;
            }
        }
    }

    // Closest layout: prefer stereo, then mono, then first available.
    auto layout = this->m_deviceCaps.layout();

    if (!layouts.contains(layout)) {
        if (layouts.contains(AkAudioCaps::Layout_stereo))
            layout = AkAudioCaps::Layout_stereo;
        else if (layouts.contains(AkAudioCaps::Layout_mono))
            layout = AkAudioCaps::Layout_mono;
        else if (!layouts.isEmpty())
            layout = layouts.first();
    }

    // Closest rate: minimum absolute difference.
    int sampleRate = this->m_deviceCaps.rate();

    if (!rates.contains(sampleRate)) {
        int bestRate = sampleRate;
        int bestDiff = INT_MAX;

        for (auto &rate: rates) {
            int diff = qAbs(rate - sampleRate);

            if (diff < bestDiff) {
                bestDiff = diff;
                bestRate = rate;
            }
        }

        sampleRate = bestRate;
    }

    return {format, layout, false, sampleRate};
}

#include "moc_audioinputs.cpp"
