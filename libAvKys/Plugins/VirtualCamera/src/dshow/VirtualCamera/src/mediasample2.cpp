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

#include "mediasample2.h"
#include "utils.h"
#include "VCamUtils/src/utils.h"

#define AK_CUR_INTERFACE "MediaFilter2"

namespace AkVCam
{
    class MediaSample2Private
    {
        public:
            DWORD m_data;
            DWORD m_typeSpecificFlags;
            DWORD m_sampleFlags;
            DWORD m_streamId;
            AM_MEDIA_TYPE *m_mediaType;
    };
}

AkVCam::MediaSample2::MediaSample2(IMemAllocator *memAllocator,
                                   LONG bufferSize,
                                   LONG align,
                                   LONG prefix):
    MediaSample(memAllocator, bufferSize, align, prefix)
{
    this->d = new MediaSample2Private;
    this->d->m_data = 0;
    this->d->m_typeSpecificFlags = 0;
    this->d->m_sampleFlags = 0;
    this->d->m_streamId = 0;
    this->d->m_mediaType = nullptr;
}

AkVCam::MediaSample2::~MediaSample2()
{
    deleteMediaType(&this->d->m_mediaType);

    delete this->d;
}

HRESULT AkVCam::MediaSample2::SetProperties(DWORD cbProperties,
                                            const BYTE *pbProperties)
{
    AkLogMethod();

    if (cbProperties < sizeof(AM_SAMPLE2_PROPERTIES))
        return E_INVALIDARG;

    if (!pbProperties)
        return E_POINTER;

    auto properties =
            reinterpret_cast<const AM_SAMPLE2_PROPERTIES *>(pbProperties);

    if (properties->pbBuffer || properties->cbBuffer)
        return E_INVALIDARG;

    this->d->m_data = properties->cbData;
    this->d->m_typeSpecificFlags = properties->dwTypeSpecificFlags;
    this->d->m_sampleFlags = properties->dwSampleFlags;
    this->SetDiscontinuity(this->d->m_sampleFlags & AM_SAMPLE_DATADISCONTINUITY);
    this->SetSyncPoint(this->d->m_sampleFlags & AM_SAMPLE_SPLICEPOINT);
    this->SetPreroll(this->d->m_sampleFlags & AM_SAMPLE_PREROLL);
    this->SetActualDataLength(properties->lActual);
    auto start = properties->tStart;
    auto stop = properties->tStop;
    this->SetTime(&start, &stop);
    this->SetMediaTime(&start, &stop);
    this->d->m_streamId = properties->dwStreamId;
    deleteMediaType(&this->d->m_mediaType);
    this->d->m_mediaType = createMediaType(properties->pMediaType);
    this->SetMediaType(properties->pMediaType);

    return S_OK;
}

HRESULT AkVCam::MediaSample2::GetProperties(DWORD cbProperties,
                                            BYTE *pbProperties)
{
    AkLogMethod();

    if (cbProperties < sizeof(AM_SAMPLE2_PROPERTIES))
        return E_INVALIDARG;

    if (!pbProperties)
        return E_POINTER;

    auto properties =
            reinterpret_cast<AM_SAMPLE2_PROPERTIES *>(pbProperties);

    properties->cbData = this->d->m_data;
    properties->dwTypeSpecificFlags = this->d->m_typeSpecificFlags;
    properties->dwSampleFlags = this->d->m_sampleFlags;
    properties->lActual = this->GetActualDataLength();
    this->GetTime(&properties->tStart, &properties->tStop);
    properties->dwStreamId = this->d->m_streamId;
    deleteMediaType(&this->d->m_mediaType);
    this->GetMediaType(&this->d->m_mediaType);
    properties->pMediaType = this->d->m_mediaType;
    this->GetPointer(&properties->pbBuffer);
    properties->cbBuffer = this->GetSize();

    return S_OK;
}
