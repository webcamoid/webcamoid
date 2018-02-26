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

#ifndef PIN_H
#define PIN_H

#include <string>
#include <vector>

#include "streamconfig.h"

namespace AkVCam
{
    class PinPrivate;
    class BaseFilter;
    class VideoFormat;

    class Pin:
            public IPin,
            public StreamConfig
    {
        public:
            Pin(BaseFilter *baseFilter=nullptr,
                const std::vector<AkVCam::VideoFormat> &formats={},
                const std::wstring &pinName={});
            virtual ~Pin();

            BaseFilter *baseFilter() const;
            void setBaseFilter(BaseFilter *baseFilter);
            static HRESULT stateChanged(void *userData, FILTER_STATE state);

            DECLARE_IAMSTREAMCONFIG_NQ

            // IUNknown
            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                                     void **ppvObject);

            // IPin
            HRESULT STDMETHODCALLTYPE Connect(IPin *pReceivePin,
                                              const AM_MEDIA_TYPE *pmt);
            HRESULT STDMETHODCALLTYPE ReceiveConnection(IPin *pConnector,
                                                        const AM_MEDIA_TYPE *pmt);
            HRESULT STDMETHODCALLTYPE Disconnect();
            HRESULT STDMETHODCALLTYPE ConnectedTo(IPin **pPin);
            HRESULT STDMETHODCALLTYPE ConnectionMediaType(AM_MEDIA_TYPE *pmt);
            HRESULT STDMETHODCALLTYPE QueryPinInfo(PIN_INFO *pInfo);
            HRESULT STDMETHODCALLTYPE QueryDirection(PIN_DIRECTION *pPinDir);
            HRESULT STDMETHODCALLTYPE QueryId(LPWSTR *Id);
            HRESULT STDMETHODCALLTYPE QueryAccept(const AM_MEDIA_TYPE *pmt);
            HRESULT STDMETHODCALLTYPE EnumMediaTypes(IEnumMediaTypes **ppEnum);
            HRESULT STDMETHODCALLTYPE QueryInternalConnections(IPin **apPin,
                                                               ULONG *nPin);
            HRESULT STDMETHODCALLTYPE EndOfStream();
            HRESULT STDMETHODCALLTYPE BeginFlush();
            HRESULT STDMETHODCALLTYPE EndFlush();
            HRESULT STDMETHODCALLTYPE NewSegment(REFERENCE_TIME tStart,
                                                 REFERENCE_TIME tStop,
                                                 double dRate);

        private:
            PinPrivate *d;
    };
}

#endif // PIN_H
