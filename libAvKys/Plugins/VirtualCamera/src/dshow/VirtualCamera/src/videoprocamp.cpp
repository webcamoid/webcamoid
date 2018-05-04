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

#include <map>
#include <vector>
#include <dshow.h>

#include "videoprocamp.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

#define AK_CUR_INTERFACE "VideoProcAmp"

namespace AkVCam
{
    class VideoProcAmpPrivate
    {
        public:
            std::map<LONG, LONG> m_control;
    };

    class ProcAmpPrivate
    {
        public:
            LONG property;
            LONG min;
            LONG max;
            LONG step;
            LONG defaultValue;
            LONG flags;

            inline static const std::vector<ProcAmpPrivate> &controls()
            {
                static const std::vector<ProcAmpPrivate> controls {
                    {VideoProcAmp_Brightness , -255, 255, 1, 0, CameraControl_Flags_Manual},
                    {VideoProcAmp_Contrast   , -255, 255, 1, 0, CameraControl_Flags_Manual},
                    {VideoProcAmp_Saturation , -255, 255, 1, 0, CameraControl_Flags_Manual},
                    {VideoProcAmp_Gamma      , -255, 255, 1, 0, CameraControl_Flags_Manual},
                    {VideoProcAmp_Hue        , -359, 359, 1, 0, CameraControl_Flags_Manual},
                    {VideoProcAmp_ColorEnable,    0,   1, 1, 1, CameraControl_Flags_Manual}
                };

                return controls;
            }

            static inline const ProcAmpPrivate *byProperty(LONG property)
            {
                for (auto &control: controls())
                    if (control.property == property)
                        return &control;

                return nullptr;
            }
    };
}

AkVCam::VideoProcAmp::VideoProcAmp():
    CUnknown(this, IID_IAMVideoProcAmp)
{
    this->d = new VideoProcAmpPrivate;
}

AkVCam::VideoProcAmp::~VideoProcAmp()
{
    delete this->d;
}

HRESULT AkVCam::VideoProcAmp::GetRange(LONG Property,
                                       LONG *pMin,
                                       LONG *pMax,
                                       LONG *pSteppingDelta,
                                       LONG *pDefault,
                                       LONG *pCapsFlags)
{
    AkLogMethod();

    if (!pMin || !pMax || !pSteppingDelta || !pDefault || !pCapsFlags)
        return E_POINTER;

    *pMin = 0;
    *pMax = 0;
    *pSteppingDelta = 0;
    *pDefault = 0;
    *pCapsFlags = 0;

    for (auto &control: ProcAmpPrivate::controls())
        if (control.property == Property) {
            *pMin = control.min;
            *pMax = control.max;
            *pSteppingDelta = control.step;
            *pDefault = control.defaultValue;
            *pCapsFlags = control.flags;

            return S_OK;
        }

    return E_PROP_ID_UNSUPPORTED;
}

HRESULT AkVCam::VideoProcAmp::Set(LONG Property, LONG lValue, LONG Flags)
{
    AkLogMethod();

    for (auto &control: ProcAmpPrivate::controls())
        if (control.property == Property) {
            if (lValue < control.min
                || lValue >= control.max
                || Flags != control.flags)
                return E_INVALIDARG;

            this->d->m_control[Property] = lValue;

            return S_OK;
        }

    return E_PROP_ID_UNSUPPORTED;
}

HRESULT AkVCam::VideoProcAmp::Get(LONG Property, LONG *lValue, LONG *Flags)
{
    AkLogMethod();

    if (!lValue || !Flags)
        return E_POINTER;

    *lValue = 0;
    *Flags = 0;

    for (auto &control: ProcAmpPrivate::controls())
        if (control.property == Property) {
            if (this->d->m_control.count(Property))
                *lValue = this->d->m_control[Property];
            else
                *lValue = control.defaultValue;

            *Flags = control.flags;

            return S_OK;
        }

    return E_PROP_ID_UNSUPPORTED;
}
