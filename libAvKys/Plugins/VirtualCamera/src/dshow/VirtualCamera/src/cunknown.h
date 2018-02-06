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

#ifndef CUNKNOWN_H
#define CUNKNOWN_H

#include <unknwnbase.h>

namespace AkVCam
{
    class CUnknownPrivate;

    class CUnknown: public IUnknown
    {
        public:
            CUnknown(CUnknown *parent, REFIID parentCLSID);
            virtual ~CUnknown();

            void setParent(CUnknown *parent, const IID *parentCLSID=nullptr);
            ULONG ref() const;

            // IUnknown
            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                                     void **ppvObject);
            ULONG STDMETHODCALLTYPE AddRef();
            ULONG STDMETHODCALLTYPE Release();

        private:
            CUnknownPrivate *d;
    };
}

#define DECLARE_IUNKNOWN_Q \
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, \
                                             void **ppvObject) \
    { \
        return CUnknown::QueryInterface(riid, ppvObject); \
    }

#define DECLARE_IUNKNOWN_R \
    ULONG STDMETHODCALLTYPE Release() \
    { \
        auto result = CUnknown::Release(); \
        \
        if (!result) \
            delete this; \
        \
        return result; \
    }

#define DECLARE_IUNKNOWN_NQR \
    ULONG ref() const \
    { \
        return CUnknown::ref(); \
    } \
    \
    void setParent(CUnknown *parent, const IID *parentCLSID=nullptr) \
    { \
        return CUnknown::setParent(parent, parentCLSID); \
    } \
    \
    ULONG STDMETHODCALLTYPE AddRef() \
    { \
        return CUnknown::AddRef(); \
    }

#define DECLARE_IUNKNOWN_NQ \
    DECLARE_IUNKNOWN_NQR \
    DECLARE_IUNKNOWN_R

#define DECLARE_IUNKNOWN_NR \
    DECLARE_IUNKNOWN_NQR \
    DECLARE_IUNKNOWN_Q

#define DECLARE_IUNKNOWN \
    DECLARE_IUNKNOWN_NQR \
    DECLARE_IUNKNOWN_Q \
    DECLARE_IUNKNOWN_R

#endif // CUNKNOWN_H
