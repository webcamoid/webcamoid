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

#ifndef AUDIOINPUTS_H
#define AUDIOINPUTS_H

#include <akaudiocaps.h>
#include <iak/akelement.h>

class AudioInputsPrivate;
class AudioInputs;
class AkAudioCaps;
class QQmlApplicationEngine;

using AudioInputsPtr = QSharedPointer<AudioInputs>;

class AudioInputs: public QObject
{
    Q_OBJECT
    // List of active source device IDs in the mixer.
    Q_PROPERTY(QStringList activeInputs
               READ activeInputs
               NOTIFY activeInputsChanged)
    // All available input device IDs (hardware devices and video source).
    Q_PROPERTY(QStringList inputs
               READ inputs
               NOTIFY inputsChanged)
    Q_PROPERTY(AkElement::ElementState inputState
               READ inputState
               WRITE setInputState
               RESET resetInputState
               NOTIFY inputStateChanged)
    Q_PROPERTY(AkElement::ElementState outputState
               READ outputState
               WRITE setOutputState
               RESET resetOutputState
               NOTIFY outputStateChanged)
    Q_PROPERTY(AkAudioCaps deviceCaps
               READ deviceCaps
               WRITE setDeviceCaps
               RESET resetDeviceCaps
               NOTIFY deviceCapsChanged)
    Q_PROPERTY(int latency
               READ latency
               WRITE setLatency
               RESET resetLatency
               NOTIFY latencyChanged)
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
        AudioInputs(QQmlApplicationEngine *engine=nullptr,
                    QObject *parent=nullptr);
        ~AudioInputs();

        // Active sources in the mixer.
        Q_INVOKABLE QStringList activeInputs() const;

        // All available inputs.
        Q_INVOKABLE QStringList inputs() const;

        Q_INVOKABLE QString description(const QString &device) const;
        Q_INVOKABLE AkAudioCaps deviceCaps() const;
        Q_INVOKABLE AkElement::ElementState inputState() const;
        Q_INVOKABLE AkElement::ElementState outputState() const;
        Q_INVOKABLE int latency() const;

        // Per-source volume (0.0 – 1.0).
        Q_INVOKABLE qreal volume(const QString &device) const;
        Q_INVOKABLE bool setVolume(const QString &device, qreal volume);

        Q_INVOKABLE AkAudioCaps preferredFormat() const;
        Q_INVOKABLE QList<AkAudioCaps::SampleFormat> supportedFormats() const;
        Q_INVOKABLE QList<AkAudioCaps::ChannelLayout> supportedChannelLayouts() const;
        Q_INVOKABLE QList<int> supportedSampleRates() const;
        Q_INVOKABLE QVariantList supportedFormatsVariant() const;
        Q_INVOKABLE QVariantList supportedChannelLayoutsVariant() const;

    private:
        AudioInputsPrivate *d;

    signals:
        void activeInputsChanged(const QStringList &activeInputs);
        void inputsChanged(const QStringList &inputs);
        void deviceCapsChanged(const AkAudioCaps &deviceCaps);
        void inputStateChanged(AkElement::ElementState inputState);
        void outputStateChanged(AkElement::ElementState outputState);
        void latencyChanged(int latency);
        void oStream(const AkPacket &packet);
        void vumeter(const QString &input, qreal level);

    public slots:
        // Add / remove sources from the mixer.
        qint64 addInput(const QString &device, const QString &description);
        bool addInput(const QString &device);
        bool removeInput(const QString &device);
        void setDeviceCaps(const AkAudioCaps &deviceCaps);
        void resetDeviceCaps();
        void setInputState(AkElement::ElementState state);
        void setOutputState(AkElement::ElementState state);
        void setLatency(int latency);
        void resetActiveInputs();
        void resetInputState();
        void resetOutputState();
        void resetLatency();
        void updateDevices();
        AkPacket iStream(const AkPacket &packet);
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);

    private slots:
        void sendPacket(const AkPacket &packet);
        void privInputsChanged(const QStringList &inputs);
};

#endif // AUDIOINPUTS_H
