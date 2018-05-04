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

#include "pushsource.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

#define AK_CUR_INTERFACE "PushSource"

AkVCam::PushSource::PushSource(IAMStreamConfig *streamConfig):
    Latency(streamConfig)
{
    this->setParent(this, &IID_IAMPushSource);
}

AkVCam::PushSource::~PushSource()
{
}

HRESULT AkVCam::PushSource::GetPushSourceFlags(ULONG *pFlags)
{
    AkLogMethod();

    if (!pFlags)
        return E_POINTER;

    *pFlags = 0;

    return S_OK;
}

HRESULT AkVCam::PushSource::SetPushSourceFlags(ULONG Flags)
{
    UNUSED(Flags)
    AkLogMethod();

    return E_NOTIMPL;
}

HRESULT AkVCam::PushSource::SetStreamOffset(REFERENCE_TIME rtOffset)
{
    UNUSED(rtOffset)
    AkLogMethod();

    return E_NOTIMPL;
}

HRESULT AkVCam::PushSource::GetStreamOffset(REFERENCE_TIME *prtOffset)
{
    UNUSED(prtOffset)
    AkLogMethod();

    return E_NOTIMPL;
}

HRESULT AkVCam::PushSource::GetMaxStreamOffset(REFERENCE_TIME *prtMaxOffset)
{
    UNUSED(prtMaxOffset)
    AkLogMethod();

    return E_NOTIMPL;
}

HRESULT AkVCam::PushSource::SetMaxStreamOffset(REFERENCE_TIME rtMaxOffset)
{
    UNUSED(rtMaxOffset)
    AkLogMethod();

    return E_NOTIMPL;
}
