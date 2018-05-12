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

#include "classfactory.h"
#include "basefilter.h"
#include "persistpropertybag.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

#define AK_CUR_INTERFACE "ClassFactory"

namespace AkVCam
{
    class ClassFactoryPrivate
    {
        public:
            CLSID m_clsid;
            static int m_locked;
    };

    int ClassFactoryPrivate::m_locked = 0;
}

AkVCam::ClassFactory::ClassFactory(const CLSID &clsid):
    CUnknown(this, IID_IClassFactory)
{
    this->d = new ClassFactoryPrivate;
    this->d->m_clsid = clsid;
}

AkVCam::ClassFactory::~ClassFactory()
{
    delete this->d;
}

bool AkVCam::ClassFactory::locked()
{
    return ClassFactoryPrivate::m_locked > 0;
}

HRESULT AkVCam::ClassFactory::QueryInterface(const IID &riid, void **ppvObject)
{
    AkLogMethod();
    AkLoggerLog("IID: ", AkVCam::stringFromClsid(riid));

    if (!ppvObject)
        return E_POINTER;

    *ppvObject = nullptr;

    if (IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IClassFactory)) {
        AkLogInterface(IClassFactory, this);
        this->AddRef();
        *ppvObject = this;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IPersistPropertyBag)) {
        auto persistPropertyBag = new PersistPropertyBag(this->d->m_clsid);
        AkLogInterface(IPersistPropertyBag, persistPropertyBag);
        persistPropertyBag->AddRef();
        *ppvObject = persistPropertyBag;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IBaseFilter)) {
        auto baseFilter = BaseFilter::create(this->d->m_clsid);
        AkLogInterface(IBaseFilter, baseFilter);
        baseFilter->AddRef();
        *ppvObject = baseFilter;

        return S_OK;
    }

    return CUnknown::QueryInterface(riid, ppvObject);
}

HRESULT AkVCam::ClassFactory::CreateInstance(IUnknown *pUnkOuter,
                                             const IID &riid,
                                             void **ppvObject)
{
    AkLogMethod();
    AkLoggerLog("Outer: ", ULONG_PTR(pUnkOuter));
    AkLoggerLog("IID: ", stringFromClsid(riid));

    if (!ppvObject)
        return E_INVALIDARG;

    *ppvObject = nullptr;

    if (pUnkOuter && !IsEqualIID(riid, IID_IUnknown))
        return E_NOINTERFACE;

    this->AddRef();
    *ppvObject = this;

    return S_OK;
}

HRESULT AkVCam::ClassFactory::LockServer(BOOL fLock)
{
    AkLogMethod();
    this->d->m_locked += fLock? 1: -1;

    return S_OK;
}
