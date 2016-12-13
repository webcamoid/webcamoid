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

#ifndef AUDIODEVWASAPI_H
#define AUDIODEVWASAPI_H

#include <akaudiocaps.h>
#include <objbase.h>
#include <initguid.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <propkeydef.h>

#include "audiodev.h"

class AudioDevWasapi: public AudioDev, public IMMNotificationClient
{
    Q_OBJECT

    public:
        explicit AudioDevWasapi(QObject *parent=NULL);
        ~AudioDevWasapi();

        Q_INVOKABLE QString error() const;
        Q_INVOKABLE QString defaultInput();
        Q_INVOKABLE QString defaultOutput();
        Q_INVOKABLE QStringList inputs();
        Q_INVOKABLE QStringList outputs();
        Q_INVOKABLE QString description(const QString &device);
        Q_INVOKABLE AkAudioCaps preferredFormat(const QString &device);
        Q_INVOKABLE bool init(const QString &device,
                              const AkAudioCaps &caps);
        Q_INVOKABLE bool init(const QString &device,
                              const AkAudioCaps &caps,
                              bool justActivate);
        Q_INVOKABLE QByteArray read(int samples);
        Q_INVOKABLE bool write(const AkAudioPacket &packet);
        Q_INVOKABLE bool uninit();

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();

    private:
        QString m_error;
        QStringList m_inputs;
        QStringList m_outputs;
        QByteArray m_audioBuffer;
        IMMDeviceEnumerator *m_deviceEnumerator;
        IMMDevice *m_pDevice;
        IAudioClient *m_pAudioClient;
        IAudioCaptureClient *m_pCaptureClient;
        IAudioRenderClient *m_pRenderClient;
        HANDLE m_hEvent;
        ULONG m_cRef;
        AkAudioCaps m_curCaps;

        QString defaultDevice(EDataFlow dataFlow);
        QStringList listDevices(EDataFlow dataFlow);

        HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId,
                                                       DWORD dwNewState);
        HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId);
        HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId);
        HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow,
                                                         ERole role,
                                                         LPCWSTR pwstrDeviceId);
        HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId,
                                                         const PROPERTYKEY key);
};

#endif // AUDIODEVWASAPI_H
