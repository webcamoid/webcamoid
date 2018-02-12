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

#ifndef CLASSFACTORY_H
#define CLASSFACTORY_H

#include "cunknown.h"

namespace AkVCam
{
    class ClassFactoryPrivate;

    class ClassFactory:
            public IClassFactory,
            public CUnknown
    {
        public:
            ClassFactory(const CLSID &clsid);
            virtual ~ClassFactory();

            static bool locked();

            DECLARE_IUNKNOWN_NQ

            // IUnknown
            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                                     void **ppvObject);

            // IClassFactory
            HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown *pUnkOuter,
                                                     REFIID riid,
                                                     void **ppvObject);
            HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock);

        private:
            ClassFactoryPrivate *d;
    };
}

#endif // CLASSFACTORY_H
