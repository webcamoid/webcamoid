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

#ifndef PERSISTPROPERTYBAG_H
#define PERSISTPROPERTYBAG_H

#include <map>
#include <ocidl.h>

#include "persist.h"

namespace AkVCam
{
    class PersistPropertyBagPrivate;
    typedef std::map<std::wstring, VARIANT> ComVariantMap;

    class PersistPropertyBag:
            public IPersistPropertyBag,
            public Persist
    {
        public:
            PersistPropertyBag(const GUID &clsid,
                               const ComVariantMap &properties={});
            virtual ~PersistPropertyBag();

            DECLARE_IPERSIST_NQ

            // IUnknown
            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                                     void **ppvObject);

            // IPersistPropertyBag
            HRESULT STDMETHODCALLTYPE InitNew();
            HRESULT STDMETHODCALLTYPE Load(IPropertyBag *pPropBag,
                                           IErrorLog *pErrorLog);
            HRESULT STDMETHODCALLTYPE Save(IPropertyBag *pPropBag,
                                           BOOL fClearDirty,
                                           BOOL fSaveAllProperties);

        private:
            PersistPropertyBagPrivate *d;
    };
}

#endif // PERSISTPROPERTYBAG_H
