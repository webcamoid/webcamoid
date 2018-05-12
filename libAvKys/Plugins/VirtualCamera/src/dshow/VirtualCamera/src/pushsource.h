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

#ifndef PUSHSOURCE_H
#define PUSHSOURCE_H

#include "latency.h"

namespace AkVCam
{
    class PushSource:
            public IAMPushSource,
            public Latency
    {
        public:
            PushSource(IAMStreamConfig *streamConfig);
            virtual ~PushSource();

            DECLARE_IAMLATENCY(IID_IAMPushSource)

            // IAMPushSource
            HRESULT WINAPI GetPushSourceFlags(ULONG *pFlags);
            HRESULT WINAPI SetPushSourceFlags(ULONG Flags);
            HRESULT WINAPI SetStreamOffset(REFERENCE_TIME rtOffset);
            HRESULT WINAPI GetStreamOffset(REFERENCE_TIME *prtOffset);
            HRESULT WINAPI GetMaxStreamOffset(REFERENCE_TIME *prtMaxOffset);
            HRESULT WINAPI SetMaxStreamOffset(REFERENCE_TIME rtMaxOffset);
    };
}

#endif // PUSHSOURCE_H
