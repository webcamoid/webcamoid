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

#include <QMap>

#include "audiodev.h"

#define MAX_ERRORS_READ_WRITE 5
#define EVENT_TIMEOUT 1000

typedef QMap<HRESULT, QString> ErrorCodesMap;

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

typedef QMap<QString, AkAudioCaps::SampleFormat> SampleFormatsMap;

inline SampleFormatsMap initSampleFormatsMap()
{
    SampleFormatsMap sampleFormats = {
        {QString("%1_%2").arg(WAVE_FORMAT_PCM).arg(8)        , AkAudioCaps::SampleFormat_u8 },
        {QString("%1_%2").arg(WAVE_FORMAT_PCM).arg(16)       , AkAudioCaps::SampleFormat_s16},
        {QString("%1_%2").arg(WAVE_FORMAT_PCM).arg(32)       , AkAudioCaps::SampleFormat_s32},
        {QString("%1_%2").arg(WAVE_FORMAT_IEEE_FLOAT).arg(32), AkAudioCaps::SampleFormat_flt}
    };

    return sampleFormats;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatsMap, sampleFormats, (initSampleFormatsMap()))

AudioDev::AudioDev(QObject *parent):
    QObject(parent)
{
    this->m_deviceEnumerator = NULL;
    this->m_pDevice = NULL;
    this->m_pAudioClient = NULL;
    this->m_pCaptureClient = NULL;
    this->m_pRenderClient = NULL;
    this->m_hEvent = NULL;
    this->m_cRef = 1;

    // Create DeviceEnumerator
    HRESULT hr;

    // Get device enumerator.
    if (FAILED(hr = CoCreateInstance(CLSID_MMDeviceEnumerator,
                                     NULL,
                                     CLSCTX_ALL,
                                     IID_IMMDeviceEnumerator,
                                     reinterpret_cast<void **>(&this->m_deviceEnumerator)))) {
        return;
    }

    if (FAILED(hr = this->m_deviceEnumerator->RegisterEndpointNotificationCallback(this))) {
        this->m_deviceEnumerator->Release();
        this->m_deviceEnumerator = NULL;

        return;
    }

    this->m_inputs = this->listDevices(eCapture);
    this->m_outputs = this->listDevices(eRender);
}

AudioDev::~AudioDev()
{
    this->uninit();
    this->m_deviceEnumerator->UnregisterEndpointNotificationCallback(this);
    this->m_deviceEnumerator->Release();
}

QString AudioDev::error() const
{
    return this->m_error;
}

QString AudioDev::defaultInput()
{
    return this->defaultDevice(eCapture);
}

QString AudioDev::defaultOutput()
{
    return this->defaultDevice(eRender);
}

QStringList AudioDev::inputs()
{
    return this->m_inputs;
}

QStringList AudioDev::outputs()
{
    return this->m_outputs;
}

QString AudioDev::AudioDev::description(const QString &device)
{
    if (!this->m_deviceEnumerator) {
        this->m_error = "Device enumerator not created.";
        emit this->errorChanged(this->m_error);

        return QString();
    }

    HRESULT hr;
    IMMDevice *pDevice = NULL;

    if (FAILED(hr = this->m_deviceEnumerator->GetDevice(device.toStdWString().c_str(),
                                                        &pDevice))) {
        this->m_error = "GetDevice: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);

        return QString();
    }

    IPropertyStore *properties = NULL;

    if (FAILED(hr = pDevice->OpenPropertyStore(STGM_READ, &properties))) {
        this->m_error = "OpenPropertyStore: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);
        pDevice->Release();

        return QString();
    }

    PROPVARIANT friendlyName;
    PropVariantInit(&friendlyName);

    if (FAILED(hr = properties->GetValue(PKEY_Device_FriendlyName,
                                         &friendlyName))) {
    }

    QString description = QString::fromWCharArray(friendlyName.pwszVal);

    PropVariantClear(&friendlyName);
    properties->Release();
    pDevice->Release();

    return description;
}

// Get native format for the default audio device.
AkAudioCaps AudioDev::preferredFormat(const QString &device)
{
    // Test if the device is already running,
    bool isActivated = this->m_pAudioClient? true: false;

    // if not activate it and get an audio client instance.
    if (!isActivated)
        if (!this->init(device, AkAudioCaps(), true))
            return AkAudioCaps();

    HRESULT hr;
    WAVEFORMATEX *pwfx = NULL;

    // Get default format.
    if (FAILED(hr = this->m_pAudioClient->GetMixFormat(&pwfx))) {
        this->m_error = "GetMixFormat: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);

        if (!isActivated)
            this->uninit();

        return AkAudioCaps();
    }

    // Convert device format to a supported one.
    WORD formatTag = pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT?
                         WAVE_FORMAT_IEEE_FLOAT:
                         WAVE_FORMAT_PCM;

    QString fmtStr = QString("%1_%2")
                     .arg(formatTag)
                     .arg(pwfx->wBitsPerSample);

    AkAudioCaps audioCaps;
    audioCaps.isValid() = true;
    audioCaps.format() = sampleFormats->value(fmtStr, AkAudioCaps::SampleFormat_u8);
    audioCaps.bps() = AkAudioCaps::bitsPerSample(audioCaps.format());
    audioCaps.channels() = pwfx->nChannels;
    audioCaps.rate() = int(pwfx->nSamplesPerSec);
    audioCaps.layout() = AkAudioCaps::defaultChannelLayout(pwfx->nChannels);
    audioCaps.align() = false;

    if (!isActivated) {
        // Stop audio device if required.
        this->uninit();

        // Workaround for buggy drivers. Test if format is really supported.
        if (!this->init(device, audioCaps)) {
            // Test sample formats from highest sample resolusion to lowest.
            static const QVector<AkAudioCaps::SampleFormat> preferredFormats = {
                AkAudioCaps::SampleFormat_flt,
                AkAudioCaps::SampleFormat_s32,
                AkAudioCaps::SampleFormat_s16,
                AkAudioCaps::SampleFormat_u8
            };

            bool isValidFormat = false;

            foreach (AkAudioCaps::SampleFormat format, preferredFormats) {
                audioCaps.format() = format;

                if (this->init(device, audioCaps)) {
                    isValidFormat = true;

                    break;
                }
            }

            if (!isValidFormat)
                audioCaps = AkAudioCaps();
        }

        this->uninit();
    }

    return audioCaps;
}

bool AudioDev::init(const QString &device,
                    const AkAudioCaps &caps,
                    bool justActivate)
{
    if (!this->m_deviceEnumerator) {
        this->m_error = "Device enumerator not created.";
        emit this->errorChanged(this->m_error);

        return false;
    }

    // Clear audio buffer.
    this->m_audioBuffer.clear();

    HRESULT hr;

    // Get audio device.
    if (FAILED(hr = this->m_deviceEnumerator->GetDevice(device.toStdWString().c_str(),
                                                   &this->m_pDevice))) {
        this->m_error = "GetDevice: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);
        this->uninit();

        return false;
    }

    // Get an instance for the audio client.
    if (FAILED(hr = this->m_pDevice->Activate(IID_IAudioClient,
                                              CLSCTX_ALL,
                                              NULL,
                                              reinterpret_cast<void **>(&this->m_pAudioClient)))) {
        this->m_error = "Activate: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);
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
    this->m_pAudioClient->GetDevicePeriod(NULL, &hnsRequestedDuration);

    // Accumulate a minimum of 1 sec. of audio in the buffer.
    REFERENCE_TIME minDuration = 10e6;

    if (hnsRequestedDuration < minDuration)
        hnsRequestedDuration = minDuration;

    // Set audio device format.
    WAVEFORMATEX wfx;
    wfx.wFormatTag = caps.format() == AkAudioCaps::SampleFormat_flt?
                         WAVE_FORMAT_IEEE_FLOAT: WAVE_FORMAT_PCM;
    wfx.nChannels = WORD(caps.channels());
    wfx.nSamplesPerSec = DWORD(caps.rate());
    wfx.wBitsPerSample = WORD(caps.bps());
    wfx.nBlockAlign = WORD(caps.channels() * caps.bps() / 8);
    wfx.nAvgBytesPerSec = DWORD(caps.rate() * wfx.nBlockAlign);
    wfx.cbSize = 0;

    this->m_curCaps = caps;

    if (FAILED(hr = this->m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                                     AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                                     hnsRequestedDuration,
                                                     hnsRequestedDuration,
                                                     &wfx,
                                                     NULL))) {
        this->m_error = "Initialize: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);
        this->uninit();

        return false;
    }

    // Create an event handler for checking when an aundio frame is required
    // for reading or writing.
    this->m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!this->m_hEvent) {
        this->m_error = "CreateEvent: Error creating event handler";
        emit this->errorChanged(this->m_error);
        this->uninit();

        return false;
    }

    // Set event handler.
    if (FAILED(this->m_pAudioClient->SetEventHandle(this->m_hEvent))) {
        this->m_error = "SetEventHandle: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);
        this->uninit();

        return false;
    }

    // Get audio capture/render client.
    if (this->inputs().contains(device))
        hr = this->m_pAudioClient->GetService(IID_IAudioCaptureClient,
                                              reinterpret_cast<void **>(&this->m_pCaptureClient));
    else
        hr = this->m_pAudioClient->GetService(IID_IAudioRenderClient,
                                              reinterpret_cast<void **>(&this->m_pRenderClient));

    if (FAILED(hr)) {
        this->m_error = "GetService: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);
        this->uninit();

        return false;
    }

    // Start audio client.
    if (FAILED(hr = this->m_pAudioClient->Start())) {
        this->m_error = "Start: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);
        this->uninit();

        return false;
    }

    return true;
}

QByteArray AudioDev::read(int samples)
{
    int bufferSize = samples
                     * this->m_curCaps.bps()
                     * this->m_curCaps.channels()
                     / 8;

    int nErrors = 0;

    // Read audio samples until audio buffer is full.
    while (this->m_audioBuffer.size() < bufferSize
           && nErrors < MAX_ERRORS_READ_WRITE) {
        // Wait until an audio frame can be read.
        if (WaitForSingleObject(this->m_hEvent, EVENT_TIMEOUT) != WAIT_OBJECT_0) {
            nErrors++;

            continue;
        }

        HRESULT hr;
        UINT32 samplesCount = 0;

        // Get the size in samples of the captured audio frame.
        if (FAILED(hr = this->m_pCaptureClient->GetNextPacketSize(&samplesCount))) {
            this->m_error = "GetNextPacketSize: " + errorCodes->value(hr);
            emit this->errorChanged(this->m_error);

            return QByteArray();
        }

        // Check if empty.
        if (samplesCount < 1)
            continue;

        BYTE *pData = NULL;
        DWORD flags = 0;

        // Read audio buffer.
        if (FAILED(hr = this->m_pCaptureClient->GetBuffer(&pData,
                                                          &samplesCount,
                                                          &flags,
                                                          NULL,
                                                          NULL))) {
            this->m_error = "GetBuffer: " + errorCodes->value(hr);
            emit this->errorChanged(this->m_error);

            return QByteArray();
        }

        size_t bufferSize = samplesCount
                            * size_t(this->m_curCaps.bps()
                                     * this->m_curCaps.channels())
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

        this->m_audioBuffer.append(buffer);

        // Remove read samples from the audio device.
        if (FAILED(hr = this->m_pCaptureClient->ReleaseBuffer(samplesCount))) {
            this->m_error = "ReleaseBuffer: " + errorCodes->value(hr);
            emit this->errorChanged(this->m_error);

            return QByteArray();
        }
    }

    // In case of error and if the buffer is empty, return.
    if (this->m_audioBuffer.isEmpty())
        return QByteArray();

    QByteArray buffer = this->m_audioBuffer.mid(0, bufferSize);
    this->m_audioBuffer.remove(0, bufferSize);

    return buffer;
}

bool AudioDev::write(const QByteArray &frame)
{
    this->m_audioBuffer.append(frame);
    int nErrors = 0;

    while (!this->m_audioBuffer.isEmpty()
           && nErrors < MAX_ERRORS_READ_WRITE) {
        // Wait until an audio frame can be writen.
        if (WaitForSingleObject(this->m_hEvent, EVENT_TIMEOUT) != WAIT_OBJECT_0) {
            nErrors++;

            continue;
        }

        HRESULT hr;
        UINT32 samplesCount;

        // Get audio buffer size in samples.
        if (FAILED(hr = this->m_pAudioClient->GetBufferSize(&samplesCount))) {
            this->m_error = "GetBufferSize: " + errorCodes->value(hr);
            emit this->errorChanged(this->m_error);

            return false;
        }

        UINT32 numSamplesPadding;

        // Get the number of samples already present in the audio buffer.
        if (FAILED(hr = this->m_pAudioClient->GetCurrentPadding(&numSamplesPadding))) {
            this->m_error = "GetCurrentPadding: " + errorCodes->value(hr);
            emit this->errorChanged(this->m_error);

            return false;
        }

        // Calculate the difference. This is the number of samples we can write
        // to the audio buffer.
        UINT32 availableSamples = samplesCount - numSamplesPadding;

        // This is probably an impossible but well... check it.
        if (availableSamples < 1)
            continue;

        // Check how many samples we can write to the audio buffer.
        UINT32 samplesInBuffer = UINT32(this->m_audioBuffer.size()
                                        * 8
                                        / this->m_curCaps.bps()
                                        / this->m_curCaps.channels());
        UINT32 samplesToWrite = qMin(availableSamples, samplesInBuffer);

        BYTE *pData = NULL;

        // Get the audio buffer.
        if (FAILED(hr = this->m_pRenderClient->GetBuffer(samplesToWrite, &pData))) {
            this->m_error = "GetBuffer: " + errorCodes->value(hr);
            emit this->errorChanged(this->m_error);

            return false;
        }

        // Copy the maximum number of audio samples we can write to the audio
        // buffer.
        size_t bufferSize = samplesToWrite
                            * size_t(this->m_curCaps.bps()
                                     * this->m_curCaps.channels())
                            / 8;

        memcpy(pData, this->m_audioBuffer.constData(), bufferSize);
        this->m_audioBuffer.remove(0, int(bufferSize));

        // Tell audio device how many samples we had written.
        if (FAILED(hr = this->m_pRenderClient->ReleaseBuffer(samplesToWrite, 0))) {
            this->m_error = "ReleaseBuffer: " + errorCodes->value(hr);
            emit this->errorChanged(this->m_error);

            return false;
        }
    }

    return true;
}

bool AudioDev::uninit()
{
    bool ok = true;
    HRESULT hr;

    // Stop audio device.
    if (this->m_pAudioClient && FAILED(hr = this->m_pAudioClient->Stop())) {
        this->m_error = "Stop: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);
        ok = false;
    }

    // Release interfaces.
    if (this->m_pCaptureClient) {
        this->m_pCaptureClient->Release();
        this->m_pCaptureClient = NULL;
    }

    if (this->m_pRenderClient) {
        this->m_pRenderClient->Release();
        this->m_pRenderClient = NULL;
    }

    if (this->m_pAudioClient) {
        this->m_pAudioClient->Release();
        this->m_pAudioClient = NULL;
    }

    if (this->m_pDevice) {
        this->m_pDevice->Release();
        this->m_pDevice = NULL;
    }

    if (this->m_hEvent) {
        CloseHandle(this->m_hEvent);
        this->m_hEvent = NULL;
    }

    return ok;
}

HRESULT AudioDev::QueryInterface(const IID &riid, void **ppvObject)
{
    if (riid == __uuidof(IUnknown)
        || riid == __uuidof(IMMNotificationClient))
        *ppvObject = static_cast<IMMNotificationClient *>(this);
    else {
        *ppvObject = NULL;

        return E_NOINTERFACE;
    }

    this->AddRef();

    return S_OK;
}

ULONG AudioDev::AddRef()
{
    return InterlockedIncrement(&this->m_cRef);
}

ULONG AudioDev::Release()
{
    ULONG lRef = InterlockedDecrement(&this->m_cRef);

    if (lRef == 0)
        delete this;

    return lRef;
}

QString AudioDev::defaultDevice(EDataFlow dataFlow)
{
    if (!this->m_deviceEnumerator) {
        this->m_error = "Device enumerator not created.";
        emit this->errorChanged(this->m_error);

        return QString();
    }

    HRESULT hr;
    IMMDevice *defaultDevice = NULL;

    if (FAILED(hr = this->m_deviceEnumerator->GetDefaultAudioEndpoint(dataFlow,
                                                                      eMultimedia,
                                                                      &defaultDevice))) {
        this->m_error = "GetDefaultAudioEndpoint: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);

        return QString();
    }

    LPWSTR deviceId;

    if (FAILED(hr = defaultDevice->GetId(&deviceId))) {
        this->m_error = "GetId: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);
        defaultDevice->Release();

        return QString();
    }

    QString id = QString::fromWCharArray(deviceId);

    CoTaskMemFree(deviceId);
    defaultDevice->Release();

    return id;
}

QStringList AudioDev::listDevices(EDataFlow dataFlow)
{
    if (!this->m_deviceEnumerator) {
        this->m_error = "Device enumerator not created.";
        emit this->errorChanged(this->m_error);

        return QStringList();
    }

    HRESULT hr;
    IMMDeviceCollection *endPoints = NULL;

    if (FAILED(hr = this->m_deviceEnumerator->EnumAudioEndpoints(dataFlow,
                                                                 eMultimedia,
                                                                 &endPoints))) {
        this->m_error = "EnumAudioEndpoints: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);

        return QStringList();
    }

    UINT nDevices = 0;

    if (FAILED(hr = endPoints->GetCount(&nDevices))) {
        this->m_error = "GetCount: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);
        endPoints->Release();

        return QStringList();
    }

    QStringList devices;

    for (UINT i = 0; i < nDevices; i++) {
        IMMDevice *device = NULL;

        if (FAILED(endPoints->Item(i, &device)))
            continue;

        LPWSTR deviceId;

        if (FAILED(hr = device->GetId(&deviceId))) {
            device->Release();

            continue;
        }

        devices << QString::fromWCharArray(deviceId);
        CoTaskMemFree(deviceId);
        device->Release();
    }

    endPoints->Release();

    return devices;
}

HRESULT AudioDev::OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState)
{
    Q_UNUSED(pwstrDeviceId)
    Q_UNUSED(dwNewState)

    return S_OK;
}

HRESULT AudioDev::OnDeviceAdded(LPCWSTR pwstrDeviceId)
{
    Q_UNUSED(pwstrDeviceId)

    // Device was installed

    return S_OK;
}

HRESULT AudioDev::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
{
    Q_UNUSED(pwstrDeviceId)

    // Device was uninstalled

    return S_OK;
}

HRESULT AudioDev::OnDefaultDeviceChanged(EDataFlow flow,
                                         ERole role,
                                         LPCWSTR pwstrDeviceId)
{
    if (role != eMultimedia)
        return S_OK;

    QString deviceId = QString::fromWCharArray(pwstrDeviceId);

    if (flow == eCapture)
        emit this->defaultInputChanged(deviceId);
    else if (flow == eRender)
        emit this->defaultOutputChanged(deviceId);

    return S_OK;
}

HRESULT AudioDev::OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key)
{
    Q_UNUSED(pwstrDeviceId)
    Q_UNUSED(key)

    QStringList inputs = this->listDevices(eCapture);

    if (this->m_inputs != inputs) {
        this->m_inputs = inputs;
        emit this->inputsChanged(inputs);
    }

    QStringList outputs = this->listDevices(eRender);

    if (this->m_outputs != outputs) {
        this->m_outputs = outputs;
        emit this->outputsChanged(outputs);
    }

    return S_OK;
}
