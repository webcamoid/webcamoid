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

#include <algorithm>
#include <vector>
#include <dshow.h>

#include "mediafilter.h"
#include "utils.h"
#include "VCamUtils/src/utils.h"

#define AK_CUR_INTERFACE "MediaFilter"

namespace AkVCam
{
    class MediaFilterPrivate
    {
        public:
            IBaseFilter *m_baseFilter;
            IReferenceClock *m_clock;
            std::vector<StateChangedCallback> m_stateChanged;
            FILTER_STATE m_state;
            REFERENCE_TIME m_start;
    };
}

AkVCam::MediaFilter::MediaFilter(const IID &classCLSID,
                                 IBaseFilter *baseFilter):
    Persist(classCLSID)
{
    this->setParent(this, &IID_IMediaFilter);
    this->d = new MediaFilterPrivate;
    this->d->m_baseFilter = baseFilter;
    this->d->m_clock = nullptr;
    this->d->m_state = State_Stopped;
    this->d->m_start = 0;
}

AkVCam::MediaFilter::~MediaFilter()
{
    if (this->d->m_clock)
        this->d->m_clock->Release();

    delete this->d;
}

void AkVCam::MediaFilter::subscribeStateChanged(const StateChangedCallback &callback)
{
    AkLogMethod();
    this->d->m_stateChanged.push_back(callback);
}

void AkVCam::MediaFilter::unsubscribeStateChanged(const StateChangedCallback &callback)
{
    AkLogMethod();

    for (auto it = this->d->m_stateChanged.begin();
         it != this->d->m_stateChanged.end();
         it++) {
        auto a = it->target<void (*)(FILTER_STATE)>();
        auto b = callback.target<void (FILTER_STATE)>();

        if (*a == b) {
            this->d->m_stateChanged.erase(it);

            break;
        }
    }
}

HRESULT AkVCam::MediaFilter::Stop()
{
    AkLogMethod();
    this->d->m_state = State_Stopped;

    for (auto &callback: this->d->m_stateChanged)
        callback(this->d->m_state);

    return S_OK;
}

HRESULT AkVCam::MediaFilter::Pause()
{
    AkLogMethod();
    this->d->m_state = State_Paused;

    for (auto &callback: this->d->m_stateChanged)
        callback(this->d->m_state);

    return S_OK;
}

HRESULT AkVCam::MediaFilter::Run(REFERENCE_TIME tStart)
{
    AkLogMethod();
    this->d->m_start = tStart;
    this->d->m_state = State_Running;

    for (auto &callback: this->d->m_stateChanged)
        callback(this->d->m_state);

    return S_OK;
}

HRESULT AkVCam::MediaFilter::GetState(DWORD dwMilliSecsTimeout,
                                      FILTER_STATE *State)
{
    UNUSED(dwMilliSecsTimeout)
    AkLogMethod();

    if (!State)
        return E_POINTER;

    *State = this->d->m_state;
    AkLoggerLog("State: ", *State);

    return S_OK;
}

HRESULT AkVCam::MediaFilter::SetSyncSource(IReferenceClock *pClock)
{
    AkLogMethod();

    if (this->d->m_clock)
        this->d->m_clock->Release();

    this->d->m_clock = pClock;

    if (this->d->m_clock)
        this->d->m_clock->AddRef();

    return S_OK;
}

HRESULT AkVCam::MediaFilter::GetSyncSource(IReferenceClock **pClock)
{
    AkLogMethod();

    if (!pClock)
        return E_POINTER;

    *pClock = this->d->m_clock;

    if (*pClock)
        (*pClock)->AddRef();

    return S_OK;
}
