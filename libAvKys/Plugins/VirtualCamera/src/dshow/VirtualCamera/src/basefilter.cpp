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

#include <algorithm>
#include <dshow.h>

#include "basefilter.h"
#include "enumpins.h"
#include "filtermiscflags.h"
#include "pin.h"
#include "referenceclock.h"
#include "specifypropertypages.h"
#include "videocontrol.h"
#include "videoprocamp.h"
#include "utils.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/utils.h"

#define AK_CUR_INTERFACE "BaseFilter"

namespace AkVCam
{
    class BaseFilterPrivate
    {
        public:
            EnumPins *m_pins;
            VideoProcAmp *m_videoProcAmp;
            ReferenceClock *m_referenceClock;
            std::wstring m_vendor;
            std::wstring m_filterName;
            IFilterGraph *m_filterGraph;
    };
}

AkVCam::BaseFilter::BaseFilter(const GUID &clsid,
                               const std::wstring &filterName,
                               const std::wstring &vendor):
    MediaFilter(clsid, this)
{
    this->setParent(this, &IID_IBaseFilter);
    this->d = new BaseFilterPrivate;
    this->d->m_pins = new AkVCam::EnumPins;
    this->d->m_pins->AddRef();
    this->d->m_videoProcAmp = new VideoProcAmp;
    this->d->m_videoProcAmp->AddRef();
    this->d->m_referenceClock = new ReferenceClock;
    this->d->m_referenceClock->AddRef();
    this->d->m_vendor = vendor;
    this->d->m_filterName = filterName;
    this->d->m_filterGraph = nullptr;
}

AkVCam::BaseFilter::~BaseFilter()
{
    this->d->m_pins->setBaseFilter(nullptr);
    this->d->m_pins->Release();
    this->d->m_videoProcAmp->Release();
    this->d->m_referenceClock->Release();

    delete this->d;
}

void AkVCam::BaseFilter::addPin(const std::vector<AkVCam::VideoFormat> &formats,
                                const std::wstring &pinName,
                                bool changed)
{
    this->d->m_pins->addPin(new Pin(this, formats, pinName), changed);
}

void AkVCam::BaseFilter::removePin(IPin *pin, bool changed)
{
    this->d->m_pins->removePin(pin, changed);
}

AkVCam::BaseFilter *AkVCam::BaseFilter::create(const GUID &clsid)
{
    auto camera = cameraFromId(clsid);

    if (camera < 0)
        return nullptr;

    auto description = cameraDescription(DWORD(camera));
    auto baseFilter = new BaseFilter(clsid,
                                     description,
                                     DSHOW_PLUGIN_VENDOR_L);
    auto formats = cameraFormats(DWORD(camera));
    baseFilter->addPin(formats, L"Video", false);

    return baseFilter;
}

IFilterGraph *AkVCam::BaseFilter::filterGraph() const
{
    return this->d->m_filterGraph;
}

IReferenceClock *AkVCam::BaseFilter::referenceClock() const
{
    return this->d->m_referenceClock;
}

HRESULT AkVCam::BaseFilter::QueryInterface(const IID &riid, void **ppvObject)
{
    AkLogMethod();
    AkLoggerLog("IID: ", AkVCam::stringFromClsid(riid));

    if (!ppvObject)
        return E_POINTER;

    *ppvObject = nullptr;

    if (IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IBaseFilter)
        || IsEqualIID(riid, IID_IMediaFilter)) {
        AkLogInterface(IBaseFilter, this);
        this->AddRef();
        *ppvObject = this;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IAMFilterMiscFlags)) {
        auto filterMiscFlags = new FilterMiscFlags;
        AkLogInterface(IAMFilterMiscFlags, filterMiscFlags);
        filterMiscFlags->AddRef();
        *ppvObject = filterMiscFlags;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IAMVideoControl)) {
        IEnumPins *pins = nullptr;
        this->d->m_pins->Clone(&pins);
        auto videoControl = new VideoControl(pins);
        pins->Release();
        AkLogInterface(IAMVideoControl, videoControl);
        videoControl->AddRef();
        *ppvObject = videoControl;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IAMVideoProcAmp)) {
        auto videoProcAmp = this->d->m_videoProcAmp;
        AkLogInterface(IAMVideoProcAmp, videoProcAmp);
        videoProcAmp->AddRef();
        *ppvObject = videoProcAmp;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IReferenceClock)) {
        auto referenceClock = this->d->m_referenceClock;
        AkLogInterface(IReferenceClock, referenceClock);
        referenceClock->AddRef();
        *ppvObject = referenceClock;

        return S_OK;
    } else if (IsEqualIID(riid, IID_ISpecifyPropertyPages)) {
        this->d->m_pins->Reset();
        IPin *pin = nullptr;
        this->d->m_pins->Next(1, &pin, nullptr);
        auto specifyPropertyPages = new SpecifyPropertyPages(pin);
        pin->Release();
        AkLogInterface(ISpecifyPropertyPages, specifyPropertyPages);
        specifyPropertyPages->AddRef();
        *ppvObject = specifyPropertyPages;

        return S_OK;
    } else {
        this->d->m_pins->Reset();
        IPin *pin = nullptr;
        this->d->m_pins->Next(1, &pin, nullptr);
        auto result = pin->QueryInterface(riid, ppvObject);
        pin->Release();

        if (SUCCEEDED(result))
            return result;
    }

    return MediaFilter::QueryInterface(riid, ppvObject);
}

HRESULT AkVCam::BaseFilter::EnumPins(IEnumPins **ppEnum)
{
    AkLogMethod();
    auto result = this->d->m_pins->Clone(ppEnum);

    if (SUCCEEDED(result))
        (*ppEnum)->Reset();

    return result;
}

HRESULT AkVCam::BaseFilter::FindPin(LPCWSTR Id, IPin **ppPin)
{
    AkLogMethod();

    if (!ppPin)
        return E_POINTER;

    *ppPin = nullptr;

    if (!Id)
        return VFW_E_NOT_FOUND;

    IPin *pin = nullptr;
    HRESULT result = VFW_E_NOT_FOUND;
    this->d->m_pins->Reset();

    while (this->d->m_pins->Next(1, &pin, nullptr) == S_OK) {
        WCHAR *pinId = nullptr;
        auto ok = pin->QueryId(&pinId);

        if (ok == S_OK && wcscmp(pinId, Id) == 0) {
            *ppPin = pin;
            (*ppPin)->AddRef();
            result = S_OK;
        }

        CoTaskMemFree(pinId);
        pin->Release();
        pin = nullptr;

        if (result == S_OK)
            break;
    }

    return result;
}

HRESULT AkVCam::BaseFilter::QueryFilterInfo(FILTER_INFO *pInfo)
{
    AkLogMethod();

    if (!pInfo)
        return E_POINTER;

    memset(pInfo->achName, 0, MAX_FILTER_NAME * sizeof(WCHAR));

    if (this->d->m_filterName.size() > 0) {
        memcpy(pInfo->achName,
               this->d->m_filterName.c_str(),
               std::max<size_t>(this->d->m_filterName.size() * sizeof(WCHAR),
                                MAX_FILTER_NAME));
    }

    pInfo->pGraph = this->d->m_filterGraph;

    if (pInfo->pGraph)
        pInfo->pGraph->AddRef();

    return S_OK;
}

HRESULT AkVCam::BaseFilter::JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName)
{
    AkLogMethod();

    this->d->m_filterGraph = pGraph;
    this->d->m_filterName = std::wstring(pName? pName: L"");

    AkLoggerLog("Filter graph: ", this->d->m_filterGraph);
    AkLoggerLog("Name: ", std::string(this->d->m_filterName.begin(),
                                      this->d->m_filterName.end()));

    return S_OK;
}

HRESULT AkVCam::BaseFilter::QueryVendorInfo(LPWSTR *pVendorInfo)
{
    AkLogMethod();

    if (this->d->m_vendor.size() < 1)
        return E_NOTIMPL;

    if (!pVendorInfo)
        return E_POINTER;

    *pVendorInfo = wcharStrFromWStr(this->d->m_vendor);

    return S_OK;
}
