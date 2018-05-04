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

#include <dshow.h>

#include "propertyset.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

#define AK_CUR_INTERFACE "PropertySet"

AkVCam::PropertySet::PropertySet():
    CUnknown(this, IID_IKsPropertySet)
{

}

AkVCam::PropertySet::~PropertySet()
{

}

HRESULT AkVCam::PropertySet::Set(const GUID &guidPropSet,
                                 DWORD dwPropID,
                                 LPVOID pInstanceData,
                                 DWORD cbInstanceData,
                                 LPVOID pPropData,
                                 DWORD cbPropData)
{
    UNUSED(guidPropSet)
    UNUSED(dwPropID)
    UNUSED(pInstanceData)
    UNUSED(cbInstanceData)
    UNUSED(pPropData)
    UNUSED(cbPropData)
    AkLogMethod();

    return E_NOTIMPL;
}

HRESULT AkVCam::PropertySet::Get(const GUID &guidPropSet,
                                 DWORD dwPropID,
                                 LPVOID pInstanceData,
                                 DWORD cbInstanceData,
                                 LPVOID pPropData,
                                 DWORD cbPropData,
                                 DWORD *pcbReturned)
{
    UNUSED(pInstanceData)
    UNUSED(cbInstanceData)
    AkLogMethod();

    if (!IsEqualGUID(guidPropSet, AMPROPSETID_Pin))
        return E_PROP_SET_UNSUPPORTED;

    if (dwPropID != AMPROPERTY_PIN_CATEGORY)
        return E_PROP_ID_UNSUPPORTED;

    if (!pPropData && !pcbReturned)
        return E_POINTER;

    if (pcbReturned)
        *pcbReturned = sizeof(GUID);

    if (!pPropData)
        return S_OK;

    if (cbPropData < sizeof(GUID))
        return E_UNEXPECTED;

    auto propData = reinterpret_cast<GUID *>(pPropData);
    *propData = PIN_CATEGORY_CAPTURE;

    return S_OK;
}

HRESULT AkVCam::PropertySet::QuerySupported(const GUID &guidPropSet,
                                            DWORD dwPropID,
                                            DWORD *pTypeSupport)
{
    AkLogMethod();

    if (!IsEqualGUID(guidPropSet, AMPROPSETID_Pin))
        return E_PROP_SET_UNSUPPORTED;

    if (dwPropID != AMPROPERTY_PIN_CATEGORY)
        return E_PROP_ID_UNSUPPORTED;

    if (pTypeSupport)
        *pTypeSupport = KSPROPERTY_SUPPORT_GET;

    return S_OK;
}
