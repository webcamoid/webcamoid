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

#ifndef VIRTUALCAMERASOURCESTREAM_H
#define VIRTUALCAMERASOURCESTREAM_H

#include <wtypes.h>
#include <gdiplus.h>
#include <streams.h>

//#define GLOGAL_CONTROLS

typedef size_t (*convert_func_t)(void *dst, const void *src, int width, int height);

class VirtualCameraSourceStream:
        public CSourceStream,
        public IAMStreamConfig,
        public IKsPropertySet,
        public IAMVideoProcAmp,
        public IAMFilterMiscFlags,
        public CCritSec
{
    public:
        VirtualCameraSourceStream(HRESULT *phr,
                                  CSource *pParent,
                                  LPCWSTR pPinName);
        ~VirtualCameraSourceStream();

        HRESULT FillBuffer(IMediaSample *pms);
        HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc,
                                 ALLOCATOR_PROPERTIES *pProperties);
        HRESULT SetMediaType(const CMediaType *pMediaType);
        HRESULT CheckMediaType(const CMediaType *pMediaType);
        HRESULT GetMediaType(int iPosition, CMediaType *pmt);
        HRESULT OnThreadCreate();

        STDMETHODIMP Notify(IBaseFilter *pSender, Quality quality);
        STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
        STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        // IAMStreamConfig
        HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt);
        HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **ppmt);
        HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount,
                                                          int *piSize);
        HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex,
                                                AM_MEDIA_TYPE **pmt,
                                                BYTE *pSCC);

        // IKsPropertySet
        HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet,
                                      DWORD dwID,
                                      void *pInstanceData,
                                      DWORD cbInstanceData,
                                      void *pPropData,
                                      DWORD cbPropData);
        HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet,
                                      DWORD dwPropID,
                                      void *pInstanceData,
                                      DWORD cbInstanceData,
                                      void *pPropData,
                                      DWORD cbPropData,
                                      DWORD *pcbReturned);
        HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet,
                                                 DWORD dwPropID,
                                                 DWORD *pTypeSupport);

        // IAMVideoProcAmp
        HRESULT WINAPI GetRange(LONG Property,
                                LONG *pMin,
                                LONG *pMax,
                                LONG *pSteppingDelta,
                                LONG *pDefault,
                                LONG *pCapsFlags);
        HRESULT WINAPI Set(LONG Property,LONG lValue,LONG Flags);
        HRESULT WINAPI Get(LONG Property,LONG *lValue,LONG *Flags);

        HRESULT CompleteConnect(IPin *pReceivePin);
        HRESULT BreakConnect();

        // IAMFilterMiscFlags
        ULONG STDMETHODCALLTYPE GetMiscFlags();

    private:
        // Frame parameters.
        CRefTime m_pts;
        DOUBLE m_fps;
        BOOL m_formatIsSet;
        Gdiplus::PixelFormat m_gdiFormat;
        convert_func_t m_convert;
        ULONG_PTR m_gdpToken;
        Gdiplus::GdiplusStartupInput m_gdpStartupInput;
        Gdiplus::Bitmap *m_bitmap;

#ifndef GLOGAL_CONTROLS
        // Picture controls
        int m_brightness;
        int m_contrast;
        int m_hue;
        int m_saturation;
        int m_gamma;
        bool m_colorEnable;
#endif

        HRESULT mediaType(int iPosition, CMediaType *pmt) const;
        HRESULT isSupported(const CMediaType *pCurMediaType, const CMediaType *pMediaType) const;
        Gdiplus::Bitmap *loadBitmapRC(LPCTSTR lpName);
        Gdiplus::Bitmap *loadBitmapIPC(LPCTSTR lpName);
        bool readBitmap(LPCTSTR lpName,
                        LPCTSTR lpAltName,
                        CMediaType *mediaType,
                        BYTE *data);
};

#endif // VIRTUALCAMERASOURCESTREAM_H
