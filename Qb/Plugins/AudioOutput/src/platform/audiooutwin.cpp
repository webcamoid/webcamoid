/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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

#include <QMap>

#include "platform/audiooutwin.h"

typedef QMap<HRESULT, QString> ErrorCodesMap;

inline ErrorCodesMap initErrorCodesMap()
{
    ErrorCodesMap errorCodes;
    errorCodes[XAUDIO2_E_INVALID_CALL] = "XAUDIO2_E_INVALID_CALL";
    errorCodes[XAUDIO2_E_XMA_DECODER_ERROR] = "XAUDIO2_E_XMA_DECODER_ERROR";
    errorCodes[XAUDIO2_E_XAPO_CREATION_FAILED] = "XAUDIO2_E_XAPO_CREATION_FAILED";
    errorCodes[XAUDIO2_E_DEVICE_INVALIDATED] = "XAUDIO2_E_DEVICE_INVALIDATED";

    return errorCodes;
}

Q_GLOBAL_STATIC_WITH_ARGS(ErrorCodesMap, errorCodes, (initErrorCodesMap()))

typedef QMap<QbAudioCaps::SampleFormat, int> BytesPerSampleMap;

inline BytesPerSampleMap initBytesPerSampleMap()
{
    BytesPerSampleMap bytesPerSample;
    bytesPerSample[QbAudioCaps::SampleFormat_u8] = 1;
    bytesPerSample[QbAudioCaps::SampleFormat_s16] = 2;
    bytesPerSample[QbAudioCaps::SampleFormat_s32] = 4;
    bytesPerSample[QbAudioCaps::SampleFormat_flt] = 4;

    return bytesPerSample;
}

Q_GLOBAL_STATIC_WITH_ARGS(BytesPerSampleMap, bytesPerSample, (initBytesPerSampleMap()))

AudioOut::AudioOut(QObject *parent):
    QObject(parent)
{
    this->m_pXAudio2 = NULL;
    this->m_pMasterVoice = NULL;
    this->m_pSourceVoice = NULL;
}

AudioOut::~AudioOut()
{
    this->uninit();
}

QString AudioOut::error() const
{
    return this->m_error;
}

bool AudioOut::init(QbAudioCaps::SampleFormat sampleFormat,
                    int channels,
                    int sampleRate)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    HRESULT hr;

    if (FAILED(hr = XAudio2Create(&this->m_pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        this->m_error = "XAudio2Create: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);
        CoUninitialize();

        return false;
    }

    if (FAILED(hr = this->m_pXAudio2->CreateMasteringVoice(&this->m_pMasterVoice,
                                                           channels,
                                                           sampleRate))) {
        this->m_error = "CreateMasteringVoice: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);
        this->m_pXAudio2->Release();
        this->m_pXAudio2 = NULL;
        CoUninitialize();

        return false;
    }

    int bps = bytesPerSample->value(sampleFormat);

    WAVEFORMATEX wfx;
    wfx.wFormatTag = sampleFormat == QbAudioCaps::SampleFormat_flt?
                         WAVE_FORMAT_IEEE_FLOAT: WAVE_FORMAT_PCM;
    wfx.nChannels = channels;
    wfx.nSamplesPerSec = sampleRate;
    wfx.wBitsPerSample = 8 * bps;
    wfx.nBlockAlign = channels * bps;
    wfx.nAvgBytesPerSec = sampleRate * wfx.nBlockAlign;
    wfx.cbSize = 0;

    if (FAILED(hr = this->m_pXAudio2->CreateSourceVoice(&this->m_pSourceVoice,
                                                        (WAVEFORMATEX *) &wfx,
                                                        0,
                                                        XAUDIO2_DEFAULT_FREQ_RATIO,
                                                        &this->m_voiceCallbacks))) {
        this->m_error = "CreateSourceVoice: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);
        this->m_pXAudio2->Release();
        this->m_pMasterVoice = NULL;
        this->m_pXAudio2 = NULL;
        CoUninitialize();

        return false;
    }

    if (FAILED(hr = this->m_pSourceVoice->Start())) {
        this->m_error = "Start: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);
        this->m_pXAudio2->Release();
        this->m_pSourceVoice = NULL;
        this->m_pMasterVoice = NULL;
        this->m_pXAudio2 = NULL;
        CoUninitialize();

        return false;
    }

    return true;
}

bool AudioOut::write(const QByteArray &frame)
{
    XAUDIO2_BUFFER xAudioBuffer;
    ZeroMemory(&xAudioBuffer, sizeof(XAUDIO2_BUFFER));
    xAudioBuffer.AudioBytes = frame.size();
    xAudioBuffer.pAudioData = (const BYTE *) frame.data();
    xAudioBuffer.pContext = &this->m_lock;

    HRESULT hr;

    if (FAILED(hr = this->m_pSourceVoice->SubmitSourceBuffer(&xAudioBuffer))) {
        this->m_error = "SubmitSourceBuffer: " + errorCodes->value(hr);
        emit this->errorChanged(this->m_error);

        return false;
    }

    this->m_lock.m_mutex.lock();
    this->m_lock.m_waitForBufferEnd.wait(&this->m_lock.m_mutex);
    this->m_lock.m_mutex.unlock();

    return true;
}

bool AudioOut::uninit()
{
    if (this->m_pSourceVoice) {
        this->m_pSourceVoice->Discontinuity();

        XAUDIO2_VOICE_STATE state;
        this->m_pSourceVoice->GetState(&state);

        while (state.BuffersQueued > 0) {
            this->m_lock.m_mutex.lock();
            this->m_lock.m_waitForBufferEnd.wait(&this->m_lock.m_mutex);
            this->m_lock.m_mutex.unlock();
            this->m_pSourceVoice->GetState(&state);
        }

        this->m_pSourceVoice->Stop(0, XAUDIO2_COMMIT_NOW);
        this->m_pSourceVoice->FlushSourceBuffers();
        this->m_pSourceVoice->DestroyVoice();

        this->m_pSourceVoice = NULL;
    }

    if (this->m_pXAudio2)
        this->m_pXAudio2->StopEngine();

    if (this->m_pMasterVoice) {
        this->m_pMasterVoice->DestroyVoice();
        this->m_pMasterVoice = NULL;
    }

    if (this->m_pXAudio2) {
        this->m_pXAudio2->Release();
        this->m_pXAudio2 = NULL;
        CoUninitialize();
    }

    return true;
}
