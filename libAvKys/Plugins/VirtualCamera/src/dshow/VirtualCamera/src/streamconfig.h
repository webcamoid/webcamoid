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

#ifndef STREAMCONFIG_H
#define STREAMCONFIG_H

#include <strmif.h>

#include "cunknown.h"

namespace AkVCam
{
    class StreamConfigPrivate;
    class Pin;

    class StreamConfig:
            public IAMStreamConfig,
            public CUnknown
    {
        public:
            StreamConfig(Pin *pin=nullptr);
            virtual ~StreamConfig();

            void setPin(Pin *pin);

            DECLARE_IUNKNOWN(IID_IAMStreamConfig)

            // IAMStreamConfig
            HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt);
            HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **pmt);
            HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount,
                                                              int *piSize);
            HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex,
                                                    AM_MEDIA_TYPE **pmt,
                                                    BYTE *pSCC);

        private:
            StreamConfigPrivate *d;
    };
}

#define DECLARE_IAMSTREAMCONFIG_NQ \
    DECLARE_IUNKNOWN_NQ \
    \
    HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt) \
    { \
        return StreamConfig::SetFormat(pmt); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **pmt) \
    { \
        return StreamConfig::GetFormat(pmt); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount, \
                                                      int *piSize) \
    { \
        return StreamConfig::GetNumberOfCapabilities(piCount, piSize); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, \
                                            AM_MEDIA_TYPE **pmt, \
                                            BYTE *pSCC) \
    { \
        return StreamConfig::GetStreamCaps(iIndex, pmt, pSCC); \
    }

#define DECLARE_IAMSTREAMCONFIG(interfaceIid) \
    DECLARE_IUNKNOWN_Q(interfaceIid) \
    DECLARE_IAMSTREAMCONFIG_NQ

#endif // STREAMCONFIG_H
