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

#ifdef Q_OS_LINUX
#include "pulseaudio/audiodev.h"
#elif defined(Q_OS_WIN32)
#include "wasapi/audiodev.h"
#elif Q_OS_OSX
#include "coreaudio/audiodev.h"
#endif

class AudioDeviceElement: public AkElement
{
    Q_OBJECT
    Q_ENUMS(DeviceMode)
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
    Q_PROPERTY(QString mode
               READ mode
               WRITE setMode
               RESET resetMode
               NOTIFY modeChanged)

    public:
        enum DeviceMode
        {
            DeviceModeInput,
            DeviceModeOutput,
            DeviceModeDummyOutput
        };

        explicit AudioDeviceElement();
        ~AudioDeviceElement();

        Q_INVOKABLE int bufferSize() const;
        Q_INVOKABLE AkCaps caps() const;
        Q_INVOKABLE QString mode() const;

    private:
        int m_bufferSize;
        AkCaps m_caps;
        DeviceMode m_mode;
        AudioDev m_audioDevice;
        AkElementPtr m_convert;
        QThreadPool m_threadPool;
        QFuture<void> m_readFramesLoopResult;
        QMutex m_mutex;
        bool m_readFramesLoop;
        bool m_pause;

        AkAudioCaps defaultCaps(DeviceMode mode);
        static void readFramesLoop(AudioDeviceElement *self);

    signals:
        void bufferSizeChanged(int bufferSize);
        void capsChanged(const AkCaps &caps);
        void modeChanged(const QString &mode);

    public slots:
        void setBufferSize(int bufferSize);
        void setCaps(const AkCaps &caps);
        void setMode(const QString &mode);
        void resetBufferSize();
        void resetCaps();
        void resetMode();
        AkPacket iStream(const AkAudioPacket &packet);
        bool setState(AkElement::ElementState state);
};

#endif // AUDIODEVICEELEMENT_H
