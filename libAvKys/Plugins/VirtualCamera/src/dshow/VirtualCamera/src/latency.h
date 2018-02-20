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

#ifndef LATENCY_H
#define LATENCY_H

#include <strmif.h>

#include "cunknown.h"

namespace AkVCam
{
    class LatencyPrivate;

    class Latency:
            public IAMLatency,
            public CUnknown
    {
        public:
            Latency(IAMStreamConfig *streamConfig=nullptr);
            virtual ~Latency();

            DECLARE_IUNKNOWN(IID_IAMLatency)

            // IAMLatency
            HRESULT WINAPI GetLatency(REFERENCE_TIME *prtLatency);

        private:
            LatencyPrivate *d;
    };
}

#define DECLARE_IAMLATENCY_NQ \
    DECLARE_IUNKNOWN_NQ \
    \
    HRESULT WINAPI GetLatency(REFERENCE_TIME *prtLatency) \
    { \
        return Latency::GetLatency(prtLatency); \
    }

#define DECLARE_IAMLATENCY(interfaceIid) \
    DECLARE_IUNKNOWN_Q(interfaceIid) \
    DECLARE_IAMLATENCY_NQ

#endif // LATENCY_H
