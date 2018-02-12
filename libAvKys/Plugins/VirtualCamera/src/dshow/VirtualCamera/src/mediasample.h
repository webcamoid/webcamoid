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

#ifndef MEDIASAMPLE_H
#define MEDIASAMPLE_H

#include <strmif.h>

#include "cunknown.h"

namespace AkVCam
{
    class MediaSamplePrivate;

    class MediaSample:
            public IMediaSample,
            public CUnknown
    {
        public:
            MediaSample(IMemAllocator *memAllocator,
                        LONG bufferSize, LONG align, LONG prefix);
            virtual ~MediaSample();

            DECLARE_IUNKNOWN_NR

            // IUnknown
            ULONG STDMETHODCALLTYPE Release();

            // IMediaSample
            HRESULT STDMETHODCALLTYPE GetPointer(BYTE **ppBuffer);
            LONG STDMETHODCALLTYPE GetSize();
            HRESULT STDMETHODCALLTYPE GetTime(REFERENCE_TIME *pTimeStart,
                                              REFERENCE_TIME *pTimeEnd);
            HRESULT STDMETHODCALLTYPE SetTime(REFERENCE_TIME *pTimeStart,
                                              REFERENCE_TIME *pTimeEnd);
            HRESULT STDMETHODCALLTYPE IsSyncPoint();
            HRESULT STDMETHODCALLTYPE SetSyncPoint(BOOL bIsSyncPoint);
            HRESULT STDMETHODCALLTYPE IsPreroll();
            HRESULT STDMETHODCALLTYPE SetPreroll(BOOL bIsPreroll);
            LONG STDMETHODCALLTYPE GetActualDataLength();
            HRESULT STDMETHODCALLTYPE SetActualDataLength(LONG lLen);
            HRESULT STDMETHODCALLTYPE GetMediaType(AM_MEDIA_TYPE **ppMediaType);
            HRESULT STDMETHODCALLTYPE SetMediaType(AM_MEDIA_TYPE *pMediaType);
            HRESULT STDMETHODCALLTYPE IsDiscontinuity();
            HRESULT STDMETHODCALLTYPE SetDiscontinuity(BOOL bDiscontinuity);
            HRESULT STDMETHODCALLTYPE GetMediaTime(LONGLONG *pTimeStart,
                                                   LONGLONG *pTimeEnd);
            HRESULT STDMETHODCALLTYPE SetMediaTime(LONGLONG *pTimeStart,
                                                   LONGLONG *pTimeEnd);

        private:
            MediaSamplePrivate *d;
    };
}

#define DECLARE_IMEDIASAMPLE \
    DECLARE_IUNKNOWN_NR \
    \
    ULONG STDMETHODCALLTYPE Release() \
    { \
        return MediaSample::Release(); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetPointer(BYTE **ppBuffer) \
    { \
        return MediaSample::GetPointer(ppBuffer); \
    } \
    \
    LONG STDMETHODCALLTYPE GetSize() \
    { \
        return MediaSample::GetSize(); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetTime(REFERENCE_TIME *pTimeStart, \
                                      REFERENCE_TIME *pTimeEnd) \
    { \
        return MediaSample::GetTime(pTimeStart, pTimeEnd); \
    } \
    \
    HRESULT STDMETHODCALLTYPE SetTime(REFERENCE_TIME *pTimeStart, \
                                      REFERENCE_TIME *pTimeEnd) \
    { \
        return MediaSample::SetTime(pTimeStart, pTimeEnd); \
    } \
    \
    HRESULT STDMETHODCALLTYPE IsSyncPoint() \
    { \
        return MediaSample::IsSyncPoint(); \
    } \
    \
    HRESULT STDMETHODCALLTYPE SetSyncPoint(BOOL bIsSyncPoint) \
    { \
        return MediaSample::SetSyncPoint(bIsSyncPoint); \
    } \
    \
    HRESULT STDMETHODCALLTYPE IsPreroll() \
    { \
        return MediaSample::IsPreroll(); \
    } \
    \
    HRESULT STDMETHODCALLTYPE SetPreroll(BOOL bIsPreroll) \
    { \
        return MediaSample::SetPreroll(bIsPreroll); \
    } \
    \
    LONG STDMETHODCALLTYPE GetActualDataLength() \
    { \
        return MediaSample::GetActualDataLength(); \
    } \
    \
    HRESULT STDMETHODCALLTYPE SetActualDataLength(LONG lLen) \
    { \
        return MediaSample::SetActualDataLength(lLen); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetMediaType(AM_MEDIA_TYPE **ppMediaType) \
    { \
        return MediaSample::GetMediaType(ppMediaType); \
    } \
    \
    HRESULT STDMETHODCALLTYPE SetMediaType(AM_MEDIA_TYPE *pMediaType) \
    { \
        return MediaSample::SetMediaType(pMediaType); \
    } \
    \
    HRESULT STDMETHODCALLTYPE IsDiscontinuity() \
    { \
        return MediaSample::IsDiscontinuity(); \
    } \
    \
    HRESULT STDMETHODCALLTYPE SetDiscontinuity(BOOL bDiscontinuity) \
    { \
        return MediaSample::SetDiscontinuity(bDiscontinuity); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetMediaTime(LONGLONG *pTimeStart, \
                                           LONGLONG *pTimeEnd) \
    { \
        return MediaSample::GetMediaTime(pTimeStart, pTimeEnd); \
    } \
    \
    HRESULT STDMETHODCALLTYPE SetMediaTime(LONGLONG *pTimeStart, \
                                           LONGLONG *pTimeEnd) \
    { \
        return MediaSample::SetMediaTime(pTimeStart, pTimeEnd); \
    }

#endif // MEDIASAMPLE_H
