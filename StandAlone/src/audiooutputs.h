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

#ifndef AUDIOOUTPUTS_H
#define AUDIOOUTPUTS_H

#include <iak/akelement.h>

class AudioOutputsPrivate;
class AudioOutputs;
class AkAudioCaps;
class QQmlApplicationEngine;

using AudioOutputsPtr = QSharedPointer<AudioOutputs>;

class AudioOutputs: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString audioOutput
               READ audioOutput
               WRITE setAudioOutput
               RESET resetAudioOutput
               NOTIFY audioOutputChanged)
    Q_PROPERTY(QStringList outputs
               READ outputs
               NOTIFY outputsChanged)
    Q_PROPERTY(AkAudioCaps deviceCaps
               READ deviceCaps
               WRITE setDeviceCaps
               RESET resetDeviceCaps
               NOTIFY deviceCapsChanged)
    Q_PROPERTY(AkElement::ElementState state
               READ state
               WRITE setState
               RESET resetState
               NOTIFY stateChanged)
    Q_PROPERTY(int latency
               READ latency
               WRITE setLatency
               RESET resetLatency
               NOTIFY latencyChanged)
    Q_PROPERTY(qreal volume
               READ volume
               WRITE setVolume
               RESET resetVolume
               NOTIFY volumeChanged)
    Q_PROPERTY(AkAudioCaps preferredFormat
               READ preferredFormat
               CONSTANT)
    Q_PROPERTY(QList<AkAudioCaps::SampleFormat> supportedFormats
               READ supportedFormats
               CONSTANT)
    Q_PROPERTY(QList<AkAudioCaps::ChannelLayout> supportedChannelLayouts
               READ supportedChannelLayouts
               CONSTANT)
    Q_PROPERTY(QList<int> supportedSampleRates
               READ supportedSampleRates
               CONSTANT)
    Q_PROPERTY(QVariantList supportedFormatsVariant
               READ supportedFormatsVariant
               CONSTANT)
    Q_PROPERTY(QVariantList supportedChannelLayoutsVariant
               READ supportedChannelLayoutsVariant
               CONSTANT)

    public:
        AudioOutputs(QQmlApplicationEngine *engine=nullptr,
                   QObject *parent=nullptr);
        ~AudioOutputs();

        Q_INVOKABLE QString audioOutput() const;
        Q_INVOKABLE QStringList outputs() const;
        Q_INVOKABLE AkAudioCaps deviceCaps() const;
        Q_INVOKABLE QString description(const QString &device) const;
        Q_INVOKABLE AkElement::ElementState state() const;
        Q_INVOKABLE int latency() const;
        Q_INVOKABLE qreal volume() const;
        Q_INVOKABLE AkAudioCaps preferredFormat();
        Q_INVOKABLE QList<AkAudioCaps::SampleFormat> supportedFormats() const;
        Q_INVOKABLE QList<AkAudioCaps::ChannelLayout> supportedChannelLayouts() const;
        Q_INVOKABLE QList<int> supportedSampleRates() const;
        Q_INVOKABLE QVariantList supportedFormatsVariant() const;
        Q_INVOKABLE QVariantList supportedChannelLayoutsVariant() const;
        Q_INVOKABLE qint64 addSource(const QString &description,
                                     qreal volume = 1.0);
        Q_INVOKABLE qint64 addSource(qint64 sourceId,
                                     const QString &description,
                                     qreal volume = 1.0);
        Q_INVOKABLE QString sourceDescription(qint64 sourceId) const;
        Q_INVOKABLE bool removeSource(qint64 sourceId);
        Q_INVOKABLE void clearSources();
        Q_INVOKABLE QVariantList sources() const;
        Q_INVOKABLE qreal sourceVolume(qint64 sourceId) const;
        Q_INVOKABLE bool setSourceVolume(qint64 sourceId, qreal volume);
        Q_INVOKABLE bool sourceExists(qint64 sourceId) const;

    private:
        AudioOutputsPrivate *d;

    signals:
        void audioOutputChanged(const QString &audioOutput);
        void outputsChanged(const QStringList &outputs);
        void deviceCapsChanged(const AkAudioCaps &deviceCaps);
        void stateChanged(AkElement::ElementState state);
        void latencyChanged(int latency);
        void volumeChanged(qreal volume);
        void sourcesChanged(const QVariantList &sources);
        void sourceAdded(qint64 sourceId);
        void sourceRemoved(qint64 sourceId);

    public slots:
        void setAudioOutput(const QString &audioOutput);
        void setDeviceCaps(const AkAudioCaps &deviceCaps);
        bool setState(AkElement::ElementState state);
        void setLatency(int latency);
        void setVolume(qreal volume);
        void resetAudioOutput();
        void resetDeviceCaps();
        void resetState();
        void resetLatency();
        void resetVolume();
        void updateDevices();
        AkPacket iStream(const AkPacket &packet);
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);
};

#endif // AUDIOOUTPUTS_H
