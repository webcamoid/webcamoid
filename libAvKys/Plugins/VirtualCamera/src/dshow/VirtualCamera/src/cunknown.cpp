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

#include <atomic>

#include "cunknown.h"
#include "utils.h"
#include "VCamUtils/src/utils.h"

#define AkCUnknownLogMethod() \
    AkLoggerLog((this->d->m_parent? \
                    stringFromClsid(this->d->m_parentCLSID): \
                    std::string("CUnknown")) \
                << "(" << (this->d->m_parent? this->d->m_parent: this) \
                << ")::" << __FUNCTION__ << "()")

#define AkCUnknownLogThis() \
    AkLoggerLog("Returning " \
                << (this->d->m_parent? \
                        stringFromClsid(this->d->m_parentCLSID): \
                        std::string("CUnknown")) \
                << "(" << (this->d->m_parent? this->d->m_parent: this) << ")")

namespace AkVCam
{
    class CUnknownPrivate
    {
        public:
            std::atomic<ULONG> m_ref;
            CUnknown *m_parent;
            CLSID m_parentCLSID;
    };
}

AkVCam::CUnknown::CUnknown(CUnknown *parent, REFIID parentCLSID)
{
    this->d = new CUnknownPrivate;
    this->d->m_ref = 0;
    this->d->m_parent = parent;
    this->d->m_parentCLSID = parentCLSID;
}

AkVCam::CUnknown::~CUnknown()
{

}

void AkVCam::CUnknown::setParent(AkVCam::CUnknown *parent,
                                 const IID *parentCLSID)
{
    this->d->m_parent = parent;
    this->d->m_parentCLSID = parentCLSID? *parentCLSID: GUID_NULL;
}

ULONG AkVCam::CUnknown::ref() const
{
    return this->d->m_ref;
}

HRESULT AkVCam::CUnknown::QueryInterface(const IID &riid, void **ppvObject)
{
    AkCUnknownLogMethod();
    AkLoggerLog("IID: " + AkVCam::stringFromClsid(riid));

    if (!ppvObject)
        return E_POINTER;

    *ppvObject = nullptr;

    if (IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, this->d->m_parentCLSID)) {
        AkCUnknownLogThis();
        this->d->m_parent->AddRef();
        *ppvObject = this->d->m_parent;

        return S_OK;
    } else {
        AkLoggerLog("Unknown interface");
    }

    return E_NOINTERFACE;
}

ULONG AkVCam::CUnknown::AddRef()
{
    AkCUnknownLogMethod();
    this->d->m_ref++;
    AkLoggerLog("REF: " << this->d->m_ref);

    return this->d->m_ref;
}

ULONG AkVCam::CUnknown::Release()
{
    AkCUnknownLogMethod();
    AkLoggerLog("REF: " << this->d->m_ref);

    if (this->d->m_ref)
        this->d->m_ref--;

    return this->d->m_ref;
}
