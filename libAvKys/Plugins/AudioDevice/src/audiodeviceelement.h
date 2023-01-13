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

#ifndef AUDIODEVICEELEMENT_H
#define AUDIODEVICEELEMENT_H

#include <akelement.h>
#include <akaudiocaps.h>

class AudioDeviceElementPrivate;

class AudioDeviceElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QString defaultInput
               READ defaultInput
               NOTIFY defaultInputChanged)
    Q_PROPERTY(QString defaultOutput
               READ defaultOutput
               NOTIFY defaultOutputChanged)
    Q_PROPERTY(QStringList inputs
               READ inputs
               NOTIFY inputsChanged)
    Q_PROPERTY(QStringList outputs
               READ outputs
               NOTIFY outputsChanged)
    Q_PROPERTY(QString device
               READ device
               WRITE setDevice
               RESET resetDevice
               NOTIFY deviceChanged)
    // In milliseconds
    Q_PROPERTY(int latency
               READ latency
               WRITE setLatency
               RESET resetLatency
               NOTIFY latencyChanged)
    Q_PROPERTY(AkAudioCaps caps
               READ caps
               WRITE setCaps
               RESET resetCaps
               NOTIFY capsChanged)

    public:
        AudioDeviceElement();
        ~AudioDeviceElement();

        Q_INVOKABLE QString defaultInput();
        Q_INVOKABLE QString defaultOutput();
        Q_INVOKABLE QStringList inputs();
        Q_INVOKABLE QStringList outputs();
        Q_INVOKABLE QString description(const QString &device);
        Q_INVOKABLE QString device() const;
        Q_INVOKABLE int latency() const;
        Q_INVOKABLE AkAudioCaps caps() const;
        Q_INVOKABLE AkAudioCaps preferredFormat(const QString &device);
        Q_INVOKABLE QList<AkAudioCaps::SampleFormat> supportedFormats(const QString &device);
        Q_INVOKABLE QList<AkAudioCaps::ChannelLayout> supportedChannelLayouts(const QString &device);
        Q_INVOKABLE QList<int> supportedSampleRates(const QString &device);

    private:
        AudioDeviceElementPrivate *d;

    protected:
        AkPacket iAudioStream(const AkAudioPacket &packet) override;

    signals:
        void defaultInputChanged(const QString &defaultInput);
        void defaultOutputChanged(const QString &defaultOutput);
        void inputsChanged(const QStringList &inputs);
        void outputsChanged(const QStringList &outputs);
        void deviceChanged(const QString &device);
        void latencyChanged(int latency);
        void capsChanged(const AkAudioCaps &caps);

    public slots:
        void setDevice(const QString &device);
        void setLatency(int latency);
        void setCaps(const AkAudioCaps &caps);
        void resetDevice();
        void resetLatency();
        void resetCaps();
        bool setState(AkElement::ElementState state) override;
};

#endif // AUDIODEVICEELEMENT_H
