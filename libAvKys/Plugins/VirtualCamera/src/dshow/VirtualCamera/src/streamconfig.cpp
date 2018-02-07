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

#include <strmif.h>
#include <amvideo.h>
#include <dvdmedia.h>
#include <uuids.h>
#include <dshow.h>

#include "streamconfig.h"
#include "basefilter.h"
#include "pin.h"
#include "utils.h"
#include "VCamUtils/src/utils.h"

#define AK_CUR_INTERFACE "StreamConfig"

namespace AkVCam
{
    class StreamConfigPrivate
    {
        public:
            Pin *m_pin;
            AM_MEDIA_TYPE *m_mediaType;
    };
}

AkVCam::StreamConfig::StreamConfig(Pin *pin):
    CUnknown(this, IID_IAMStreamConfig)
{
    this->d = new StreamConfigPrivate;
    this->d->m_pin = pin;
    this->d->m_mediaType = nullptr;
}

AkVCam::StreamConfig::~StreamConfig()
{
    deleteMediaType(&this->d->m_mediaType);
    delete this->d;
}

void AkVCam::StreamConfig::setPin(Pin *pin)
{
    this->d->m_pin = pin;
}

void AkVCam::StreamConfig::setMediaType(const AM_MEDIA_TYPE *mediaType)
{
    deleteMediaType(&this->d->m_mediaType);
    this->d->m_mediaType = createMediaType(mediaType);
}

HRESULT AkVCam::StreamConfig::SetFormat(AM_MEDIA_TYPE *pmt)
{
    AkLogMethod();

    if (!pmt)
        return E_POINTER;

    if (this->d->m_pin) {
        PIN_INFO pinInfo;

        if (SUCCEEDED(this->d->m_pin->QueryPinInfo(&pinInfo))
            && pinInfo.pFilter) {
            FILTER_STATE state;
            auto result = pinInfo.pFilter->GetState(0, &state);
            pinInfo.pFilter->Release();

            if (FAILED(result) || state != State_Stopped)
                return VFW_E_NOT_STOPPED;
        }

        if (FAILED(this->d->m_pin->QueryAccept(pmt)))
            return VFW_E_INVALIDMEDIATYPE;

        IPin *pin = nullptr;

        if (SUCCEEDED(this->d->m_pin->ConnectedTo(&pin))) {
            if (pin)
                pin->Release();
            else
                return VFW_E_NOT_CONNECTED;
        }
    }

    deleteMediaType(&this->d->m_mediaType);
    this->d->m_mediaType = createMediaType(pmt);

    if (this->d->m_pin
        && this->d->m_pin->baseFilter()
        && this->d->m_pin->baseFilter()->filterGraph())
        this->d->m_pin->baseFilter()->filterGraph()->Reconnect(this->d->m_pin);

    return S_OK;
}

HRESULT AkVCam::StreamConfig::GetFormat(AM_MEDIA_TYPE **pmt)
{
    AkLogMethod();

    if (!pmt)
        return E_POINTER;

    *pmt = nullptr;

    if (this->d->m_pin) {
        IPin *pin = nullptr;

        if (SUCCEEDED(this->d->m_pin->ConnectedTo(&pin))) {
            if (pin)
                pin->Release();
            else
                return VFW_E_NOT_CONNECTED;
        }
    }

    if (!this->d->m_mediaType
        || IsEqualGUID(this->d->m_mediaType->majortype, GUID_NULL))
        *pmt = nullptr;
    else
        *pmt = createMediaType(this->d->m_mediaType);

    return S_OK;
}

HRESULT AkVCam::StreamConfig::GetNumberOfCapabilities(int *piCount,
                                                      int *piSize)
{
    AkLogMethod();

    if (!piCount || !piSize)
        return E_POINTER;

    *piCount = 0;
    *piSize = 0;

    if (this->d->m_pin) {
        IPin *pin = nullptr;

        if (SUCCEEDED(this->d->m_pin->ConnectedTo(&pin))) {
            if (pin)
                pin->Release();
            else
                return VFW_E_NOT_CONNECTED;
        }

        IEnumMediaTypes *mediaTypes = nullptr;

        if (SUCCEEDED(this->d->m_pin->EnumMediaTypes(&mediaTypes))) {
            mediaTypes->Reset();
            AM_MEDIA_TYPE *mediaType = nullptr;

            while (SUCCEEDED(mediaTypes->Next(1, &mediaType, nullptr))) {
                (*piCount)++;
                deleteMediaType(&mediaType);
            }

            mediaTypes->Release();
        }
    }

    if (*piCount)
        *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);

    return S_OK;
}

HRESULT AkVCam::StreamConfig::GetStreamCaps(int iIndex,
                                            AM_MEDIA_TYPE **pmt,
                                            BYTE *pSCC)
{
    AkLogMethod();

    if (!pmt || !pSCC)
        return E_POINTER;

    *pmt = nullptr;
    auto configCaps = reinterpret_cast<VIDEO_STREAM_CONFIG_CAPS *>(pSCC);
    memset(&configCaps, 0, sizeof(VIDEO_STREAM_CONFIG_CAPS));

    if (iIndex < 0)
        return E_INVALIDARG;

    if (this->d->m_pin) {
        IPin *pin = nullptr;

        if (SUCCEEDED(this->d->m_pin->ConnectedTo(&pin))) {
            if (pin)
                pin->Release();
            else
                return VFW_E_NOT_CONNECTED;
        }

        IEnumMediaTypes *mediaTypes = nullptr;

        if (SUCCEEDED(this->d->m_pin->EnumMediaTypes(&mediaTypes))) {
            mediaTypes->Reset();
            AM_MEDIA_TYPE *mediaType = nullptr;

            for (int i = 0;
                 SUCCEEDED(mediaTypes->Next(1, &mediaType, nullptr));
                 i++) {
                if (i == iIndex) {
                    *pmt = mediaType;
                    configCaps->guid = mediaType->formattype;

                    if (IsEqualGUID(configCaps->guid, FORMAT_VideoInfo)) {
                        auto format = reinterpret_cast<VIDEOINFOHEADER *>(mediaType->pbFormat);
                        configCaps->MinFrameInterval = format->AvgTimePerFrame;
                        configCaps->MaxFrameInterval = format->AvgTimePerFrame;
                    } else if (IsEqualGUID(configCaps->guid, FORMAT_VideoInfo2)) {
                        auto format = reinterpret_cast<VIDEOINFOHEADER2 *>(mediaType->pbFormat);
                        configCaps->MinFrameInterval = format->AvgTimePerFrame;
                        configCaps->MaxFrameInterval = format->AvgTimePerFrame;
                    }

                    break;
                }

                deleteMediaType(&mediaType);
            }

            mediaTypes->Release();
        }
    }

    if (!pmt)
        return S_FALSE;

    return S_OK;
}
