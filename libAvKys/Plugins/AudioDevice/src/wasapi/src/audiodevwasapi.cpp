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

#include <QMap>
#include <QVector>
#include <akaudiopacket.h>
#include <audioclient.h>
#include <objbase.h>
#include <initguid.h>
#include <propkeydef.h>

#include "audiodevwasapi.h"

DEFINE_PROPERTYKEY(PKEY_Device_FriendlyName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 14);

#define MAX_ERRORS_READ_WRITE 5
#define EVENT_TIMEOUT 1000

using ErrorCodesMap = QMap<HRESULT, QString>;

inline ErrorCodesMap initErrorCodesMap()
{
    ErrorCodesMap errorCodes = {
        // COM library errors.
        {REGDB_E_CLASSNOTREG  , "REGDB_E_CLASSNOTREG"  },
        {CLASS_E_NOAGGREGATION, "CLASS_E_NOAGGREGATION"},
        {E_NOINTERFACE        , "E_NOINTERFACE"        },
        {E_POINTER            , "E_POINTER"            },

        // IMMDeviceEnumerator errors.
        {E_INVALIDARG , "E_INVALIDARG" },
        {E_NOTFOUND   , "E_NOTFOUND"   },
        {E_OUTOFMEMORY, "E_OUTOFMEMORY"},

        // IAudioClient errors.
        {AUDCLNT_E_ALREADY_INITIALIZED         , "AUDCLNT_E_ALREADY_INITIALIZED"         },
        {AUDCLNT_E_WRONG_ENDPOINT_TYPE         , "AUDCLNT_E_WRONG_ENDPOINT_TYPE"         },
        {AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED     , "AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED"     },
        {AUDCLNT_E_BUFFER_SIZE_ERROR           , "AUDCLNT_E_BUFFER_SIZE_ERROR"           },
        {AUDCLNT_E_CPUUSAGE_EXCEEDED           , "AUDCLNT_E_CPUUSAGE_EXCEEDED"           },
        {AUDCLNT_E_DEVICE_INVALIDATED          , "AUDCLNT_E_DEVICE_INVALIDATED"          },
        {AUDCLNT_E_DEVICE_IN_USE               , "AUDCLNT_E_DEVICE_IN_USE"               },
        {AUDCLNT_E_ENDPOINT_CREATE_FAILED      , "AUDCLNT_E_ENDPOINT_CREATE_FAILED"      },
        {AUDCLNT_E_INVALID_DEVICE_PERIOD       , "AUDCLNT_E_INVALID_DEVICE_PERIOD"       },
        {AUDCLNT_E_UNSUPPORTED_FORMAT          , "AUDCLNT_E_UNSUPPORTED_FORMAT"          },
        {AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED  , "AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED"  },
        {AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL, "AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL"},
        {AUDCLNT_E_SERVICE_NOT_RUNNING         , "AUDCLNT_E_SERVICE_NOT_RUNNING"         },
        {AUDCLNT_E_NOT_INITIALIZED             , "AUDCLNT_E_NOT_INITIALIZED"             },
        {AUDCLNT_E_NOT_STOPPED                 , "AUDCLNT_E_NOT_STOPPED"                 },
        {AUDCLNT_E_EVENTHANDLE_NOT_SET         , "AUDCLNT_E_EVENTHANDLE_NOT_SET"         },
        {AUDCLNT_E_INVALID_SIZE                , "AUDCLNT_E_INVALID_SIZE"                },
        {AUDCLNT_E_OUT_OF_ORDER                , "AUDCLNT_E_OUT_OF_ORDER"                },
        {AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED    , "AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED"    },
        {AUDCLNT_E_BUFFER_ERROR                , "AUDCLNT_E_BUFFER_ERROR"                },
        {AUDCLNT_E_BUFFER_TOO_LARGE            , "AUDCLNT_E_BUFFER_TOO_LARGE"            },
        {AUDCLNT_E_BUFFER_OPERATION_PENDING    , "AUDCLNT_E_BUFFER_OPERATION_PENDING"    }
    };

    return errorCodes;
}

Q_GLOBAL_STATIC_WITH_ARGS(ErrorCodesMap, errorCodes, (initErrorCodesMap()))

class AudioDevWasapiPrivate
{
    public:
        AudioDevWasapi *self;
        QString m_error;
        QStringList m_sources;
        QStringList m_sinks;
        QString m_defaultSink;
        QString m_defaultSource;
        QMap<QString, QString> m_descriptionMap;
        QMap<QString, QList<AkAudioCaps::SampleFormat>> m_supportedFormats;
        QMap<QString, QList<AkAudioCaps::ChannelLayout>> m_supportedLayouts;
        QMap<QString, QList<int>> m_supportedSampleRates;
        QMap<QString, AkAudioCaps> m_preferredInputCaps;
        QMap<QString, AkAudioCaps> m_preferredOutputCaps;
        QByteArray m_audioBuffer;
        AkAudioCaps m_curCaps;
        QString m_curDevice;
        IMMDeviceEnumerator *m_deviceEnumerator {nullptr};
        IMMDevice *m_pDevice {nullptr};
        IAudioClient *m_pAudioClient {nullptr};
        IAudioCaptureClient *m_pCaptureClient {nullptr};
        IAudioRenderClient *m_pRenderClient {nullptr};
        HANDLE m_hEvent {nullptr};
        ULONG m_cRef {1};
        int m_samples {0};

        explicit AudioDevWasapiPrivate(AudioDevWasapi *self);
        bool waveFormatFromCaps(WAVEFORMATEX *wfx,
                                const AkAudioCaps &caps) const;
        AkAudioCaps capsFromWaveFormat(WAVEFORMATEX *wfx) const;
        void fillDeviceInfo(const QString &device,
                            QList<AkAudioCaps::SampleFormat> *supportedFormats,
                            QList<AkAudioCaps::ChannelLayout> *supportedLayouts,
                            QList<int> *supportedSampleRates) const;
        AkAudioCaps preferredCaps(const QString &device,
                                  EDataFlow dataFlow) const;
};

AudioDevWasapi::AudioDevWasapi(QObject *parent):
    AudioDev(parent)
{
    this->d = new AudioDevWasapiPrivate(this);

    // Create DeviceEnumerator
    HRESULT hr;

    // Get device enumerator.
    if (FAILED(hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),
                                     nullptr,
                                     CLSCTX_ALL,
                                     __uuidof(IMMDeviceEnumerator),
                                     reinterpret_cast<void **>(&this->d->m_deviceEnumerator)))) {
        return;
    }

    if (FAILED(hr = this->d->m_deviceEnumerator->RegisterEndpointNotificationCallback(this))) {
        this->d->m_deviceEnumerator->Release();
        this->d->m_deviceEnumerator = nullptr;

        return;
    }

    this->updateDevices();
}

AudioDevWasapi::~AudioDevWasapi()
{
    this->uninit();
    this->d->m_deviceEnumerator->UnregisterEndpointNotificationCallback(this);
    this->d->m_deviceEnumerator->Release();
    delete this->d;
}

QString AudioDevWasapi::error() const
{
    return this->d->m_error;
}

QString AudioDevWasapi::defaultInput()
{
    return this->d->m_defaultSource;
}

QString AudioDevWasapi::defaultOutput()
{
    return this->d->m_defaultSink;
}

QStringList AudioDevWasapi::inputs()
{
    return this->d->m_sources;
}

QStringList AudioDevWasapi::outputs()
{
    return this->d->m_sinks;
}

QString AudioDevWasapi::description(const QString &device)
{
    return this->d->m_descriptionMap.value(device);
}

// Get native format for the default audio device.
AkAudioCaps AudioDevWasapi::preferredFormat(const QString &device)
{
    if (this->d->m_preferredOutputCaps.contains(device))
        return this->d->m_preferredOutputCaps[device];

    if (this->d->m_preferredInputCaps.contains(device))
        return this->d->m_preferredInputCaps[device];

    return AkAudioCaps();
}

QList<AkAudioCaps::SampleFormat> AudioDevWasapi::supportedFormats(const QString &device)
{
    return this->d->m_supportedFormats.value(device);
}

QList<AkAudioCaps::ChannelLayout> AudioDevWasapi::supportedChannelLayouts(const QString &device)
{
    return this->d->m_supportedLayouts.value(device);
}

QList<int> AudioDevWasapi::supportedSampleRates(const QString &device)
{
    return this->d->m_supportedSampleRates.value(device);
}

bool AudioDevWasapi::init(const QString &device, const AkAudioCaps &caps)
{
    return this->init(device, caps, false);
}

bool AudioDevWasapi::init(const QString &device,
                          const AkAudioCaps &caps,
                          bool justActivate)
{
    if (!this->d->m_deviceEnumerator) {
        this->d->m_error = "Device enumerator not created.";
        emit this->errorChanged(this->d->m_error);

        return false;
    }

    // Clear audio buffer.
    this->d->m_audioBuffer.clear();

    HRESULT hr;

    // Get audio device.
    if (FAILED(hr = this->d->m_deviceEnumerator->GetDevice(device.toStdWString().c_str(),
                                                           &this->d->m_pDevice))) {
        this->d->m_error = "GetDevice: " + errorCodes->value(hr);
        emit this->errorChanged(this->d->m_error);
        this->uninit();

        return false;
    }

    // Get an instance for the audio client.
    if (FAILED(hr = this->d->m_pDevice->Activate(__uuidof(IAudioClient),
                                                 CLSCTX_ALL,
                                                 nullptr,
                                                 reinterpret_cast<void **>(&this->d->m_pAudioClient)))) {
        this->d->m_error = "Activate: " + errorCodes->value(hr);
        emit this->errorChanged(this->d->m_error);
        this->uninit();

        return false;
    }

    // Just get the audio client instance.
    if (justActivate)
        return true;

    // Get the minimum size of the buffer in 100-nanosecond units,
    // this means you must do:
    //
    // bufferSize(seconds) = 100e-9 * hnsRequestedDuration
    //
    // to get the size of the buffer in seconds.
    //
    REFERENCE_TIME hnsRequestedDuration;
    this->d->m_pAudioClient->GetDevicePeriod(nullptr, &hnsRequestedDuration);

    // Accumulate a minimum of 1 sec. of audio in the buffer.
    REFERENCE_TIME minDuration = 10e6;

    if (hnsRequestedDuration < minDuration)
        hnsRequestedDuration = minDuration;

    // Set audio device format.
    WAVEFORMATEX wfx;
    WAVEFORMATEX *ptrWfx = &wfx;
    WAVEFORMATEX *closestWfx = nullptr;
    this->d->waveFormatFromCaps(&wfx, caps);

    if (FAILED(this->d->m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED,
                                                          &wfx,
                                                          &closestWfx))) {
        this->uninit();

        return false;
    }

    if (closestWfx)
        ptrWfx = closestWfx;

    this->d->m_curCaps = this->d->capsFromWaveFormat(ptrWfx);

    if (FAILED(hr = this->d->m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                                        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                                        hnsRequestedDuration,
                                                        hnsRequestedDuration,
                                                        ptrWfx,
                                                        nullptr))) {

        if (closestWfx)
            CoTaskMemFree(closestWfx);

        this->d->m_error = "Initialize: " + errorCodes->value(hr);
        emit this->errorChanged(this->d->m_error);
        this->uninit();

        return false;
    }

    if (closestWfx)
        CoTaskMemFree(closestWfx);

    // Create an event handler for checking when an aundio frame is required
    // for reading or writing.
    this->d->m_hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    if (!this->d->m_hEvent) {
        this->d->m_error = "CreateEvent: Error creating event handler";
        emit this->errorChanged(this->d->m_error);
        this->uninit();

        return false;
    }

    // Set event handler.
    if (FAILED(this->d->m_pAudioClient->SetEventHandle(this->d->m_hEvent))) {
        this->d->m_error = "SetEventHandle: " + errorCodes->value(hr);
        emit this->errorChanged(this->d->m_error);
        this->uninit();

        return false;
    }

    // Get audio capture/render client.
    if (this->inputs().contains(device))
        hr = this->d->m_pAudioClient->GetService(__uuidof(IAudioCaptureClient),
                                                 reinterpret_cast<void **>(&this->d->m_pCaptureClient));
    else
        hr = this->d->m_pAudioClient->GetService(__uuidof(IAudioRenderClient),
                                                 reinterpret_cast<void **>(&this->d->m_pRenderClient));

    if (FAILED(hr)) {
        this->d->m_error = "GetService: " + errorCodes->value(hr);
        emit this->errorChanged(this->d->m_error);
        this->uninit();

        return false;
    }

    // Start audio client.
    if (FAILED(hr = this->d->m_pAudioClient->Start())) {
        this->d->m_error = "Start: " + errorCodes->value(hr);
        emit this->errorChanged(this->d->m_error);
        this->uninit();

        return false;
    }

    this->d->m_curDevice = device;
    this->d->m_samples = qMax(this->latency() * caps.rate() / 1000, 1);

    return true;
}

QByteArray AudioDevWasapi::read()
{
    int bufferSize = this->d->m_samples
                     * this->d->m_curCaps.bps()
                     * this->d->m_curCaps.channels()
                     / 8;

    int nErrors = 0;

    // Read audio samples until audio buffer is full.
    while (this->d->m_audioBuffer.size() < bufferSize
           && nErrors < MAX_ERRORS_READ_WRITE) {
        // Wait until an audio frame can be read.
        if (WaitForSingleObject(this->d->m_hEvent, EVENT_TIMEOUT) != WAIT_OBJECT_0) {
            nErrors++;

            continue;
        }

        HRESULT hr;
        UINT32 samplesCount = 0;

        // Get the size in samples of the captured audio frame.
        if (FAILED(hr = this->d->m_pCaptureClient->GetNextPacketSize(&samplesCount))) {
            this->d->m_error = "GetNextPacketSize: " + errorCodes->value(hr);
            emit this->errorChanged(this->d->m_error);

            return QByteArray();
        }

        // Check if empty.
        if (samplesCount < 1)
            continue;

        BYTE *pData = nullptr;
        DWORD flags = 0;

        // Read audio buffer.
        if (FAILED(hr = this->d->m_pCaptureClient->GetBuffer(&pData,
                                                             &samplesCount,
                                                             &flags,
                                                             nullptr,
                                                             nullptr))) {
            this->d->m_error = "GetBuffer: " + errorCodes->value(hr);
            emit this->errorChanged(this->d->m_error);

            return QByteArray();
        }

        size_t bufferSize =
                samplesCount
                * size_t(this->d->m_curCaps.bps()
                         * this->d->m_curCaps.channels())
                / 8;

        // This flag means we must ignore the incoming buffer and write zeros
        // to it.
        if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
            pData = new BYTE[bufferSize];
            memset(pData, 0, bufferSize);
        }

        // Copy audio frame to the audio buffer.
        QByteArray buffer(reinterpret_cast<const char *>(pData), int(bufferSize));

        if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
            delete [] pData;

        this->d->m_audioBuffer.append(buffer);

        // Remove read samples from the audio device.
        if (FAILED(hr = this->d->m_pCaptureClient->ReleaseBuffer(samplesCount))) {
            this->d->m_error = "ReleaseBuffer: " + errorCodes->value(hr);
            emit this->errorChanged(this->d->m_error);

            return QByteArray();
        }
    }

    // In case of error and if the buffer is empty, return.
    if (this->d->m_audioBuffer.isEmpty())
        return QByteArray();

    QByteArray buffer = this->d->m_audioBuffer.mid(0, bufferSize);
    this->d->m_audioBuffer.remove(0, bufferSize);

    return buffer;
}

bool AudioDevWasapi::write(const AkAudioPacket &packet)
{
    QByteArray data(reinterpret_cast<const char *>(packet.constPlane(0)),
                    int(packet.planeSize(0)));
    this->d->m_audioBuffer.append(data);
    int nErrors = 0;

    while (!this->d->m_audioBuffer.isEmpty()
           && nErrors < MAX_ERRORS_READ_WRITE) {
        // Wait until an audio frame can be writen.
        if (WaitForSingleObject(this->d->m_hEvent, EVENT_TIMEOUT) != WAIT_OBJECT_0) {
            nErrors++;

            continue;
        }

        HRESULT hr;
        UINT32 samplesCount;

        // Get audio buffer size in samples.
        if (FAILED(hr = this->d->m_pAudioClient->GetBufferSize(&samplesCount))) {
            this->d->m_error = "GetBufferSize: " + errorCodes->value(hr);
            emit this->errorChanged(this->d->m_error);

            return false;
        }

        UINT32 numSamplesPadding;

        // Get the number of samples already present in the audio buffer.
        if (FAILED(hr = this->d->m_pAudioClient->GetCurrentPadding(&numSamplesPadding))) {
            this->d->m_error = "GetCurrentPadding: " + errorCodes->value(hr);
            emit this->errorChanged(this->d->m_error);

            return false;
        }

        // Calculate the difference. This is the number of samples we can write
        // to the audio buffer.
        UINT32 availableSamples = samplesCount - numSamplesPadding;

        // This is probably an impossible but well... check it.
        if (availableSamples < 1)
            continue;

        // Check how many samples we can write to the audio buffer.
        UINT32 samplesInBuffer = UINT32(this->d->m_audioBuffer.size()
                                        * 8
                                        / this->d->m_curCaps.bps()
                                        / this->d->m_curCaps.channels());
        UINT32 samplesToWrite = qMin(availableSamples, samplesInBuffer);

        BYTE *pData = nullptr;

        // Get the audio buffer.
        if (FAILED(hr = this->d->m_pRenderClient->GetBuffer(samplesToWrite, &pData))) {
            this->d->m_error = "GetBuffer: " + errorCodes->value(hr);
            emit this->errorChanged(this->d->m_error);

            return false;
        }

        // Copy the maximum number of audio samples we can write to the audio
        // buffer.
        size_t bufferSize =
                samplesToWrite
                * size_t(this->d->m_curCaps.bps()
                         * this->d->m_curCaps.channels())
                / 8;

        memcpy(pData, this->d->m_audioBuffer.constData(), bufferSize);
        this->d->m_audioBuffer.remove(0, int(bufferSize));

        // Tell audio device how many samples we had written.
        if (FAILED(hr = this->d->m_pRenderClient->ReleaseBuffer(samplesToWrite, 0))) {
            this->d->m_error = "ReleaseBuffer: " + errorCodes->value(hr);
            emit this->errorChanged(this->d->m_error);

            return false;
        }
    }

    return true;
}

bool AudioDevWasapi::uninit()
{
    bool ok = true;
    HRESULT hr;

    // Stop audio device.
    if (this->d->m_pAudioClient && FAILED(hr = this->d->m_pAudioClient->Stop())) {
        this->d->m_error = "Stop: " + errorCodes->value(hr);
        emit this->errorChanged(this->d->m_error);
        ok = false;
    }

    // Release interfaces.
    if (this->d->m_pCaptureClient) {
        this->d->m_pCaptureClient->Release();
        this->d->m_pCaptureClient = nullptr;
    }

    if (this->d->m_pRenderClient) {
        this->d->m_pRenderClient->Release();
        this->d->m_pRenderClient = nullptr;
    }

    if (this->d->m_pAudioClient) {
        this->d->m_pAudioClient->Release();
        this->d->m_pAudioClient = nullptr;
    }

    if (this->d->m_pDevice) {
        this->d->m_pDevice->Release();
        this->d->m_pDevice = nullptr;
    }

    if (this->d->m_hEvent) {
        CloseHandle(this->d->m_hEvent);
        this->d->m_hEvent = nullptr;
    }

    this->d->m_curDevice.clear();

    return ok;
}

HRESULT AudioDevWasapi::QueryInterface(const IID &riid, void **ppvObject)
{
    if (riid == __uuidof(IUnknown)
        || riid == __uuidof(IMMNotificationClient))
        *ppvObject = static_cast<IMMNotificationClient *>(this);
    else {
        *ppvObject = nullptr;

        return E_NOINTERFACE;
    }

    this->AddRef();

    return S_OK;
}

ULONG AudioDevWasapi::AddRef()
{
    return InterlockedIncrement(&this->d->m_cRef);
}

ULONG AudioDevWasapi::Release()
{
    ULONG lRef = InterlockedDecrement(&this->d->m_cRef);

    if (lRef == 0)
        delete this;

    return lRef;
}

AudioDevWasapiPrivate::AudioDevWasapiPrivate(AudioDevWasapi *self):
    self(self)
{
}

bool AudioDevWasapiPrivate::waveFormatFromCaps(WAVEFORMATEX *wfx,
                                               const AkAudioCaps &caps) const
{
    if (!wfx)
        return false;

    wfx->wFormatTag = caps.format() == AkAudioCaps::SampleFormat_flt?
                          WAVE_FORMAT_IEEE_FLOAT: WAVE_FORMAT_PCM;
    wfx->nChannels = WORD(caps.channels());
    wfx->nSamplesPerSec = DWORD(caps.rate());
    wfx->wBitsPerSample = WORD(caps.bps());
    wfx->nBlockAlign = wfx->nChannels * wfx->wBitsPerSample / 8;
    wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;
    wfx->cbSize = 0;

    return true;
}

AkAudioCaps AudioDevWasapiPrivate::capsFromWaveFormat(WAVEFORMATEX *wfx) const
{
    if (!wfx)
        return AkAudioCaps();

    auto sampleType =
        wfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT?
            AkAudioCaps::SampleType_float:
            AkAudioCaps::SampleType_int;
    auto sampleFormat =
        AkAudioCaps::sampleFormatFromProperties(sampleType,
                                                int(wfx->wBitsPerSample),
                                                Q_BYTE_ORDER);

    return AkAudioCaps(sampleFormat,
                       AkAudioCaps::defaultChannelLayout(int(wfx->nChannels)),
                       false,
                       int(wfx->nSamplesPerSec));
}

void AudioDevWasapiPrivate::fillDeviceInfo(const QString &device,
                                           QList<AkAudioCaps::SampleFormat> *supportedFormats,
                                           QList<AkAudioCaps::ChannelLayout> *supportedLayouts,
                                           QList<int> *supportedSampleRates) const
{
    if (!this->m_deviceEnumerator)
        return;

    IMMDevice *pDevice = nullptr;
    IAudioClient *pAudioClient = nullptr;

    // Test if the device is already running,
    if (this->m_curDevice != device) {
        // Get audio device.
        if (FAILED(this->m_deviceEnumerator->GetDevice(device.toStdWString().c_str(),
                                                       &pDevice)))
            return;

        // Get an instance for the audio client.
        if (FAILED(pDevice->Activate(__uuidof(IAudioClient),
                                     CLSCTX_ALL,
                                     nullptr,
                                     reinterpret_cast<void **>(&pAudioClient)))) {
            pDevice->Release();

            return;
        }
    } else {
        pDevice = this->m_pDevice;
        pAudioClient = this->m_pAudioClient;
    }

    static const QVector<AkAudioCaps::SampleFormat> preferredFormats {
        AkAudioCaps::SampleFormat_flt,
        AkAudioCaps::SampleFormat_s32,
        AkAudioCaps::SampleFormat_s16,
        AkAudioCaps::SampleFormat_u8,
    };

    for (auto &format: preferredFormats)
        for (int channels = 1; channels < 3; channels++)
            for (auto &rate: self->commonSampleRates()) {
                WAVEFORMATEX wfx;
                WAVEFORMATEX *closestWfx = nullptr;
                AkAudioCaps audioCaps(format,
                                      AkAudioCaps::defaultChannelLayout(channels),
                                      false,
                                      rate);
                this->waveFormatFromCaps(&wfx, audioCaps);

                if (SUCCEEDED(pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED,
                                                              &wfx,
                                                              &closestWfx))) {
                    AkAudioCaps::SampleFormat sampleFormat;
                    AkAudioCaps::ChannelLayout layout;
                    int sampleRate;

                    if (closestWfx) {
                        auto sampleType =
                            closestWfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT?
                                AkAudioCaps::SampleType_float:
                                AkAudioCaps::SampleType_int;
                        sampleFormat =
                            AkAudioCaps::sampleFormatFromProperties(sampleType,
                                                                    int(closestWfx->wBitsPerSample),
                                                                    Q_BYTE_ORDER);
                        layout = AkAudioCaps::defaultChannelLayout(int(closestWfx->nChannels));
                        sampleRate = int(closestWfx->nSamplesPerSec);
                        CoTaskMemFree(closestWfx);
                    } else {
                        sampleFormat = format;
                        layout = AkAudioCaps::defaultChannelLayout(channels);
                        sampleRate = rate;
                    }

                    if (!supportedFormats->contains(sampleFormat))
                        supportedFormats->append(sampleFormat);

                    if (layout != AkAudioCaps::Layout_none
                        && !supportedLayouts->contains(layout))
                        supportedLayouts->append(layout);

                    if (!supportedSampleRates->contains(sampleRate))
                        supportedSampleRates->append(sampleRate);
                }
            }

    if (this->m_curDevice != device) {
        pAudioClient->Release();
        pDevice->Release();
    }

    std::sort(supportedFormats->begin(), supportedFormats->end());
}

AkAudioCaps AudioDevWasapiPrivate::preferredCaps(const QString &device,
                                                 EDataFlow dataFlow) const
{
    if (!this->m_deviceEnumerator)
        return AkAudioCaps();

    IMMDevice *pDevice = nullptr;
    IAudioClient *pAudioClient = nullptr;

    // Test if the device is already running,
    if (this->m_curDevice != device) {
        // Get audio device.
        if (FAILED(this->m_deviceEnumerator->GetDevice(device.toStdWString().c_str(),
                                                       &pDevice)))
            return AkAudioCaps();

        // Get an instance for the audio client.
        if (FAILED(pDevice->Activate(__uuidof(IAudioClient),
                                     CLSCTX_ALL,
                                     nullptr,
                                     reinterpret_cast<void **>(&pAudioClient)))) {
            pDevice->Release();

            return AkAudioCaps();
        }
    } else {
        pDevice = this->m_pDevice;
        pAudioClient = this->m_pAudioClient;
    }

    AkAudioCaps caps = dataFlow == eCapture?
                AkAudioCaps(AkAudioCaps::SampleFormat_u8,
                            AkAudioCaps::Layout_mono,
                            false,
                            8000):
                AkAudioCaps(AkAudioCaps::SampleFormat_s16,
                            AkAudioCaps::Layout_stereo,
                            false,
                            44100);

    WAVEFORMATEX wfx;
    WAVEFORMATEX *closestWfx = nullptr;
    this->waveFormatFromCaps(&wfx, caps);

    if (SUCCEEDED(pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED,
                                      &wfx,
                                      &closestWfx))) {
        if (closestWfx) {
            caps = this->capsFromWaveFormat(closestWfx);
            CoTaskMemFree(closestWfx);
        }
    }

    if (this->m_curDevice != device) {
        pAudioClient->Release();
        pDevice->Release();
    }

    return caps;
}

HRESULT AudioDevWasapi::OnDeviceStateChanged(LPCWSTR pwstrDeviceId,
                                             DWORD dwNewState)
{
    Q_UNUSED(pwstrDeviceId)
    Q_UNUSED(dwNewState)

    return S_OK;
}

HRESULT AudioDevWasapi::OnDeviceAdded(LPCWSTR pwstrDeviceId)
{
    Q_UNUSED(pwstrDeviceId)

    // Device was installed

    return S_OK;
}

HRESULT AudioDevWasapi::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
{
    Q_UNUSED(pwstrDeviceId)

    // Device was uninstalled

    return S_OK;
}

HRESULT AudioDevWasapi::OnDefaultDeviceChanged(EDataFlow flow,
                                               ERole role,
                                               LPCWSTR pwstrDeviceId)
{
    if (role != eMultimedia)
        return S_OK;

    QString deviceId = QString::fromWCharArray(pwstrDeviceId);

    if (flow == eCapture) {
        this->d->m_defaultSource = deviceId;
        emit this->defaultInputChanged(deviceId);
    } else if (flow == eRender) {
        this->d->m_defaultSink = deviceId;
        emit this->defaultOutputChanged(deviceId);
    }

    return S_OK;
}

HRESULT AudioDevWasapi::OnPropertyValueChanged(LPCWSTR pwstrDeviceId,
                                               const PROPERTYKEY key)
{
    Q_UNUSED(pwstrDeviceId)
    Q_UNUSED(key)

    this->updateDevices();

    return S_OK;
}

void AudioDevWasapi::updateDevices()
{
    if (!this->d->m_deviceEnumerator) {
        this->d->m_error = "Device enumerator not created.";
        emit this->errorChanged(this->d->m_error);

        return;
    }

    decltype(this->d->m_sources) inputs;
    decltype(this->d->m_sinks) outputs;
    decltype(this->d->m_defaultSink) defaultSink;
    decltype(this->d->m_defaultSource) defaultSource;
    decltype(this->d->m_descriptionMap) descriptionMap;
    decltype(this->d->m_supportedFormats) supportedFormats;
    decltype(this->d->m_supportedLayouts) supportedChannels;
    decltype(this->d->m_supportedSampleRates) supportedSampleRates;
    decltype(this->d->m_preferredInputCaps) preferredInputCaps;
    decltype(this->d->m_preferredOutputCaps) preferredOutputCaps;

    for (auto &dataFlow: QVector<EDataFlow> {eCapture, eRender}) {
        HRESULT hr;
        IMMDevice *defaultDevice = nullptr;

        if (SUCCEEDED(hr = this->d->m_deviceEnumerator->GetDefaultAudioEndpoint(dataFlow,
                                                                                eMultimedia,
                                                                                &defaultDevice))) {
            LPWSTR deviceId;

            if (SUCCEEDED(hr = defaultDevice->GetId(&deviceId))) {
                if (dataFlow == eCapture)
                    defaultSource = QString::fromWCharArray(deviceId);
                else
                    defaultSink = QString::fromWCharArray(deviceId);

                CoTaskMemFree(deviceId);
            }

            defaultDevice->Release();
        }

        IMMDeviceCollection *endPoints = nullptr;

        if (SUCCEEDED(hr = this->d->m_deviceEnumerator->EnumAudioEndpoints(dataFlow,
                                                                           eMultimedia,
                                                                           &endPoints))) {
            UINT nDevices = 0;

            if (SUCCEEDED(hr = endPoints->GetCount(&nDevices)))
                for (UINT i = 0; i < nDevices; i++) {
                    IMMDevice *device = nullptr;

                    if (SUCCEEDED(endPoints->Item(i, &device))) {
                        LPWSTR deviceId;

                        if (SUCCEEDED(hr = device->GetId(&deviceId))) {
                            IPropertyStore *properties = nullptr;

                            if (SUCCEEDED(hr = device->OpenPropertyStore(STGM_READ, &properties))) {
                                PROPVARIANT friendlyName;
                                PropVariantInit(&friendlyName);

                                if (SUCCEEDED(hr = properties->GetValue(PKEY_Device_FriendlyName,
                                                                        &friendlyName))) {
                                    auto devId = QString::fromWCharArray(deviceId);

                                    QList<AkAudioCaps::SampleFormat> _supportedFormats;
                                    QList<AkAudioCaps::ChannelLayout> _supportedLayouts;
                                    QList<int> _supportedSampleRates;
                                    this->d->fillDeviceInfo(devId,
                                                            &_supportedFormats,
                                                            &_supportedLayouts,
                                                            &_supportedSampleRates);

                                    if (_supportedFormats.isEmpty())
                                        _supportedFormats =
                                                this->d->m_supportedFormats.value(devId);

                                    if (_supportedLayouts.isEmpty())
                                        _supportedLayouts =
                                                this->d->m_supportedLayouts.value(devId);

                                    if (_supportedSampleRates.isEmpty())
                                        _supportedSampleRates =
                                                this->d->m_supportedSampleRates.value(devId);

                                    if (!_supportedFormats.isEmpty()
                                        && !_supportedLayouts.isEmpty()
                                        && !_supportedSampleRates.isEmpty()) {
                                        if (dataFlow == eCapture) {
                                            inputs << devId;
                                            preferredInputCaps[devId] =
                                                this->d->preferredCaps(devId,
                                                                       dataFlow);
                                        } else {
                                            outputs << devId;
                                            preferredOutputCaps[devId] =
                                                this->d->preferredCaps(devId,
                                                                       dataFlow);
                                        }

                                        descriptionMap[devId] =
                                                QString::fromWCharArray(friendlyName.pwszVal);
                                        supportedFormats[devId] = _supportedFormats;
                                        supportedChannels[devId] = _supportedLayouts;
                                        supportedSampleRates[devId] = _supportedSampleRates;
                                    }

                                    PropVariantClear(&friendlyName);
                                }

                                properties->Release();
                            }

                            CoTaskMemFree(deviceId);
                        }

                        device->Release();
                    }
                }

            endPoints->Release();
        }
    }

    if (this->d->m_supportedFormats != supportedFormats)
        this->d->m_supportedFormats = supportedFormats;

    if (this->d->m_supportedLayouts != supportedChannels)
        this->d->m_supportedLayouts = supportedChannels;

    if (this->d->m_supportedSampleRates != supportedSampleRates)
        this->d->m_supportedSampleRates = supportedSampleRates;

    if (this->d->m_descriptionMap != descriptionMap)
        this->d->m_descriptionMap = descriptionMap;

    this->d->m_preferredInputCaps = preferredInputCaps;
    this->d->m_preferredOutputCaps = preferredOutputCaps;

    if (this->d->m_sources != inputs) {
        this->d->m_sources = inputs;
        emit this->inputsChanged(inputs);
    }

    if (this->d->m_sinks != outputs) {
        this->d->m_sinks = outputs;
        emit this->outputsChanged(outputs);
    }

    if (defaultSource.isEmpty() && !inputs.isEmpty())
        defaultSource = inputs.first();

    if (defaultSink.isEmpty() && !outputs.isEmpty())
        defaultSink = outputs.first();

    if (this->d->m_defaultSource != defaultSource) {
        this->d->m_defaultSource = defaultSource;
        emit this->defaultInputChanged(defaultSource);
    }

    if (this->d->m_defaultSink != defaultSink) {
        this->d->m_defaultSink = defaultSink;
        emit this->defaultOutputChanged(defaultSink);
    }
}

#include "moc_audiodevwasapi.cpp"
