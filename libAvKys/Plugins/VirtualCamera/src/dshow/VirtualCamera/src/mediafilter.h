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

#ifndef MEDIAFILTER_H
#define MEDIAFILTER_H

#include <strmif.h>

#include "persistpropertybag.h"

namespace AkVCam
{
    class MediaFilterPrivate;
    typedef HRESULT (* StateChangedCallback)(void *userData,
                                             FILTER_STATE state);

    class MediaFilter:
            public IMediaFilter,
            public PersistPropertyBag
    {
        public:
            MediaFilter(REFIID classCLSID, IBaseFilter *baseFilter);
            virtual ~MediaFilter();

            void subscribeStateChanged(void *userData,
                                       StateChangedCallback callback);
            void unsubscribeStateChanged(void *userData,
                                         StateChangedCallback callback);

            DECLARE_IPERSISTPROPERTYBAG(IID_IMediaFilter)

            // IMediaFilter
            HRESULT STDMETHODCALLTYPE Stop();
            HRESULT STDMETHODCALLTYPE Pause();
            HRESULT STDMETHODCALLTYPE Run(REFERENCE_TIME tStart);
            HRESULT STDMETHODCALLTYPE GetState(DWORD dwMilliSecsTimeout,
                                               FILTER_STATE *State);
            HRESULT STDMETHODCALLTYPE SetSyncSource(IReferenceClock *pClock);
            HRESULT STDMETHODCALLTYPE GetSyncSource(IReferenceClock **pClock);

        private:
            MediaFilterPrivate *d;

        protected:
            virtual void stateChanged(FILTER_STATE state);
    };
}

#define DECLARE_IMEDIAFILTER_NQ \
    DECLARE_IPERSISTPROPERTYBAG_NQ \
    \
    void subscribeStateChanged(void *userData, \
                               StateChangedCallback callback) \
    { \
        MediaFilter::subscribeStateChanged(userData, callback); \
    } \
    \
    void unsubscribeStateChanged(void *userData, \
                                 StateChangedCallback callback) \
    { \
        MediaFilter::unsubscribeStateChanged(userData, callback); \
    } \
    \
    HRESULT STDMETHODCALLTYPE Stop() \
    { \
        return MediaFilter::Stop(); \
    } \
    \
    HRESULT STDMETHODCALLTYPE Pause() \
    { \
        return MediaFilter::Pause(); \
    } \
    \
    HRESULT STDMETHODCALLTYPE Run(REFERENCE_TIME tStart) \
    { \
        return MediaFilter::Run(tStart); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetState(DWORD dwMilliSecsTimeout, \
                                       FILTER_STATE *State) \
    { \
        return MediaFilter::GetState(dwMilliSecsTimeout, State); \
    } \
    \
    HRESULT STDMETHODCALLTYPE SetSyncSource(IReferenceClock *pClock) \
    { \
        return MediaFilter::SetSyncSource(pClock); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetSyncSource(IReferenceClock **pClock) \
    { \
        return MediaFilter::GetSyncSource(pClock); \
    }

#define DECLARE_IMEDIAFILTER(interfaceIid) \
    DECLARE_IPERSISTPROPERTYBAG_Q(interfaceIid) \
    DECLARE_IMEDIAFILTER_NQ

#endif // MEDIAFILTER_H
