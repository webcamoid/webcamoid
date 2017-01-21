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

#ifndef AUDIOLAYER_H
#define AUDIOLAYER_H

#include <QMutex>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <akelement.h>

class AudioLayer;

typedef QSharedPointer<AudioLayer> AudioLayerPtr;

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
    Q_PROPERTY(AkCaps inputCaps
               READ inputCaps
               WRITE setInputCaps
               RESET resetInputCaps
               NOTIFY inputCapsChanged)
    Q_PROPERTY(AkCaps outputCaps
               READ outputCaps
               NOTIFY outputCapsChanged)
    Q_PROPERTY(AkCaps inputDeviceCaps
               READ inputDeviceCaps
               WRITE setInputDeviceCaps
               RESET resetInputDeviceCaps
               NOTIFY inputDeviceCapsChanged)
    Q_PROPERTY(AkCaps outputDeviceCaps
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

    public:
        explicit AudioLayer(QQmlApplicationEngine *engine=NULL, QObject *parent=NULL);
        ~AudioLayer();

        Q_INVOKABLE QStringList audioInput() const;
        Q_INVOKABLE QString audioOutput() const;
        Q_INVOKABLE QStringList inputs() const;
        Q_INVOKABLE QStringList outputs() const;
        Q_INVOKABLE AkCaps inputCaps() const;
        Q_INVOKABLE AkCaps outputCaps() const;
        Q_INVOKABLE AkCaps inputDeviceCaps() const;
        Q_INVOKABLE AkCaps outputDeviceCaps() const;
        Q_INVOKABLE QString inputDescription() const;
        Q_INVOKABLE QString description(const QString &device) const;
        Q_INVOKABLE AkElement::ElementState inputState() const;
        Q_INVOKABLE AkElement::ElementState outputState() const;
        Q_INVOKABLE AkAudioCaps preferredFormat(const QString &device);
        Q_INVOKABLE QStringList supportedFormats(const QString &device);
        Q_INVOKABLE QList<int> supportedChannels(const QString &device);
        Q_INVOKABLE QList<int> supportedSampleRates(const QString &device);

    private:
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

    signals:
        void audioInputChanged(const QStringList &audioInput);
        void audioOutputChanged(const QString &audioOutput);
        void inputsChanged(const QStringList &inputs);
        void outputsChanged(const QStringList &outputs);
        void inputCapsChanged(const AkCaps &inputCaps);
        void outputCapsChanged(const AkCaps &outputCaps);
        void inputDeviceCapsChanged(const AkCaps &inputDeviceCaps);
        void outputDeviceCapsChanged(const AkCaps &outputDeviceCaps);
        void inputDescriptionChanged(const QString &inputDescription);
        void inputStateChanged(AkElement::ElementState inputState);
        void outputStateChanged(AkElement::ElementState outputState);
        void oStream(const AkPacket &packet);

    public slots:
        void setAudioInput(const QStringList &audioInput);
        void setAudioOutput(const QString &audioOutput);
        void setInputCaps(const AkCaps &inputCaps);
        void setInputDeviceCaps(const AkCaps &inputDeviceCaps);
        void setOutputDeviceCaps(const AkCaps &outputDeviceCaps);
        void setInputDescription(const QString &inputDescription);
        void setInputState(AkElement::ElementState inputState);
        bool setOutputState(AkElement::ElementState outputState);
        void resetAudioInput();
        void resetAudioOutput();
        void resetInputCaps();
        void resetInputDeviceCaps();
        void resetOutputDeviceCaps();
        void resetInputDescription();
        void resetInputState();
        void resetOutputState();
        AkPacket iStream(const AkPacket &packet);
        void setQmlEngine(QQmlApplicationEngine *engine=NULL);

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
