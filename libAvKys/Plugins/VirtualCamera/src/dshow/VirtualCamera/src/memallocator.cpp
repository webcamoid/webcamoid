/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include <condition_variable>
#include <mutex>
#include <vector>
#include <dshow.h>

#include "memallocator.h"
#include "mediasample.h"
#include "utils.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/utils.h"

#define AK_CUR_INTERFACE "MemAllocator"

namespace AkVCam
{
    class MemAllocatorPrivate
    {
        public:
            std::vector<MediaSample *> m_samples;
            ALLOCATOR_PROPERTIES m_properties;
            std::mutex m_mutex;
            std::condition_variable_any m_bufferReleased;
            bool m_commited;
            bool m_decommiting;
    };
}

AkVCam::MemAllocator::MemAllocator():
    CUnknown(this, IID_IMemAllocator)
{
    this->d = new MemAllocatorPrivate;
    memset(&this->d->m_properties, 0, sizeof(ALLOCATOR_PROPERTIES));
    this->d->m_commited = false;
    this->d->m_decommiting = false;
}

AkVCam::MemAllocator::~MemAllocator()
{
    for (auto &sample: this->d->m_samples)
        sample->Release();

    delete this->d;
}

HRESULT AkVCam::MemAllocator::SetProperties(ALLOCATOR_PROPERTIES *pRequest,
                                            ALLOCATOR_PROPERTIES *pActual)
{
    AkLogMethod();

    if (!pRequest || !pActual)
        return E_POINTER;

    if (this->d->m_commited)
        return VFW_E_ALREADY_COMMITTED;

    if (pRequest->cbAlign < 1)
        return VFW_E_BADALIGN;

    for (auto &sample:this->d->m_samples)
        if (sample->ref() > 1)
            return VFW_E_BUFFERS_OUTSTANDING;

    memcpy(&this->d->m_properties, pRequest, sizeof(ALLOCATOR_PROPERTIES));
    memcpy(pActual, &this->d->m_properties, sizeof(ALLOCATOR_PROPERTIES));

    return S_OK;
}

HRESULT AkVCam::MemAllocator::GetProperties(ALLOCATOR_PROPERTIES *pProps)
{
    AkLogMethod();

    if (!pProps)
        return E_POINTER;

    std::lock_guard<std::mutex> lock(this->d->m_mutex);
    memcpy(pProps, &this->d->m_properties, sizeof(ALLOCATOR_PROPERTIES));

    return S_OK;
}

HRESULT AkVCam::MemAllocator::Commit()
{
    AkLogMethod();

    if (this->d->m_commited)
        return S_OK;

    if (this->d->m_properties.cBuffers < 1
        || this->d->m_properties.cbBuffer < 1) {
        AkLoggerLog("Wrong memory allocator size");

        return VFW_E_SIZENOTSET;
    }

    this->d->m_samples.clear();

    for (LONG i = 0; i < this->d->m_properties.cBuffers; i++) {
        auto sample =
                new MediaSample(this,
                                this->d->m_properties.cbBuffer,
                                this->d->m_properties.cbAlign,
                                this->d->m_properties.cbPrefix);
        sample->AddRef();
        this->d->m_samples.push_back(sample);
    }

    this->d->m_commited = true;

    return S_OK;
}

HRESULT AkVCam::MemAllocator::Decommit()
{
    AkLogMethod();

    if (!this->d->m_commited)
        return S_OK;

    this->d->m_decommiting = true;
    auto totalSamples = this->d->m_samples.size();
    size_t freeSamples = 0;

    for (size_t i = 0; i < totalSamples; i++)
        if (this->d->m_samples[i]) {
            if (this->d->m_samples[i]->ref() < 2) {
                this->d->m_samples[i]->Release();
                this->d->m_samples[i] = nullptr;
                freeSamples++;
            }
        } else {
            freeSamples++;
        }

    AkLoggerLog("Free samples: " << freeSamples << "/" << totalSamples);

    if (freeSamples >= totalSamples) {
        AkLoggerLog("Decommiting");
        this->d->m_samples.clear();
        this->d->m_commited = false;
        this->d->m_decommiting = false;
    }

    return S_OK;
}

HRESULT AkVCam::MemAllocator::GetBuffer(IMediaSample **ppBuffer,
                                        REFERENCE_TIME *pStartTime,
                                        REFERENCE_TIME *pEndTime,
                                        DWORD dwFlags)
{
    AkLogMethod();

    if (!ppBuffer)
        return E_POINTER;

    *ppBuffer = nullptr;

    if (pStartTime)
        *pStartTime = 0;

    if (pEndTime)
        *pEndTime = 0;

    if (!this->d->m_commited || this->d->m_decommiting) {
        AkLoggerLog("Allocator not commited.");

        return VFW_E_NOT_COMMITTED;
    }

    HRESULT result = S_FALSE;

    do {
        for (auto &sample: this->d->m_samples) {
            if (sample->ref() < 2) {
                *ppBuffer = sample;
                (*ppBuffer)->AddRef();
                (*ppBuffer)->GetTime(pStartTime, pEndTime);
                result = S_OK;

                break;
            }
        }

        this->d->m_mutex.lock();

        if (result == S_FALSE) {
            if (dwFlags & AM_GBF_NOWAIT)
                result = VFW_E_TIMEOUT;
            else
                this->d->m_bufferReleased.wait(this->d->m_mutex);
        }

        this->d->m_mutex.unlock();
    } while (result == S_FALSE);

    return result;
}

HRESULT AkVCam::MemAllocator::ReleaseBuffer(IMediaSample *pBuffer)
{
    UNUSED(pBuffer)
    AkLogMethod();

    this->d->m_mutex.lock();
    this->d->m_bufferReleased.notify_one();
    this->d->m_mutex.unlock();

    return S_OK;
}
