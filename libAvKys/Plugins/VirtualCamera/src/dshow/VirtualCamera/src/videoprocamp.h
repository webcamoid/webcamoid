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

#ifndef VIDEOPROCAMP_H
#define VIDEOPROCAMP_H

#include <strmif.h>

#include "cunknown.h"

namespace AkVCam
{
    class VideoProcAmpPrivate;

    class VideoProcAmp:
            public IAMVideoProcAmp,
            public CUnknown
    {
        public:
            VideoProcAmp();
            virtual ~VideoProcAmp();

            DECLARE_IUNKNOWN(IID_IAMVideoProcAmp)

            // IAMVideoProcAmp
            HRESULT STDMETHODCALLTYPE GetRange(LONG Property,
                                               LONG *pMin,
                                               LONG *pMax,
                                               LONG *pSteppingDelta,
                                               LONG *pDefault,
                                               LONG *pCapsFlags);
            HRESULT STDMETHODCALLTYPE Set(LONG Property,
                                          LONG lValue,
                                          LONG Flags);
            HRESULT STDMETHODCALLTYPE Get(LONG Property,
                                          LONG *lValue,
                                          LONG *Flags);

        private:
            VideoProcAmpPrivate *d;
    };
}

#endif // VIDEOPROCAMP_H
