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
    ErrorCodesMap errorCodes;

    // COM library errors.
    errorCodes[REGDB_E_CLASSNOTREG] = "REGDB_E_CLASSNOTREG";
    errorCodes[CLASS_E_NOAGGREGATION] = "CLASS_E_NOAGGREGATION";
    errorCodes[E_NOINTERFACE] = "E_NOINTERFACE";
    errorCodes[E_POINTER] = "E_POINTER";

    // IMMDeviceEnumerator errors.
    errorCodes[E_INVALIDARG] = "E_INVALIDARG";
    errorCodes[E_NOTFOUND] = "E_NOTFOUND";
    errorCodes[E_OUTOFMEMORY] = "E_OUTOFMEMORY";

    // IAudioClient errors.
    errorCodes[AUDCLNT_E_ALREADY_INITIALIZED] = "AUDCLNT_E_ALREADY_INITIALIZED";
    errorCodes[AUDCLNT_E_WRONG_ENDPOINT_TYPE] = "AUDCLNT_E_WRONG_ENDPOINT_TYPE";
    errorCodes[AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED] = "AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED";
    errorCodes[AUDCLNT_E_BUFFER_SIZE_ERROR] = "AUDCLNT_E_BUFFER_SIZE_ERROR";
    errorCodes[AUDCLNT_E_CPUUSAGE_EXCEEDED] = "AUDCLNT_E_CPUUSAGE_EXCEEDED";
    errorCodes[AUDCLNT_E_DEVICE_INVALIDATED] = "AUDCLNT_E_DEVICE_INVALIDATED";
    errorCodes[AUDCLNT_E_DEVICE_IN_USE] = "AUDCLNT_E_DEVICE_IN_USE";
    errorCodes[AUDCLNT_E_ENDPOINT_CREATE_FAILED] = "AUDCLNT_E_ENDPOINT_CREATE_FAILED";
    errorCodes[AUDCLNT_E_INVALID_DEVICE_PERIOD] = "AUDCLNT_E_INVALID_DEVICE_PERIOD";
    errorCodes[AUDCLNT_E_UNSUPPORTED_FORMAT] = "AUDCLNT_E_UNSUPPORTED_FORMAT";
    errorCodes[AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED] = "AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED";
    errorCodes[AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL] = "AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL";
    errorCodes[AUDCLNT_E_SERVICE_NOT_RUNNING] = "AUDCLNT_E_SERVICE_NOT_RUNNING";
    errorCodes[AUDCLNT_E_NOT_INITIALIZED] = "AUDCLNT_E_NOT_INITIALIZED";
    errorCodes[AUDCLNT_E_NOT_STOPPED] = "AUDCLNT_E_NOT_STOPPED";
    errorCodes[AUDCLNT_E_EVENTHANDLE_NOT_SET] = "AUDCLNT_E_EVENTHANDLE_NOT_SET";
    errorCodes[AUDCLNT_E_INVALID_SIZE] = "AUDCLNT_E_INVALID_SIZE";
    errorCodes[AUDCLNT_E_OUT_OF_ORDER] = "AUDCLNT_E_OUT_OF_ORDER";
    errorCodes[AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED] = "AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED";
    errorCodes[AUDCLNT_E_BUFFER_ERROR] = "AUDCLNT_E_BUFFER_ERROR";
    errorCodes[AUDCLNT_E_BUFFER_TOO_LARGE] = "AUDCLNT_E_BUFFER_TOO_LARGE";
    errorCodes[AUDCLNT_E_BUFFER_OPERATION_PENDING] = "AUDCLNT_E_BUFFER_OPERATION_PENDING";

    return errorCodes;
}

Q_GLOBAL_STATIC_WITH_ARGS(ErrorCodesMap, errorCodes, (initErrorCodesMap()))

typedef QMap<QString, AkAudioCaps::SampleFormat> SampleFormatsMap;

inline SampleFormatsMap initSampleFormatsMap()
{
    SampleFormatsMap sampleFormats;
    sampleFormats[QString("%1_%2").arg(WAVE_FORMAT_PCM).arg(8)] = AkAudioCaps::SampleFormat_u8;
    sampleFormats[QString("%1_%2").arg(WAVE_FORMAT_PCM).arg(16)] = AkAudioCaps::SampleFormat_s16;
    sampleFormats[QString("%1_%2").arg(WAVE_FORMAT_PCM).arg(32)] = AkAudioCaps::SampleFormat_s32;
    sampleFormats[QString("%1_%2").arg(WAVE_FORMAT_IEEE_FLOAT).arg(32)] = AkAudioCaps::SampleFormat_flt;

    return sampleFormats;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatsMap, sampleFormats, (initSampleFormatsMap()))

AudioDev::AudioDev(QObject *parent):
    QObject(parent)
{
    this->m_pEnumerator = NULL;
    this->m_pDevice = NULL;
    this->m_pAudioClient = NULL;
    this->m_pCaptureClient = NULL;
    this->m_pRenderClient = NULL;
    this->m_hEvent = NULL;
}

AudioDev::~AudioDev()
{
    this->uninit();
}

QString AudioDev::error() const
{
    return this->m_error;
}

// Get native format for the default audio device.
bool AudioDev::preferredFormat(DeviceMode mode,
                                  AkAudioCaps::SampleFormat *sampleFormat,
                                  int *channels,
                                  int *sampleRate)
{
    // Test if the device is already running,
    bool isActivated = this->m_pAudioClient? true: false;

    // if not activate it and get an audio client instance.
    if (!isActivated)
        if (!this->init(mode, AkAudioCaps::SampleFormat_none, 0, 0, true))
            return false;

    HRESULT hr;
    WAVEFORMATEX *pwfx = NULL;

    // Get default format.
    if (FAILED(hr = this->m_pAudioClient->GetMixFormat(&pwfx))) {
        this->m_error = "GetMixFormat: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);

        if (!isActivated)
            this->uninit();

        return false;
    }

    // Convert device format to a supported one.
    WORD formatTag = pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT?
                         WAVE_FORMAT_IEEE_FLOAT:
                         WAVE_FORMAT_PCM;

    QString fmtStr = QString("%1_%2")
                     .arg(formatTag)
                     .arg(pwfx->wBitsPerSample);

    *sampleFormat = sampleFormats->value(fmtStr, AkAudioCaps::SampleFormat_u8);
    *channels = pwfx->nChannels;
    *sampleRate = int(pwfx->nSamplesPerSec);

    if (!isActivated)

    if (!isActivated) {
        // Stop audio device if required.
        this->uninit();

        // Workaround for buggy drivers. Test if format is really supported.
        if (!this->init(mode, *sampleFormat, *channels, *sampleRate)) {
            // Test sample formats from highest sample resolusion to lowest.
            QVector<AkAudioCaps::SampleFormat> preferredFormats;
            preferredFormats << AkAudioCaps::SampleFormat_flt;
            preferredFormats << AkAudioCaps::SampleFormat_s32;
            preferredFormats << AkAudioCaps::SampleFormat_s16;
            preferredFormats << AkAudioCaps::SampleFormat_u8;

            foreach (AkAudioCaps::SampleFormat format, preferredFormats)
                if (this->init(mode, format, *channels, *sampleRate)) {
                    *sampleFormat = format;

                    break;
                }
        }

        this->uninit();
    }

    return true;
}

bool AudioDev::init(DeviceMode mode,
                    AkAudioCaps::SampleFormat sampleFormat,
                    int channels,
                    int sampleRate,
                    bool justActivate)
{
    HRESULT hr;

    // Clear audio buffer.
    this->m_audioBuffer.clear();

    // Get device enumerator.
    if (FAILED(hr = CoCreateInstance(CLSID_MMDeviceEnumerator,
                                     NULL,
                                     CLSCTX_ALL,
                                     IID_IMMDeviceEnumerator,
                                     reinterpret_cast<void **>(&this->m_pEnumerator)))) {
        this->m_error = "CoCreateInstance: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);

        return false;
    }

    // Get default input/output audio device.
    if (FAILED(hr = this->m_pEnumerator->GetDefaultAudioEndpoint(mode == DeviceModeCapture?
                                                                     eCapture: eRender,
                                                                 eMultimedia,
                                                                 &this->m_pDevice))) {
        this->m_error = "GetDefaultAudioEndpoint: " + errorCodes->value(hr);
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

    /* Get the minimum size of the buffer in 100-nanosecond units,
       this means you must do:

       bufferSize(seconds) = 100e-9 * hnsRequestedDuration

       to get the size of the buffer in seconds.
    */
    REFERENCE_TIME hnsRequestedDuration;
    this->m_pAudioClient->GetDevicePeriod(NULL, &hnsRequestedDuration);

    // Accumulate a minimum of 1 sec. of audio in the buffer.
    REFERENCE_TIME minDuration = 10e6;

    if (hnsRequestedDuration < minDuration)
        hnsRequestedDuration = minDuration;

    int bps = AkAudioCaps::bitsPerSample(sampleFormat);

    // Set audio device format.
    WAVEFORMATEX wfx;
    wfx.wFormatTag = sampleFormat == AkAudioCaps::SampleFormat_flt?
                         WAVE_FORMAT_IEEE_FLOAT: WAVE_FORMAT_PCM;
    wfx.nChannels = WORD(channels);
    wfx.nSamplesPerSec = DWORD(sampleRate);
    wfx.wBitsPerSample = WORD(bps);
    wfx.nBlockAlign = WORD(channels * bps / 8);
    wfx.nAvgBytesPerSec = DWORD(sampleRate * wfx.nBlockAlign);
    wfx.cbSize = 0;

    this->m_curBps = bps / 8;
    this->m_curChannels = channels;

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
    if (mode == DeviceModeCapture)
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
                     * this->m_curBps
                     * this->m_curChannels;

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

        size_t bufferSize = samplesCount * size_t(this->m_curBps * this->m_curChannels);

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
    this->m_audioBuffer = frame;
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
                                        / this->m_curBps
                                        / this->m_curChannels);
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
                            * this->m_curBps
                            * this->m_curChannels;

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

    if (this->m_pEnumerator) {
        this->m_pEnumerator->Release();
        this->m_pEnumerator = NULL;
    }

    if (this->m_hEvent) {
        CloseHandle(this->m_hEvent);
        this->m_hEvent = NULL;
    }

    return ok;
}
