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

#ifndef MEMALLOCATOR_H
#define MEMALLOCATOR_H

#include <strmif.h>

#include "cunknown.h"

namespace AkVCam
{
    class MemAllocatorPrivate;

    class MemAllocator:
            public IMemAllocator,
            public CUnknown
    {
        public:
            MemAllocator();
            virtual ~MemAllocator();

            DECLARE_IUNKNOWN(IID_IMemAllocator)

            // IMemAllocator
            HRESULT STDMETHODCALLTYPE SetProperties(ALLOCATOR_PROPERTIES *pRequest,
                                                    ALLOCATOR_PROPERTIES *pActual);
            HRESULT STDMETHODCALLTYPE GetProperties(ALLOCATOR_PROPERTIES *pProps);
            HRESULT STDMETHODCALLTYPE Commit();
            HRESULT STDMETHODCALLTYPE Decommit();
            HRESULT STDMETHODCALLTYPE GetBuffer(IMediaSample **ppBuffer,
                                                REFERENCE_TIME *pStartTime,
                                                REFERENCE_TIME *pEndTime,
                                                DWORD dwFlags);
            HRESULT STDMETHODCALLTYPE ReleaseBuffer(IMediaSample *pBuffer);

        private:
            MemAllocatorPrivate *d;
    };
}

#endif // MEMALLOCATOR_H
