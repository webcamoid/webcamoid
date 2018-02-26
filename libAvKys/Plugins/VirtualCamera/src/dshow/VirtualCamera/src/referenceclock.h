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

#ifndef REFERENCECLOCK_H
#define REFERENCECLOCK_H

#include <strmif.h>

#include "cunknown.h"

#define TIME_BASE 1.0e7

namespace AkVCam
{
    class ReferenceClockPrivate;

    class ReferenceClock:
            public IReferenceClock,
            public CUnknown
    {
        public:
            ReferenceClock();
            virtual ~ReferenceClock();

            DECLARE_IUNKNOWN(IID_IReferenceClock)

            HRESULT STDMETHODCALLTYPE GetTime(REFERENCE_TIME *pTime);
            HRESULT STDMETHODCALLTYPE AdviseTime(REFERENCE_TIME baseTime,
                                                 REFERENCE_TIME streamTime,
                                                 HEVENT hEvent,
                                                 DWORD_PTR *pdwAdviseCookie);
            HRESULT STDMETHODCALLTYPE AdvisePeriodic(REFERENCE_TIME startTime,
                                                     REFERENCE_TIME periodTime,
                                                     HSEMAPHORE hSemaphore,
                                                     DWORD_PTR *pdwAdviseCookie);
            HRESULT STDMETHODCALLTYPE Unadvise(DWORD_PTR dwAdviseCookie);

        private:
            ReferenceClockPrivate *d;
    };
}

#endif // REFERENCECLOCK_H
