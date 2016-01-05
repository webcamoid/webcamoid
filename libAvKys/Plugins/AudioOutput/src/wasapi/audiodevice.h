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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H

#include <akaudiocaps.h>
#include <objbase.h>
#include <initguid.h>
#include <audioclient.h>
#include <mmdeviceapi.h>

class AudioDevice: public QObject
{
    Q_OBJECT
    Q_ENUMS(DeviceMode)
    Q_PROPERTY(QString error
               READ error
               NOTIFY errorChanged)

    public:
        enum DeviceMode
        {
            DeviceModeCapture,
            DeviceModePlayback
        };

        explicit AudioDevice(QObject *parent=NULL);
        ~AudioDevice();

        Q_INVOKABLE QString error() const;
        Q_INVOKABLE bool preferredFormat(DeviceMode mode,
                                         AkAudioCaps::SampleFormat *sampleFormat,
                                         int *channels,
                                         int *sampleRate);
        Q_INVOKABLE bool init(DeviceMode mode,
                              AkAudioCaps::SampleFormat sampleFormat,
                              int channels,
                              int sampleRate,
                              bool justActivate=false);
        Q_INVOKABLE QByteArray read(int samples);
        Q_INVOKABLE bool write(const QByteArray &frame);
        Q_INVOKABLE bool uninit();

    private:
        QString m_error;
        QByteArray m_audioBuffer;
        IMMDeviceEnumerator *m_pEnumerator;
        IMMDevice *m_pDevice;
        IAudioClient *m_pAudioClient;
        IAudioCaptureClient *m_pCaptureClient;
        IAudioRenderClient *m_pRenderClient;
        HANDLE m_hEvent;
        int m_curBps;
        int m_curChannels;

    signals:
        void errorChanged(const QString &error);
};

#endif // AUDIODEVICE_H
