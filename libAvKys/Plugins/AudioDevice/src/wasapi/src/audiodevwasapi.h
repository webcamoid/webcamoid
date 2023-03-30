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

#ifndef AUDIODEVWASAPI_H
#define AUDIODEVWASAPI_H

#include <mmdeviceapi.h>

#include "audiodev.h"

class AudioDevWasapiPrivate;

class AudioDevWasapi: public AudioDev, public IMMNotificationClient
{
    Q_OBJECT

    public:
        AudioDevWasapi(QObject *parent=nullptr);
        ~AudioDevWasapi();

        Q_INVOKABLE QString error() const override;
        Q_INVOKABLE QString defaultInput() override;
        Q_INVOKABLE QString defaultOutput() override;
        Q_INVOKABLE QStringList inputs() override;
        Q_INVOKABLE QStringList outputs() override;
        Q_INVOKABLE QString description(const QString &device) override;
        Q_INVOKABLE AkAudioCaps preferredFormat(const QString &device) override;
        Q_INVOKABLE QList<AkAudioCaps::SampleFormat> supportedFormats(const QString &device) override;
        Q_INVOKABLE QList<AkAudioCaps::ChannelLayout> supportedChannelLayouts(const QString &device) override;
        Q_INVOKABLE QList<int> supportedSampleRates(const QString &device) override;
        Q_INVOKABLE bool init(const QString &device,
                              const AkAudioCaps &caps) override;
        Q_INVOKABLE bool init(const QString &device,
                              const AkAudioCaps &caps,
                              bool justActivate);
        Q_INVOKABLE QByteArray read() override;
        Q_INVOKABLE bool write(const AkAudioPacket &packet) override;
        Q_INVOKABLE bool uninit() override;

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();

    private:
        AudioDevWasapiPrivate *d;

        HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId,
                                                       DWORD dwNewState);
        HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId);
        HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId);
        HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow,
                                                         ERole role,
                                                         LPCWSTR pwstrDeviceId);
        HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId,
                                                         const PROPERTYKEY key);

    private slots:
        void updateDevices();
};

#endif // AUDIODEVWASAPI_H
