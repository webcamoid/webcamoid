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

#include "persistpropertybag.h"
#include "basefilter.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

#define AK_CUR_INTERFACE "PersistPropertyBag"

namespace AkVCam
{
    class PersistPropertyBagPrivate
    {
        public:
            ComVariantMap m_properties;
    };
}

AkVCam::PersistPropertyBag::PersistPropertyBag(const GUID &clsid,
                                               const ComVariantMap &properties):
    Persist(clsid)
{
    this->setParent(this, &IID_IPersistPropertyBag);
    this->d = new PersistPropertyBagPrivate;
    this->d->m_properties = properties;
}

AkVCam::PersistPropertyBag::~PersistPropertyBag()
{
    delete this->d;
}

HRESULT AkVCam::PersistPropertyBag::QueryInterface(const IID &riid,
                                                   void **ppvObject)
{
    AkLogMethod();
    AkLoggerLog("IID: ", AkVCam::stringFromClsid(riid));

    if (!ppvObject)
        return E_POINTER;

    *ppvObject = nullptr;

    if (IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IPersistPropertyBag)) {
        AkLogInterface(IPersistPropertyBag, this);
        this->AddRef();
        *ppvObject = this;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IBaseFilter)) {
        CLSID clsid;
        auto result = this->GetClassID(&clsid);

        if (FAILED(result))
            return result;

        auto baseFilter = BaseFilter::create(clsid);

        if (!baseFilter)
            return E_FAIL;

        AkLogInterface(IBaseFilter, baseFilter);
        baseFilter->AddRef();
        *ppvObject = baseFilter;

        return S_OK;
    }

    return Persist::QueryInterface(riid, ppvObject);
}

HRESULT AkVCam::PersistPropertyBag::InitNew()
{
    AkLogMethod();

    return S_OK;
}

HRESULT AkVCam::PersistPropertyBag::Load(IPropertyBag *pPropBag,
                                         IErrorLog *pErrorLog)
{
    UNUSED(pErrorLog)
    AkLogMethod();

    if (!pPropBag)
        return E_POINTER;

    for (auto &prop: this->d->m_properties) {
        VARIANT	value;
        VariantInit(&value);

        if (FAILED(pPropBag->Read(prop.first.c_str(), &value, nullptr)))
            continue;

        this->d->m_properties[prop.first] = value;
    }

    return S_OK;
}

HRESULT AkVCam::PersistPropertyBag::Save(IPropertyBag *pPropBag,
                                         BOOL fClearDirty,
                                         BOOL fSaveAllProperties)
{
    UNUSED(fClearDirty)
    UNUSED(fSaveAllProperties)
    AkLogMethod();

    if (!pPropBag)
        return E_POINTER;

    for (auto &prop: this->d->m_properties)
        pPropBag->Write(prop.first.c_str(), &prop.second);

    return S_OK;
}
