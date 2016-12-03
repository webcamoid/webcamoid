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

#ifndef AUDIODEVICEELEMENT_H
#define AUDIODEVICEELEMENT_H

#include <QTimer>
#include <QThreadPool>
#include <QtConcurrent>
#include <ak.h>

#include "audiodev.h"

typedef QSharedPointer<AudioDev> AudioDevPtr;

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
    // Buffer size in samples.
    Q_PROPERTY(int bufferSize
               READ bufferSize
               WRITE setBufferSize
               RESET resetBufferSize
               NOTIFY bufferSizeChanged)
    Q_PROPERTY(AkCaps caps
               READ caps
               WRITE setCaps
               RESET resetCaps
               NOTIFY capsChanged)
    Q_PROPERTY(QString audioLib
               READ audioLib
               WRITE setAudioLib
               RESET resetAudioLib
               NOTIFY audioLibChanged)

    public:
        explicit AudioDeviceElement();
        ~AudioDeviceElement();

        Q_INVOKABLE QString defaultInput();
        Q_INVOKABLE QString defaultOutput();
        Q_INVOKABLE QStringList inputs();
        Q_INVOKABLE QStringList outputs();
        Q_INVOKABLE QString description(const QString &device);
        Q_INVOKABLE QString device() const;
        Q_INVOKABLE int bufferSize() const;
        Q_INVOKABLE AkCaps caps() const;
        Q_INVOKABLE QString audioLib() const;

    private:
        QString m_device;
        int m_bufferSize;
        AkCaps m_caps;
        AudioDevPtr m_audioDevice;
        AkElementPtr m_convert;
        QThreadPool m_threadPool;
        QFuture<void> m_readFramesLoopResult;
        QMutex m_mutex;
        QMutex m_mutexLib;
        bool m_readFramesLoop;
        bool m_pause;

        static void readFramesLoop(AudioDeviceElement *self);

    signals:
        void defaultInputChanged(const QString &defaultInput);
        void defaultOutputChanged(const QString &defaultOutput);
        void inputsChanged(const QStringList &inputs);
        void outputsChanged(const QStringList &outputs);
        void deviceChanged(const QString &device);
        void bufferSizeChanged(int bufferSize);
        void capsChanged(const AkCaps &caps);
        void audioLibChanged(const QString &audioLib);

    public slots:
        void setDevice(const QString &device);
        void setBufferSize(int bufferSize);
        void setCaps(const AkCaps &caps);
        void setAudioLib(const QString &audioLib);
        void resetDevice();
        void resetBufferSize();
        void resetCaps();
        void resetAudioLib();
        AkPacket iStream(const AkAudioPacket &packet);
        bool setState(AkElement::ElementState state);

    private slots:
        void audioLibUpdated(const QString &audioLib);
};

#endif // AUDIODEVICEELEMENT_H
