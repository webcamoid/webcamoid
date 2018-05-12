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

#include "qualitycontrol.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

#define AK_CUR_INTERFACE "QualityControl"

AkVCam::QualityControl::QualityControl():
    CUnknown(this, IID_IQualityControl)
{

}

AkVCam::QualityControl::~QualityControl()
{

}

HRESULT AkVCam::QualityControl::Notify(IBaseFilter *pSelf, Quality q)
{
    UNUSED(q)
    AkLogMethod();

    if (!pSelf)
        return E_POINTER;

    AkLoggerLog("Type: ", q.Type == Famine? "Famine": "Flood");
    AkLoggerLog("Proportion: ", q.Proportion);
    AkLoggerLog("Late: ", q.Late);
    AkLoggerLog("TimeStamp:", q.TimeStamp);

    return S_OK;
}

HRESULT AkVCam::QualityControl::SetSink(IQualityControl *piqc)
{
    UNUSED(piqc)
    AkLogMethod();

    return E_NOTIMPL;
}
