/* Webcamoid, webcam capture application.
 * Copyright (C) 2018  Gonzalo Exequiel Pedone
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

#include <cmath>
#include <strmif.h>
#include <amvideo.h>
#include <dvdmedia.h>
#include <uuids.h>

#include "videocontrol.h"
#include "enumpins.h"
#include "pin.h"
#include "referenceclock.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"
#include "VCamUtils/src/image/videoformat.h"

#define AK_CUR_INTERFACE "VideoControl"

namespace AkVCam
{
    class VideoControlPrivate
    {
        public:
            IEnumPins *m_enumPins;
    };
}

AkVCam::VideoControl::VideoControl(IEnumPins *enumPins):
    CUnknown(this, IID_IAMVideoControl)
{
    this->d = new VideoControlPrivate;
    this->d->m_enumPins = enumPins;
    this->d->m_enumPins->AddRef();
}

AkVCam::VideoControl::~VideoControl()
{
    this->d->m_enumPins->Release();
    delete this->d;
}

HRESULT AkVCam::VideoControl::GetCaps(IPin *pPin, LONG *pCapsFlags)
{
    AkLogMethod();

    if (!pPin || !pCapsFlags)
        return E_POINTER;

    *pCapsFlags = 0;
    this->d->m_enumPins->Reset();
    HRESULT result = E_FAIL;
    IPin *pin = nullptr;

    while (this->d->m_enumPins->Next(1, &pin, nullptr) == S_OK) {
        if (pin == pPin) {
            *pCapsFlags = VideoControlFlag_FlipHorizontal
                        | VideoControlFlag_FlipVertical;
            result = S_OK;
            pin->Release();

            break;
        }

        pin->Release();
    }

    return result;
}

HRESULT AkVCam::VideoControl::SetMode(IPin *pPin, LONG Mode)
{
    AkLogMethod();

    if (!pPin)
        return E_POINTER;

    this->d->m_enumPins->Reset();
    HRESULT result = E_FAIL;
    IPin *pin = nullptr;

    while (this->d->m_enumPins->Next(1, &pin, nullptr) == S_OK) {
        if (pin == pPin) {
            auto cpin = dynamic_cast<Pin *>(pin);
            cpin->setHorizontalFlip(bool(Mode & VideoControlFlag_FlipHorizontal));
            cpin->setVerticalFlip(bool(Mode & VideoControlFlag_FlipVertical));
            result = S_OK;
            pin->Release();

            break;
        }

        pin->Release();
    }

    return result;
}

HRESULT AkVCam::VideoControl::GetMode(IPin *pPin, LONG *Mode)
{
    AkLogMethod();

    if (!pPin || !Mode)
        return E_POINTER;

    *Mode = 0;
    this->d->m_enumPins->Reset();
    HRESULT result = E_FAIL;
    IPin *pin = nullptr;

    while (this->d->m_enumPins->Next(1, &pin, nullptr) == S_OK) {
        if (pin == pPin) {
            auto cpin = dynamic_cast<Pin *>(pin);

            if (cpin->horizontalFlip())
                *Mode |= VideoControlFlag_FlipHorizontal;

            if (cpin->verticalFlip())
                *Mode |= VideoControlFlag_FlipVertical;

            result = S_OK;
            pin->Release();

            break;
        }

        pin->Release();
    }

    return result;
}

HRESULT AkVCam::VideoControl::GetCurrentActualFrameRate(IPin *pPin,
                                                        LONGLONG *ActualFrameRate)
{
    AkLogMethod();

    if (!pPin || !ActualFrameRate)
        return E_POINTER;

    *ActualFrameRate = 0;
    this->d->m_enumPins->Reset();
    HRESULT result = E_FAIL;
    IPin *pin = nullptr;

    while (this->d->m_enumPins->Next(1, &pin, nullptr) == S_OK) {
        if (pin == pPin) {
            IAMStreamConfig *streamConfig = nullptr;
            result = pin->QueryInterface(IID_IAMStreamConfig,
                                         reinterpret_cast<void **>(&streamConfig));

            if (SUCCEEDED(result)) {
                AM_MEDIA_TYPE *mediaType = nullptr;
                result = streamConfig->GetFormat(&mediaType);

                if (SUCCEEDED(result)) {
                    if (IsEqualGUID(mediaType->formattype, FORMAT_VideoInfo)) {
                        auto format = reinterpret_cast<VIDEOINFOHEADER *>(mediaType->pbFormat);
                        *ActualFrameRate = format->AvgTimePerFrame;
                    } else if (IsEqualGUID(mediaType->formattype, FORMAT_VideoInfo2)) {
                        auto format = reinterpret_cast<VIDEOINFOHEADER2 *>(mediaType->pbFormat);
                        *ActualFrameRate = format->AvgTimePerFrame;
                    } else {
                        result = E_FAIL;
                    }

                    deleteMediaType(&mediaType);
                }

                streamConfig->Release();
            }

            pin->Release();

            break;
        }

        pin->Release();
    }

    return result;
}

HRESULT AkVCam::VideoControl::GetMaxAvailableFrameRate(IPin *pPin,
                                                       LONG iIndex,
                                                       SIZE Dimensions,
                                                       LONGLONG *MaxAvailableFrameRate)
{
    AkLogMethod();

    if (!pPin || MaxAvailableFrameRate)
        return E_POINTER;

    *MaxAvailableFrameRate = 0;
    this->d->m_enumPins->Reset();
    HRESULT result = E_FAIL;
    IPin *pin = nullptr;

    while (this->d->m_enumPins->Next(1, &pin, nullptr) == S_OK) {
        if (pin == pPin) {
            IAMStreamConfig *streamConfig = nullptr;
            result = pin->QueryInterface(IID_IAMStreamConfig,
                                         reinterpret_cast<void **>(&streamConfig));

            if (SUCCEEDED(result)) {
                AM_MEDIA_TYPE *mediaType = nullptr;
                VIDEO_STREAM_CONFIG_CAPS configCaps;
                result = streamConfig->GetStreamCaps(iIndex,
                                                     &mediaType,
                                                     reinterpret_cast<BYTE *>(&configCaps));

                if (SUCCEEDED(result)) {
                    if (configCaps.MaxOutputSize.cx == Dimensions.cx
                        && configCaps.MaxOutputSize.cy == Dimensions.cy) {
                        *MaxAvailableFrameRate = configCaps.MaxFrameInterval;
                    } else {
                        result = E_FAIL;
                    }

                    deleteMediaType(&mediaType);
                }

                streamConfig->Release();
            }

            pin->Release();

            break;
        }

        pin->Release();
    }

    return result;
}

HRESULT AkVCam::VideoControl::GetFrameRateList(IPin *pPin,
                                               LONG iIndex,
                                               SIZE Dimensions,
                                               LONG *ListSize,
                                               LONGLONG **FrameRates)
{
    AkLogMethod();

    if (!pPin || !ListSize || !FrameRates)
        return E_POINTER;

    *ListSize = 0;
    *FrameRates = nullptr;
    this->d->m_enumPins->Reset();
    HRESULT result = E_FAIL;
    IPin *pin = nullptr;

    while (this->d->m_enumPins->Next(1, &pin, nullptr) == S_OK) {
        if (pin == pPin) {
            IAMStreamConfig *streamConfig = nullptr;
            result = pin->QueryInterface(IID_IAMStreamConfig,
                                         reinterpret_cast<void **>(&streamConfig));

            if (SUCCEEDED(result)) {
                AM_MEDIA_TYPE *mediaType = nullptr;
                VIDEO_STREAM_CONFIG_CAPS configCaps;
                result = streamConfig->GetStreamCaps(iIndex,
                                                     &mediaType,
                                                     reinterpret_cast<BYTE *>(&configCaps));

                if (SUCCEEDED(result)) {
                    if (configCaps.MaxOutputSize.cx == Dimensions.cx
                        && configCaps.MaxOutputSize.cy == Dimensions.cy) {
                        *ListSize = 1;
                        *FrameRates = reinterpret_cast<LONGLONG *>(CoTaskMemAlloc(sizeof(LONGLONG)));
                        **FrameRates = configCaps.MaxFrameInterval;
                    } else {
                        result = E_FAIL;
                    }

                    deleteMediaType(&mediaType);
                }

                streamConfig->Release();
            }

            pin->Release();

            break;
        }

        pin->Release();
    }

    return result;
}
