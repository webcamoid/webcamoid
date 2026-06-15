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

#include <QMutex>
#include <QFile>
#include <QThread>
#include <QAbstractEventDispatcher>
#include <QSettings>
#include <QQuickItem>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQmlApplicationEngine>
#include <akaudiopacket.h>
#include <akcaps.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <iak/akelement.h>

#include "audiooutputs.h"

#define MAX_SAMPLE_RATE 512e3

using ObjectPtr = QSharedPointer<QObject>;

class AudioOutputsPrivate
{
    public:
        AudioOutputs *self;
        QQmlApplicationEngine *m_engine {nullptr};
        AkElementPtr m_audioOut {akPluginManager->create<AkElement>("AudioSource/AudioDevice")};
        AkAudioCaps m_deviceCaps {AkAudioCaps::SampleFormat_s16,
                                  AkAudioCaps::Layout_stereo,
                                  false,
                                  44100};
        int m_latency {25};
        qreal m_volume {1.0};
        QMutex m_mutex;

        explicit AudioOutputsPrivate(AudioOutputs *self);
        void loadProperties();
        void saveProperties();
        AkAudioCaps closestCaps(const QString &device) const;
};

AudioOutputs::AudioOutputs(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new AudioOutputsPrivate(this);
    this->setQmlEngine(engine);

    if (this->d->m_audioOut) {
        auto device = this->d->m_audioOut->property("defaultOutput").toString();
        this->d->m_audioOut->setProperty("device", device);

        QObject::connect(this->d->m_audioOut.data(),
                         SIGNAL(deviceChanged(QString)),
                         this,
                         SIGNAL(audioOutputChanged(QString)));
        QObject::connect(this->d->m_audioOut.data(),
                         SIGNAL(outputsChanged(QStringList)),
                         this,
                         SIGNAL(outputsChanged(QStringList)));
        QObject::connect(this->d->m_audioOut.data(),
                         SIGNAL(stateChanged(AkElement::ElementState)),
                         this,
                         SIGNAL(stateChanged(AkElement::ElementState)));
    }

    this->d->loadProperties();
}

AudioOutputs::~AudioOutputs()
{
    this->resetState();

    this->d->m_mutex.lock();
    this->d->m_audioOut.clear();
    this->d->m_mutex.unlock();

    delete this->d;
}

QString AudioOutputs::audioOutput() const
{
    if (this->d->m_audioOut)
        return this->d->m_audioOut->property("device").toString();

    return QString();
}

QStringList AudioOutputs::outputs() const
{
    QStringList outputs;

    if (this->d->m_audioOut)
        outputs = this->d->m_audioOut->property("outputs").toStringList();

    return outputs;
}

AkAudioCaps AudioOutputs::deviceCaps() const
{
    return this->d->m_deviceCaps;
}

QString AudioOutputs::description(const QString &device) const
{
    QString description;

    if (this->d->m_audioOut)
        QMetaObject::invokeMethod(this->d->m_audioOut.data(),
                                  "description",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, device));

    return description;
}

AkElement::ElementState AudioOutputs::state() const
{
    if (this->d->m_audioOut)
        return this->d->m_audioOut->property("state").value<AkElement::ElementState>();

    return AkElement::ElementStateNull;
}

int AudioOutputs::latency() const
{
    return this->d->m_latency;
}

qreal AudioOutputs::volume() const
{
    return this->d->m_volume;
}

AkAudioCaps AudioOutputs::preferredFormat()
{
    return {AkAudioCaps::SampleFormat_s16,
            AkAudioCaps::Layout_stereo,
            false,
            44100};
}

QList<AkAudioCaps::SampleFormat> AudioOutputs::supportedFormats() const
{
    return {AkAudioCaps::SampleFormat_s16,
            AkAudioCaps::SampleFormat_s32,
            AkAudioCaps::SampleFormat_flt};
}

QList<AkAudioCaps::ChannelLayout> AudioOutputs::supportedChannelLayouts() const
{
    return {AkAudioCaps::Layout_mono, AkAudioCaps::Layout_stereo};
}

QList<int> AudioOutputs::supportedSampleRates() const
{
    return {44100, 48000};
}

QVariantList AudioOutputs::supportedFormatsVariant() const
{
    QVariantList list;

    for (auto &format: this->supportedFormats())
        list << format;

    return list;
}

QVariantList AudioOutputs::supportedChannelLayoutsVariant() const
{
    QVariantList list;

    for (auto &format: this->supportedChannelLayouts())
        list << format;

    return list;
}

void AudioOutputs::setAudioOutput(const QString &audioOutput)
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

    this->d->saveProperties();
}

void AudioOutputs::setDeviceCaps(const AkAudioCaps &deviceCaps)
{
    if (this->d->m_deviceCaps == deviceCaps)
        return;

    this->d->m_deviceCaps = deviceCaps;
    this->d->saveProperties();
    emit this->deviceCapsChanged(deviceCaps);
}

bool AudioOutputs::setState(AkElement::ElementState state)
{
    if (this->d->m_audioOut) {
        if (state == AkElement::ElementStatePlaying) {
            auto device = this->d->m_audioOut->property("device").toString();
            this->d->m_audioOut->setProperty("caps", this->d->closestCaps(device).toVariant());
            this->d->m_audioOut->setProperty("latency", this->d->m_latency);
        }

        return this->d->m_audioOut->setProperty("state", state);
    }

    return false;
}

void AudioOutputs::setLatency(int latency)
{
    if (this->d->m_latency == latency)
        return;

    this->d->m_latency = latency;
    this->d->saveProperties();
    emit this->latencyChanged(latency);
}

void AudioOutputs::setVolume(qreal volume)
{
    if (qFuzzyCompare(this->d->m_volume, volume))
        return;

    this->d->m_volume = volume;
    this->d->saveProperties();
    emit this->volumeChanged(volume);
}

void AudioOutputs::resetAudioOutput()
{
    QString device;

    if (this->d->m_audioOut)
        device = this->d->m_audioOut->property("defaultOutput").toString();

    this->setAudioOutput(device);
}

void AudioOutputs::resetDeviceCaps()
{
    this->setDeviceCaps(this->preferredFormat());
}

void AudioOutputs::resetState()
{
    this->setState(AkElement::ElementStateNull);
}

void AudioOutputs::resetLatency()
{
    this->setLatency(25);
}

void AudioOutputs::resetVolume()
{
    this->setVolume(1.0);
}

void AudioOutputs::updateDevices()
{
    if (this->d->m_audioOut)
        QMetaObject::invokeMethod(this->d->m_audioOut.data(),
                                  "updateDevices");
}

AkPacket AudioOutputs::iStream(const AkPacket &packet)
{
    if (packet.caps().type() != AkCaps::CapsAudio)
        return {};

    this->d->m_mutex.lock();

    if (this->d->m_audioOut) {
        auto pkt = AkAudioPacket(packet).adjustVolume(this->d->m_volume);
        this->d->m_audioOut->iStream(pkt);
    }

    this->d->m_mutex.unlock();

    return {};
}

void AudioOutputs::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->d->m_engine == engine)
        return;

    this->d->m_engine = engine;

    if (engine)
        engine->rootContext()->setContextProperty("audioOutputs", this);
}

AudioOutputsPrivate::AudioOutputsPrivate(AudioOutputs *self):
    self(self)
{

}

void AudioOutputsPrivate::loadProperties()
{
    QSettings config;

    config.beginGroup("AudioConfigs");

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
    this->m_volume = config.value("volume", 1.0).toReal();
    QString confOutput = config.value("audioOutput").toString();

    if (self->outputs().contains(confOutput))
        self->setAudioOutput(confOutput);

    config.endGroup();
}

void AudioOutputsPrivate::saveProperties()
{
    QSettings config;
    config.beginGroup("AudioConfigs");

    config.setValue("format", int(this->m_deviceCaps.format()));
    config.setValue("layout", int(this->m_deviceCaps.layout()));
    config.setValue("rate", this->m_deviceCaps.rate());
    config.setValue("latency", this->m_latency);
    config.setValue("volume", this->m_volume);

    QString audioOutput;

    if (this->m_audioOut)
        audioOutput = this->m_audioOut->property("device").toString();

    config.setValue("audioOutput", audioOutput);
    config.endGroup();
}

AkAudioCaps AudioOutputsPrivate::closestCaps(const QString &device) const
{
    if (device.isEmpty() || !this->m_audioOut)
        return {};

    // Query supported lists from the device element.
    QList<AkAudioCaps::SampleFormat> formats;
    QList<AkAudioCaps::ChannelLayout> layouts;
    QList<int> rates;

    QMetaObject::invokeMethod(this->m_audioOut.data(), "supportedFormats",
                              Q_RETURN_ARG(QList<AkAudioCaps::SampleFormat>, formats),
                              Q_ARG(QString, device));
    QMetaObject::invokeMethod(this->m_audioOut.data(), "supportedChannelLayouts",
                              Q_RETURN_ARG(QList<AkAudioCaps::ChannelLayout>, layouts),
                              Q_ARG(QString, device));
    QMetaObject::invokeMethod(this->m_audioOut.data(), "supportedSampleRates",
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

#include "moc_audiooutputs.cpp"
