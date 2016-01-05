/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "framegrabber.h"

FrameGrabber::FrameGrabber(): QObject()
{

}

FrameGrabber::~FrameGrabber()
{
}

ULONG FrameGrabber::AddRef()
{
    return 1;
}

ULONG FrameGrabber::Release()
{
    return 2;
}

HRESULT FrameGrabber::QueryInterface(const IID &riid, void **ppvObject)
{
    if (NULL == ppvObject)
        return E_POINTER;

    if (riid == __uuidof(IUnknown)) {
        *ppvObject = static_cast<IUnknown *>(this);

        return S_OK;
    }

    if (riid == __uuidof(ISampleGrabberCB)) {
        *ppvObject = static_cast<ISampleGrabberCB *>(this);

        return S_OK;
    }

    return E_NOTIMPL;
}

HRESULT FrameGrabber::SampleCB(double time, IMediaSample *sample)
{
    BYTE *buffer = NULL;
    ulong bufferSize = sample->GetSize();

    HRESULT hr = sample->GetPointer(&buffer);

    if (FAILED(hr))
        return S_FALSE;

    QByteArray oBuffer((char *) buffer, bufferSize);

    emit this->frameReady(time, oBuffer);

    return S_OK;
}

HRESULT FrameGrabber::BufferCB(double time, BYTE *buffer, long bufferSize)
{
    QByteArray oBuffer((char *) buffer, bufferSize);

    emit this->frameReady(time, oBuffer);

    return S_OK;
}
