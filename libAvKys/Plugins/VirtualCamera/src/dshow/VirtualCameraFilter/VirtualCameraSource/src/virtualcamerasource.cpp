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

#include "virtualcamerasource.h"
#include "virtualcamerasourcestream.h"

VirtualCameraSource::VirtualCameraSource(LPUNKNOWN lpunk, HRESULT *phr):
    CSource(FILTER_NAME, lpunk, CLSID_VirtualCameraSource)
{
    ASSERT(phr);
    CAutoLock cAutoLock(&m_cStateLock);

    VirtualCameraSourceStream *pin = new VirtualCameraSourceStream(phr, this, OUTPUT_PIN_NAME);

    if (!pin)
        *phr = E_OUTOFMEMORY;
}

CUnknown *WINAPI VirtualCameraSource::CreateInstance(LPUNKNOWN lpunk,
                                                      HRESULT *phr)
{
    ASSERT(phr);

    // Create an instance of it self.
    CUnknown *punk = new VirtualCameraSource(lpunk, phr);

    if (!punk && phr)
        *phr = E_OUTOFMEMORY;

    return punk;
}

STDMETHODIMP VirtualCameraSource::QueryInterface(const IID &riid, void **ppv)
{
    if (riid == __uuidof(IAMStreamConfig)
        || riid == __uuidof(IKsPropertySet)
        || riid == __uuidof(IAMVideoProcAmp)
        || riid == __uuidof(IAMFilterMiscFlags))
        return this->m_paStreams[0]->QueryInterface(riid, ppv);

    return CSource::QueryInterface(riid, ppv);
}

STDMETHODIMP VirtualCameraSource::NonDelegatingQueryInterface(const IID &riid, void **ppv)
{
    if (riid == __uuidof(IAMStreamConfig)
        || riid == __uuidof(IKsPropertySet)
        || riid == __uuidof(IAMVideoProcAmp)
        || riid == __uuidof(IAMFilterMiscFlags))
        return this->m_paStreams[0]->NonDelegatingQueryInterface(riid, ppv);

    return CSource::NonDelegatingQueryInterface(riid, ppv);
}
