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

#include "persist.h"
#include "utils.h"
#include "VCamUtils/src/utils.h"

#define AK_CUR_INTERFACE "Persist"

namespace AkVCam
{
    class PersistPrivate
    {
        public:
            CLSID m_clsid;
    };
}

AkVCam::Persist::Persist(const IID &classCLSID):
    CUnknown(this, IID_IPersist)
{
    this->d = new PersistPrivate;
    this->d->m_clsid = classCLSID;
}

AkVCam::Persist::~Persist()
{
    delete this->d;
}

HRESULT AkVCam::Persist::GetClassID(CLSID *pClassID)
{
    AkLogMethod();

    if (!pClassID)
        return E_POINTER;

    *pClassID = this->d->m_clsid;

    return S_OK;
}
