/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#ifndef SAMPLEGRABBER_H
#define SAMPLEGRABBER_H

#include <strmif.h>

DEFINE_GUID(IID_ISampleGrabberCB, 0x0579154a, 0x2b53, 0x4994, 0xb0,0xd0, 0xe7, 0x73, 0x14, 0x8e, 0xff, 0x85);

MIDL_INTERFACE("0579154a-2b53-4994-b0d0-e773148eff85")
ISampleGrabberCB: public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE SampleCB(double SampleTime,
                                               IMediaSample *pSample) = 0;
    virtual HRESULT STDMETHODCALLTYPE BufferCB(double SampleTime,
                                               BYTE *pBuffer,
                                               LONG BufferLen) = 0;
};

#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(ISampleGrabberCB, 0x0579154a, 0x2b53, 0x4994, 0xb0, 0xd0, 0xe7, 0x73, 0x14, 0x8e, 0xff, 0x85)
#endif

DEFINE_GUID(IID_ISampleGrabber, 0x6b652fff, 0x11fe, 0x4fce, 0x92,0xad, 0x02, 0x66, 0xb5, 0xd7, 0xc7, 0x8f);

MIDL_INTERFACE("6b652fff-11fe-4fce-92ad-0266b5d7c78f")
ISampleGrabber: public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE SetOneShot(BOOL OneShot) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMediaType(const AM_MEDIA_TYPE *pType) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType(AM_MEDIA_TYPE *pType) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetBufferSamples(BOOL BufferThem) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer(LONG *pBufferSize,
                                                       LONG *pBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentSample(IMediaSample **ppSample) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCallback(ISampleGrabberCB *pCallback,
                                                  LONG WhichMethodToCallback) = 0;
};

#endif // SAMPLEGRABBER_H
