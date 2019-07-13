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

#ifndef AUDIOLAYER_H
#define AUDIOLAYER_H

#include <akelement.h>
#include <akaudiocaps.h>

class AudioLayerPrivate;
class AudioLayer;
class AkAudioCaps;
class QQmlApplicationEngine;

using AudioLayerPtr = QSharedPointer<AudioLayer>;

class AudioLayer: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList audioInput
               READ audioInput
               WRITE setAudioInput
               RESET resetAudioInput
               NOTIFY audioInputChanged)
    Q_PROPERTY(QString audioOutput
               READ audioOutput
               WRITE setAudioOutput
               RESET resetAudioOutput
               NOTIFY audioOutputChanged)
    Q_PROPERTY(QStringList inputs
               READ inputs
               NOTIFY inputsChanged)
    Q_PROPERTY(QStringList outputs
               READ outputs
               NOTIFY outputsChanged)
    Q_PROPERTY(AkAudioCaps inputCaps
               READ inputCaps
               WRITE setInputCaps
               RESET resetInputCaps
               NOTIFY inputCapsChanged)
    Q_PROPERTY(AkAudioCaps outputCaps
               READ outputCaps
               NOTIFY outputCapsChanged)
    Q_PROPERTY(AkAudioCaps inputDeviceCaps
               READ inputDeviceCaps
               WRITE setInputDeviceCaps
               RESET resetInputDeviceCaps
               NOTIFY inputDeviceCapsChanged)
    Q_PROPERTY(AkAudioCaps outputDeviceCaps
               READ outputDeviceCaps
               WRITE setOutputDeviceCaps
               RESET resetOutputDeviceCaps
               NOTIFY outputDeviceCapsChanged)
    Q_PROPERTY(QString inputDescription
               READ inputDescription
               WRITE setInputDescription
               RESET resetInputDescription
               NOTIFY inputDescriptionChanged)
    Q_PROPERTY(AkElement::ElementState outputState
               READ outputState
               WRITE setOutputState
               RESET resetOutputState
               NOTIFY outputStateChanged)
    Q_PROPERTY(AkElement::ElementState inputState
               READ inputState
               WRITE setInputState
               RESET resetInputState
               NOTIFY inputStateChanged)
    Q_PROPERTY(int inputLatency
               READ inputLatency
               WRITE setInputLatency
               RESET resetInputLatency
               NOTIFY inputLatencyChanged)
    Q_PROPERTY(int outputLatency
               READ outputLatency
               WRITE setOutputLatency
               RESET resetOutputLatency
               NOTIFY outputLatencyChanged)

    public:
        AudioLayer(QQmlApplicationEngine *engine=nullptr,
                   QObject *parent=nullptr);
        ~AudioLayer();

        Q_INVOKABLE QStringList audioInput() const;
        Q_INVOKABLE QString audioOutput() const;
        Q_INVOKABLE QStringList inputs() const;
        Q_INVOKABLE QStringList outputs() const;
        Q_INVOKABLE AkAudioCaps inputCaps() const;
        Q_INVOKABLE AkAudioCaps outputCaps() const;
        Q_INVOKABLE AkAudioCaps inputDeviceCaps() const;
        Q_INVOKABLE AkAudioCaps outputDeviceCaps() const;
        Q_INVOKABLE QString inputDescription() const;
        Q_INVOKABLE QString description(const QString &device) const;
        Q_INVOKABLE AkElement::ElementState inputState() const;
        Q_INVOKABLE AkElement::ElementState outputState() const;
        Q_INVOKABLE int inputLatency() const;
        Q_INVOKABLE int outputLatency() const;
        Q_INVOKABLE AkAudioCaps preferredFormat(const QString &device);
        Q_INVOKABLE QList<AkAudioCaps::SampleFormat> supportedFormats(const QString &device) const;
        Q_INVOKABLE QList<AkAudioCaps::ChannelLayout> supportedChannelLayouts(const QString &device) const;
        Q_INVOKABLE QList<int> supportedSampleRates(const QString &device) const;

    private:
        AudioLayerPrivate *d;

    signals:
        void audioInputChanged(const QStringList &audioInput);
        void audioOutputChanged(const QString &audioOutput);
        void inputsChanged(const QStringList &inputs);
        void outputsChanged(const QStringList &outputs);
        void inputCapsChanged(const AkAudioCaps &inputCaps);
        void outputCapsChanged(const AkAudioCaps &outputCaps);
        void inputDeviceCapsChanged(const AkAudioCaps &inputDeviceCaps);
        void outputDeviceCapsChanged(const AkAudioCaps &outputDeviceCaps);
        void inputDescriptionChanged(const QString &inputDescription);
        void inputStateChanged(AkElement::ElementState inputState);
        void outputStateChanged(AkElement::ElementState outputState);
        void inputLatencyChanged(int inputLatency);
        void outputLatencyChanged(int outputLatency);
        void oStream(const AkPacket &packet);

    public slots:
        void setAudioInput(const QStringList &audioInput);
        void setAudioOutput(const QString &audioOutput);
        void setInputCaps(const AkAudioCaps &inputCaps);
        void setInputDeviceCaps(const AkAudioCaps &inputDeviceCaps);
        void setOutputDeviceCaps(const AkAudioCaps &outputDeviceCaps);
        void setInputDescription(const QString &inputDescription);
        void setInputState(AkElement::ElementState inputState);
        bool setOutputState(AkElement::ElementState outputState);
        void setInputLatency(int inputLatency);
        void setOutputLatency(int outputLatency);
        void resetAudioInput();
        void resetAudioOutput();
        void resetInputCaps();
        void resetInputDeviceCaps();
        void resetOutputDeviceCaps();
        void resetInputDescription();
        void resetInputState();
        void resetOutputState();
        void resetInputLatency();
        void resetOutputLatency();
        AkPacket iStream(const AkPacket &packet);
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);

    private slots:
        void privInputsChanged(const QStringList &inputs);
        void updateInputState();
        void updateOutputState();
        void loadProperties();
        void saveAudioInput(const QStringList &audioInput);
        void saveAudioOutput(const QString &audioOutput);
        void saveAudioDeviceAudioLib(const QString &audioLib);
        void saveAudioConvertConvertLib(const QString &convertLib);
        void saveProperties();
};

#endif // AUDIOLAYER_H
