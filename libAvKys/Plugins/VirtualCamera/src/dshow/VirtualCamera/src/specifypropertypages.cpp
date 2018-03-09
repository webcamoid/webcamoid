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

#include <vector>
#include <ddraw.h>
#include <initguid.h>
#include <uuids.h>

#include "specifypropertypages.h"
#include "utils.h"
#include "VCamUtils/src/utils.h"

#define AK_CUR_INTERFACE "SpecifyPropertyPages"

namespace AkVCam
{
    class SpecifyPropertyPagesPrivate
    {
        public:
            IPin *m_pin;
    };
}

AkVCam::SpecifyPropertyPages::SpecifyPropertyPages(IPin *pin):
    CUnknown(this, IID_ISpecifyPropertyPages)
{
    this->d = new SpecifyPropertyPagesPrivate;
    this->d->m_pin = pin;
    this->d->m_pin->AddRef();
}

AkVCam::SpecifyPropertyPages::~SpecifyPropertyPages()
{
    this->d->m_pin->Release();
    delete this->d;
}

HRESULT AkVCam::SpecifyPropertyPages::GetPages(CAUUID *pPages)
{
    AkLogMethod();

    if (!pPages)
        return E_POINTER;

    std::vector<GUID> pages {
        CLSID_VideoProcAmpPropertyPage,
    };

    IPin *pin = nullptr;

    if (SUCCEEDED(this->d->m_pin->ConnectedTo(&pin))) {
        pages.push_back(CLSID_VideoStreamConfigPropertyPage);
        pin->Release();
    }

    pPages->cElems = pages.size();
    pPages->pElems =
            reinterpret_cast<GUID *>(CoTaskMemAlloc(sizeof(GUID) * pages.size()));
    AkLoggerLog("Returning property pages:");

    for (size_t i = 0; i < pages.size(); i++) {
        memcpy(&pPages->pElems[i], &pages[i], sizeof(GUID));
        AkLoggerLog("    ", stringFromClsid(pages[i]));
    }

    return S_OK;
}
