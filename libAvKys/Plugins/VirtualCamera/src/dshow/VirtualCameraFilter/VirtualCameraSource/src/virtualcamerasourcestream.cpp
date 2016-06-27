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
 * Web-Site: http://webcamoid.github.io/
 */

#include "virtualcamerasourcestream.h"
#include "vcguidef.h"
#include "resources.h"
#include "colorconv.h"
#include "imgfilters.h"
#include "ipcbridge.h"

#define TIME_BASE 1.0e7

namespace VideoFormat
{
    class VideoFormat
    {
        public:
            DWORD compression;
            GUID guid;
            WORD bpp;
            const DWORD *masks;
            Gdiplus::PixelFormat gdiFormat;
            convert_func_t convert;
    };

    static VideoFormat videoFormats[] = {
        // RGB formats
        //{BI_RGB, MEDIASUBTYPE_RGB32, 32, NULL, PixelFormat32bppRGB, bgr3_to_bgr4},
        {BI_RGB, MEDIASUBTYPE_RGB24, 24, NULL, PixelFormat24bppRGB, NULL},
        {BI_BITFIELDS, MEDIASUBTYPE_RGB565, 16, bits565, PixelFormat16bppRGB565, rgb3_to_rgbp},
        {BI_BITFIELDS, MEDIASUBTYPE_RGB555, 16, bits555, PixelFormat16bppRGB555, rgb3_to_rgbo},

        // Luminance+Chrominance formats
        {MAKEFOURCC('U', 'Y', 'V', 'Y'), MEDIASUBTYPE_UYVY, 16, NULL, PixelFormatUndefined, bgr3_to_uyvy},
        {MAKEFOURCC('Y', 'U', 'Y', '2'), MEDIASUBTYPE_YUY2, 16, NULL, PixelFormatUndefined, bgr3_to_yuy2},
        //{MAKEFOURCC('Y', 'V', '1', '2'), MEDIASUBTYPE_YV12, 12, NULL, PixelFormatUndefined, bgr3_to_yv12},

        // two planes -- one Y, one Cr + Cb interleaved
        //{MAKEFOURCC('N', 'V', '1', '2'), MEDIASUBTYPE_NV12, 12, NULL, PixelFormatUndefined, rgb3_to_nv12},
        {0, GUID_NULL, 0, NULL, PixelFormatUndefined, NULL}
    };

    inline int count()
    {
        int n = 0;

        for (; videoFormats[n].compression
               || videoFormats[n].guid != GUID_NULL
               || videoFormats[n].bpp
               || videoFormats[n].masks
               || videoFormats[n].gdiFormat != PixelFormatUndefined
               || videoFormats[n].convert != NULL
             ; n++) {
        }

        return n;
    }

    static int length = count();

    inline const VideoFormat *byGuid(const GUID &guid)
    {
        for (int i = 0; i < length; i++)
            if (videoFormats[i].guid == guid)
                return &videoFormats[i];

        return NULL;
    }
}

namespace FrameResolution
{
    class FrameResolution
    {
        public:
            LONG width;
            LONG height;
            const char *name;
    };

    // This supported resolutions list is based on:
    //
    // https://en.wikipedia.org/wiki/Graphics_display_resolution
    //
    // I just enabled the most common resolutions because enabling all at once will
    // slowdown the capture program.
    static FrameResolution frameResolutions[] = {
        {640, 480, "VGA"},       // Default resolution go first
        {160, 120, "QQVGA"},
    //    {240, 160, "HQVGA"},
        {320, 240, "QVGA"},
    //    {360, 240, "WQVGA"},
    //    {384, 240, "WQVGA"},
    //    {400, 240, "WQVGA"},
    //    {480, 320, "HVGA"},
        {640, 360, "nHD"},
    //    {720, 480, "WVGA"},
    //    {768, 480, "WVGA"},
    //    {800, 480, "WVGA"},
        {800, 600, "SVGA"},
    //    {854, 480, "FWVGA"},
    //    {960, 540, "qHD"},
    //    {960, 640, "DVGA"},
        {1024, 576, "WSVGA"},
        {1024, 600, "WSVGA"},
        {1024, 768, "XGA"},
    //    {1152, 768, "WXGA"},
    //    {1152, 864, "XGA+"},
        {1280, 720, "HD"},
    //    {1280, 768, "WXGA"},
    //    {1280, 800, "WXGA"},
    //    {1280, 1024, "SXGA"},
    //    {1360, 768, "WXGA"},
        {1366, 768, "FWXGA"},
    //    {1400, 1050, "SXGA+"},
    //    {1440, 900, "WXGA+"},
    //    {1440, 960, "WSXGA"},
    //    {1600, 900, "HD+"},
        {1600, 1200, "UXGA"},
    //    {1680, 1050, "WSXGA+"},
        {1920, 1080, "FHD"},
    //    {1920, 1200, "WUXGA"},
    //    {2048, 1152, "QWXGA"},
    //    {2048, 1536, "QXGA"},
    //    {2560, 1440, "(W)QHD"},
    //    {2560, 1600, "WQXGA"},
    //    {2560, 2048, "QSXGA"},
    //    {3200, 1800, "QHD+"},
    //    {3200, 2048, "WQSXGA"},
    //    {3200, 2400, "QUXGA"},
    //    {3840, 2160, "4K UHD"},
    //    {3840, 2400, "WQUXGA"},
    //    {4096, 3072, "HXGA"},
    //    {5120, 2880, "5K UHD+"},
    //    {5120, 3200, "WHXGA"},
    //    {5120, 4096, "HSXGA"},
    //    {6400, 4096, "WHSXGA"},
    //    {6400, 4800, "HUXGA"},
    //    {7680, 4320, "8K UHD"},
    //    {7680, 4800, "WHUXGA"},
        {0, 0, NULL}
    };

    inline int count()
    {
        int n = 0;

        for (; frameResolutions[n].width
               || frameResolutions[n].height
               || frameResolutions[n].name
             ; n++) {
        }

        return n;
    }

    static int length = count();
}

// Picture controls
static int imgprop_brightness = 0;
static int imgprop_contrast = 0;
static int imgprop_hue = 0;
static int imgprop_saturation = 0;
static int imgprop_gamma = 0;
static bool imgprop_colorEnable = true;

VirtualCameraSourceStream::VirtualCameraSourceStream(HRESULT *phr,
                                                     CSource *pParent,
                                                     LPCWSTR pPinName):
    CSourceStream(FILTER_NAME, phr, pParent, pPinName)
{
    ASSERT(phr);
    CAutoLock cAutolock(this->m_pFilter->pStateLock());

    this->m_fps = 30;
    this->m_formatIsSet = FALSE;
    this->m_gdiFormat = PixelFormatUndefined;
    this->m_convert = NULL;
    this->m_bitmap = NULL;

    // Somewhere where it will run once before you need to use GDI:
    Gdiplus::GdiplusStartup(&this->m_gdpToken, &this->m_gdpStartupInput, NULL);
}

VirtualCameraSourceStream::~VirtualCameraSourceStream()
{
    //CAutoLock cAutoLock(this);
    CAutoLock cAutolock(this->m_pFilter->pStateLock());

    if (this->m_bitmap)
        delete this->m_bitmap;

    Gdiplus::GdiplusShutdown(this->m_gdpToken);
}

HRESULT VirtualCameraSourceStream::FillBuffer(IMediaSample *mediaSample)
{
    CheckPointer(mediaSample, E_POINTER);
    size_t lDataLen = size_t(mediaSample->GetSize());

    if (!lDataLen)
        return S_FALSE;

    BYTE *pData = NULL;

    if (FAILED(mediaSample->GetPointer(&pData)) || !pData)
        return S_FALSE;

    ZeroMemory(pData, lDataLen);

    {
        CAutoLock cAutoLock(this);

        this->readBitmap(IPC_FILE_NAME,
                         MAKEINTRESOURCE(IDB_PATTERN1),
                         &this->m_mt,
                         pData);

        // The current time is the sample's start
        CRefTime rtStart = this->m_pts;

        // Increment to find the finish time
        this->m_pts += REFERENCE_TIME(TIME_BASE / this->m_fps);

        mediaSample->SetTime(reinterpret_cast<REFERENCE_TIME *>(&rtStart),
                     reinterpret_cast<REFERENCE_TIME *>(&this->m_pts));
        mediaSample->SetMediaTime(reinterpret_cast<REFERENCE_TIME *>(&rtStart),
                          reinterpret_cast<REFERENCE_TIME *>(&this->m_pts));
    }

    mediaSample->SetSyncPoint(TRUE);

    return NOERROR;
}

HRESULT VirtualCameraSourceStream::DecideBufferSize(IMemAllocator *pIMemAlloc,
                                                    ALLOCATOR_PROPERTIES *pProperties)
{
    CheckPointer(pIMemAlloc, E_POINTER);
    CheckPointer(pProperties, E_POINTER);

    CAutoLock cAutolock(this->m_pFilter->pStateLock());
    CAutoLock cAutoLock(this);

    pProperties->cBuffers = 1;
    pProperties->cbBuffer = LONG(this->m_mt.lSampleSize);

    ASSERT(pProperties->cbBuffer);

    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pIMemAlloc->SetProperties(pProperties, &Actual);

    if (FAILED(hr))
        return hr;

    if (Actual.cbBuffer < pProperties->cbBuffer)
        return E_FAIL;

    ASSERT(Actual.cBuffers == 1);

    return NOERROR;
}

HRESULT VirtualCameraSourceStream::SetMediaType(const CMediaType *pMediaType)
{
    CAutoLock cAutolock(this->m_pFilter->pStateLock());

    HRESULT hr = CSourceStream::SetMediaType(pMediaType);
    GUID format = *pMediaType->Subtype();

    const VideoFormat::VideoFormat *vf = VideoFormat::byGuid(format);

    if (!vf)
        return VFW_E_INVALIDMEDIATYPE;

    this->m_gdiFormat = vf->gdiFormat;
    this->m_convert = vf->convert;

    return hr;
}

HRESULT VirtualCameraSourceStream::CheckMediaType(const CMediaType *pMediaType)
{
    CheckPointer(pMediaType, E_POINTER);

    CAutoLock cAutoLock(this);
    CAutoLock cAutolock(this->m_pFilter->pStateLock());

    VIDEOINFO *pvi = reinterpret_cast<VIDEOINFO *>(pMediaType->Format());

    if (!pvi)
        return E_INVALIDARG;

    return this->m_formatIsSet?
                this->isSupported(&this->m_mt, pMediaType):
                this->isSupported(NULL, pMediaType);
}

HRESULT VirtualCameraSourceStream::GetMediaType(int iPosition, CMediaType *pmt)
{
    CheckPointer(pmt, E_POINTER);
    CAutoLock cAutoLock(this);

    if (this->m_formatIsSet) {
        if (iPosition == 0) {
            *pmt = this->m_mt;

            return S_OK;
        } else
            return VFW_E_INVALIDMEDIATYPE;
    }

    return this->mediaType(iPosition, pmt);
}

HRESULT VirtualCameraSourceStream::OnThreadCreate()
{
    CAutoLock cAutoLock(this);
    this->m_pts = 0;

    return NOERROR;
}

STDMETHODIMP VirtualCameraSourceStream::Notify(IBaseFilter *pSender, Quality quality)
{
    UNUSED(pSender)
    UNUSED(quality)

    return E_NOTIMPL;
}

STDMETHODIMP VirtualCameraSourceStream::QueryInterface(const IID &riid, void **ppv)
{
    // Standard OLE stuff
    if (riid == __uuidof(IAMStreamConfig))
        *ppv = static_cast<IAMStreamConfig *>(this);
    else if (riid == __uuidof(IKsPropertySet))
        *ppv = static_cast<IKsPropertySet *>(this);
    else if (riid == __uuidof(IAMVideoProcAmp))
        *ppv = static_cast<IAMVideoProcAmp *>(this);
    else if (riid == __uuidof(IAMFilterMiscFlags))
        *ppv = static_cast<IAMFilterMiscFlags *>(this);
    else
        return CSourceStream::QueryInterface(riid, ppv);

    this->AddRef();

    return S_OK;
}

STDMETHODIMP VirtualCameraSourceStream::NonDelegatingQueryInterface(const IID &riid, void **ppv)
{
    // Standard OLE stuff
    if (riid == __uuidof(IAMStreamConfig))
        *ppv = static_cast<IAMStreamConfig *>(this);
    else if (riid == __uuidof(IKsPropertySet))
        *ppv = static_cast<IKsPropertySet *>(this);
    else if (riid == __uuidof(IAMVideoProcAmp))
        *ppv = static_cast<IAMVideoProcAmp *>(this);
    else if (riid == __uuidof(IAMFilterMiscFlags))
        *ppv = static_cast<IAMFilterMiscFlags *>(this);
    else
        return CSourceStream::NonDelegatingQueryInterface(riid, ppv);

    this->AddRef();

    return S_OK;
}

ULONG VirtualCameraSourceStream::AddRef()
{
    return this->GetOwner()->AddRef();
}

ULONG VirtualCameraSourceStream::Release()
{
    return this->GetOwner()->Release();
}

HRESULT VirtualCameraSourceStream::SetFormat(AM_MEDIA_TYPE *pmt)
{
    CheckPointer(pmt, E_POINTER)

    CAutoLock cAutolock(this->m_pFilter->pStateLock());
    CAutoLock cAutoLock(this);

    // Cannot set the format unless the filter is stopped.
    FILTER_STATE curState = State_Stopped;
    this->m_pFilter->GetState(0, &curState);

    if (curState != State_Stopped)
        return VFW_E_NOT_STOPPED;

    HRESULT hr = this->isSupported(NULL, reinterpret_cast<CMediaType *>(pmt));

    if (hr != S_OK)
        return hr;

    this->m_mt = *pmt;
    this->m_formatIsSet = TRUE;
    GUID format = *this->m_mt.Subtype();

    const VideoFormat::VideoFormat *vf = VideoFormat::byGuid(format);

    if (!vf)
        return VFW_E_INVALIDMEDIATYPE;

    this->m_gdiFormat = vf->gdiFormat;
    this->m_convert = vf->convert;
    IPin* pin = NULL;
    this->ConnectedTo(&pin);

    if (pin) {
        if (SUCCEEDED(pin->QueryAccept(pmt))) {
            hr = this->m_pFilter->GetFilterGraph()->Reconnect(this);

            if (FAILED(hr)) {
                this->m_formatIsSet = FALSE;

                return hr;
            }
        }
    }

    return S_OK;
}

HRESULT VirtualCameraSourceStream::GetFormat(AM_MEDIA_TYPE **ppmt)
{
    *ppmt = CreateMediaType(&this->m_mt);

    return S_OK;
}

HRESULT VirtualCameraSourceStream::GetNumberOfCapabilities(int *piCount,
                                                           int *piSize)
{
    CheckPointer(piCount, E_POINTER);
    CheckPointer(piSize, E_POINTER);

    *piCount = VideoFormat::length * FrameResolution::length;
    *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);

    return S_OK;
}

HRESULT VirtualCameraSourceStream::GetStreamCaps(int iIndex,
                                                 AM_MEDIA_TYPE **pmt,
                                                 BYTE *pSCC)
{
    CheckPointer(pSCC, E_POINTER);

    CMediaType mediaType;
    HRESULT hr = this->mediaType(iIndex, &mediaType);

    if (hr != S_OK || hr == VFW_S_NO_MORE_ITEMS)
        return hr;

    *pmt = CreateMediaType(&mediaType);

    VIDEO_STREAM_CONFIG_CAPS *pvscc = reinterpret_cast<VIDEO_STREAM_CONFIG_CAPS *>(pSCC);
    VIDEOINFO *pvi = reinterpret_cast<VIDEOINFO *>((*pmt)->pbFormat);

    pvscc->guid = FORMAT_VideoInfo;
    pvscc->VideoStandard = AnalogVideo_None;
    pvscc->InputSize.cx = pvi->bmiHeader.biWidth;
    pvscc->InputSize.cy = pvi->bmiHeader.biHeight;
    pvscc->MinCroppingSize.cx = FrameResolution::frameResolutions[0].width;
    pvscc->MinCroppingSize.cy = FrameResolution::frameResolutions[0].height;
    pvscc->MaxCroppingSize.cx = FrameResolution::frameResolutions[FrameResolution::length - 1].width;
    pvscc->MaxCroppingSize.cy = FrameResolution::frameResolutions[FrameResolution::length - 1].height;
    pvscc->CropGranularityX = 1;
    pvscc->CropGranularityY = 1;
    pvscc->CropAlignX = 0;
    pvscc->CropAlignY = 0;

    pvscc->MinOutputSize.cx = FrameResolution::frameResolutions[0].width;
    pvscc->MinOutputSize.cy = FrameResolution::frameResolutions[0].height;
    pvscc->MaxOutputSize.cx = FrameResolution::frameResolutions[FrameResolution::length - 1].width;
    pvscc->MaxOutputSize.cy = FrameResolution::frameResolutions[FrameResolution::length - 1].height;
    pvscc->OutputGranularityX = 1;
    pvscc->OutputGranularityY = 1;
    pvscc->StretchTapsX = 1;
    pvscc->StretchTapsY = 1;
    pvscc->ShrinkTapsX = 1;
    pvscc->ShrinkTapsY = 1;

    DOUBLE minFps = 0.02;

    pvscc->MinFrameInterval = REFERENCE_TIME(TIME_BASE / this->m_fps);
    pvscc->MaxFrameInterval = REFERENCE_TIME(TIME_BASE / minFps);
    pvscc->MinBitsPerSecond = LONG(8 * (*pmt)->lSampleSize * minFps);
    pvscc->MaxBitsPerSecond = LONG(8 * (*pmt)->lSampleSize * this->m_fps);

    return S_OK;
}

HRESULT VirtualCameraSourceStream::Set(const GUID &guidPropSet,
                                       DWORD dwID,
                                       void *pInstanceData,
                                       DWORD cbInstanceData,
                                       void *pPropData,
                                       DWORD cbPropData)
{
    UNUSED(guidPropSet)
    UNUSED(dwID)
    UNUSED(pInstanceData)
    UNUSED(cbInstanceData)
    UNUSED(pPropData)
    UNUSED(cbPropData)

    return E_NOTIMPL;
}

HRESULT VirtualCameraSourceStream::Get(const GUID &guidPropSet,
                                       DWORD dwPropID,
                                       void *pInstanceData,
                                       DWORD cbInstanceData,
                                       void *pPropData,
                                       DWORD cbPropData,
                                       DWORD *pcbReturned)
{
    UNUSED(pInstanceData)
    UNUSED(cbInstanceData)

    if (guidPropSet != AMPROPSETID_Pin)
        return E_PROP_SET_UNSUPPORTED;

    if (dwPropID != AMPROPERTY_PIN_CATEGORY)
        return E_PROP_ID_UNSUPPORTED;

    if (pPropData == NULL && pcbReturned == NULL)
        return E_POINTER;

    if (pcbReturned)
        *pcbReturned = sizeof(GUID);

    if (pPropData == NULL)
        return S_OK; // Caller just wants to know the size.

    if (cbPropData < sizeof(GUID))
        return E_UNEXPECTED;// The buffer is too small.

    *reinterpret_cast<GUID *>(pPropData) = PIN_CATEGORY_CAPTURE;

    return S_OK;
}

HRESULT VirtualCameraSourceStream::QuerySupported(const GUID &guidPropSet,
                                                  DWORD dwPropID,
                                                  DWORD *pTypeSupport)
{
    if (guidPropSet != AMPROPSETID_Pin)
        return E_PROP_SET_UNSUPPORTED;

    if (dwPropID != AMPROPERTY_PIN_CATEGORY)
        return E_PROP_ID_UNSUPPORTED;

    // We support getting this property, but not setting it.
    if (pTypeSupport)
        *pTypeSupport = KSPROPERTY_SUPPORT_GET;

    return S_OK;
}

HRESULT VirtualCameraSourceStream::GetRange(LONG Property,
                                            LONG *pMin,
                                            LONG *pMax,
                                            LONG *pSteppingDelta,
                                            LONG *pDefault,
                                            LONG *pCapsFlags)
{
    CheckPointer(pMin, E_POINTER);
    CheckPointer(pMax, E_POINTER);
    CheckPointer(pSteppingDelta, E_POINTER);
    CheckPointer(pDefault, E_POINTER);
    CheckPointer(pCapsFlags, E_POINTER);

    switch (Property) {
    case VideoProcAmp_Brightness:
    case VideoProcAmp_Contrast:
    case VideoProcAmp_Saturation:
    case VideoProcAmp_Gamma:
        break;
    case VideoProcAmp_Hue:
        *pMin = -359;
        *pMax = 359;
        *pSteppingDelta = 1;
        *pDefault = 0;
        *pCapsFlags = CameraControl_Flags_Manual;

        return S_OK;
    case VideoProcAmp_ColorEnable:
        *pMin = 0;
        *pMax = 1;
        *pSteppingDelta = 1;
        *pDefault = 1;
        *pCapsFlags = CameraControl_Flags_Manual;

        return S_OK;
    default:
        return E_PROP_ID_UNSUPPORTED;
    }

    *pMin = -255;
    *pMax = 255;
    *pSteppingDelta = 1;
    *pDefault = 0;
    *pCapsFlags = CameraControl_Flags_Manual;

    return S_OK;
}

HRESULT VirtualCameraSourceStream::Set(LONG Property, LONG lValue, LONG Flags)
{
    UNUSED(Flags)

    switch (Property) {
    case VideoProcAmp_Brightness:
    case VideoProcAmp_Contrast:
    case VideoProcAmp_Saturation:
    case VideoProcAmp_Gamma:
        if (abs(lValue) > 255)
            return E_INVALIDARG;

        break;
    case VideoProcAmp_Hue:
        if (abs(lValue) > 359)
            return E_INVALIDARG;

        break;
    case VideoProcAmp_ColorEnable:
        break;
    default:
        return E_PROP_ID_UNSUPPORTED;
    }

    switch (Property) {
    case VideoProcAmp_Brightness:
        imgprop_brightness = lValue;
        break;
    case VideoProcAmp_Contrast:
        imgprop_contrast = lValue;
        break;
    case VideoProcAmp_Hue:
        imgprop_hue = lValue;
        break;
    case VideoProcAmp_Saturation:
        imgprop_saturation = lValue;
        break;
    case VideoProcAmp_Gamma:
        imgprop_gamma = lValue;
        break;
    case VideoProcAmp_ColorEnable:
        imgprop_colorEnable = lValue;
        break;
    default:
        return E_PROP_ID_UNSUPPORTED;
    }

    return S_OK;
}

HRESULT VirtualCameraSourceStream::Get(LONG Property, LONG *lValue, LONG *Flags)
{
    CheckPointer(lValue, E_POINTER);
    CheckPointer(Flags, E_POINTER);

    switch (Property) {
    case VideoProcAmp_Brightness:
        *lValue = imgprop_brightness;
        break;
    case VideoProcAmp_Contrast:
        *lValue = imgprop_contrast;
        break;
    case VideoProcAmp_Hue:
        *lValue = imgprop_hue;
        break;
    case VideoProcAmp_Saturation:
        *lValue = imgprop_saturation;
        break;
    case VideoProcAmp_Gamma:
        *lValue = imgprop_gamma;
        break;
    case VideoProcAmp_ColorEnable:
        *lValue = imgprop_colorEnable;
        break;
    default:
        return E_PROP_ID_UNSUPPORTED;
    }

    *Flags = CameraControl_Flags_Manual;

    return S_OK;
}

HRESULT VirtualCameraSourceStream::CompleteConnect(IPin *pReceivePin)
{
    HRESULT hr;
    VirtualCameraSourceStream *pin = new VirtualCameraSourceStream(&hr,
                                                                   this->m_pFilter,
                                                                   OUTPUT_PIN_NAME);

    UNUSED(pin)
    ASSERT(pin != NULL);

    return CSourceStream::CompleteConnect(pReceivePin);
}

HRESULT VirtualCameraSourceStream::BreakConnect()
{
    if (this->m_pFilter->GetPinCount() > 1)
        for (int i = 0; i < this->m_pFilter->GetPinCount(); i++) {
            CBasePin *pin = this->m_pFilter->GetPin(i);

            if (!pin->IsConnected())
                this->m_pFilter->RemovePin(dynamic_cast<CSourceStream *>(pin));
        }

    return CSourceStream::BreakConnect();
}

ULONG VirtualCameraSourceStream::GetMiscFlags()
{
    return AM_FILTER_MISC_FLAGS_IS_SOURCE;
}

HRESULT VirtualCameraSourceStream::mediaType(int iPosition, CMediaType *pmt) const
{
    if (iPosition < 0)
        return E_INVALIDARG;

    if (iPosition >= VideoFormat::length * FrameResolution::length)
        return VFW_S_NO_MORE_ITEMS;

    VIDEOINFO *pvi = reinterpret_cast<VIDEOINFO *>(pmt->AllocFormatBuffer(sizeof(VIDEOINFO)));

    if (!pvi)
        return E_OUTOFMEMORY;

    ZeroMemory(pvi, sizeof(VIDEOINFO));

    int format = iPosition / FrameResolution::length;
    int resolution = iPosition % FrameResolution::length;

    pvi->bmiHeader.biCompression = VideoFormat::videoFormats[format].compression;
    pvi->bmiHeader.biBitCount = VideoFormat::videoFormats[format].bpp;

    if (pvi->bmiHeader.biCompression == BI_RGB
        && pvi->bmiHeader.biBitCount == 8)
        pvi->bmiHeader.biClrUsed = iPALETTE_COLORS;

    if (pvi->bmiHeader.biCompression == BI_BITFIELDS)
        for (int i = 0; i < 3; i++)
            pvi->TrueColorInfo.dwBitMasks[i] = VideoFormat::videoFormats[format].masks[i];

    HDC hdc = GetDC(NULL);
    PALETTEENTRY palette[iPALETTE_COLORS];

    if (GetSystemPaletteEntries(hdc,
                                0,
                                iPALETTE_COLORS,
                                reinterpret_cast<LPPALETTEENTRY>(&palette)))
        for (int i = 0; i < iPALETTE_COLORS; i++) {
            pvi->TrueColorInfo.bmiColors[i].rgbRed = palette[i].peRed;
            pvi->TrueColorInfo.bmiColors[i].rgbBlue = palette[i].peBlue;
            pvi->TrueColorInfo.bmiColors[i].rgbGreen = palette[i].peGreen;
            pvi->TrueColorInfo.bmiColors[i].rgbReserved = 0;
        }

    ReleaseDC(NULL, hdc);

    pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth = FrameResolution::frameResolutions[resolution].width;
    pvi->bmiHeader.biHeight = FrameResolution::frameResolutions[resolution].height;
    pvi->bmiHeader.biPlanes = 1;
    pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    pvi->AvgTimePerFrame = REFERENCE_TIME(TIME_BASE / this->m_fps);

    SetRectEmpty(&(pvi->rcSource));
    SetRectEmpty(&(pvi->rcTarget));

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    pmt->SetSubtype(&SubTypeGUID);
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);

    return S_OK;
}

HRESULT VirtualCameraSourceStream::isSupported(const CMediaType *pCurMediaType,
                                               const CMediaType *pMediaType) const
{
    if (pCurMediaType && pMediaType->MatchesPartial(pCurMediaType))
        return S_OK;

    if (*pMediaType->Type() != MEDIATYPE_Video
        || !pMediaType->IsFixedSize())
        return E_INVALIDARG;

    VIDEOINFO *pvi = reinterpret_cast<VIDEOINFO *>(pMediaType->Format());

    if (pvi == NULL)
        return E_INVALIDARG;

    const GUID *SubType = pMediaType->Subtype();

    if (SubType == NULL)
        return E_INVALIDARG;

    if (!VideoFormat::byGuid(*SubType))
        return E_INVALIDARG;

    BOOL isSupported = FALSE;

    for (int i = 0; i < FrameResolution::length; i++)
        if (FrameResolution::frameResolutions[i].width == pvi->bmiHeader.biWidth
            && FrameResolution::frameResolutions[i].height == abs(pvi->bmiHeader.biHeight)) {
            isSupported = TRUE;

            break;
        }

    if (!isSupported)
        return E_INVALIDARG;

    return S_OK;
}

Gdiplus::Bitmap *VirtualCameraSourceStream::loadBitmapRC(LPCTSTR lpName)
{
    // Get the resource handler for the picture.
    HRSRC bitmapResource = FindResource(g_hInst,
                                        lpName,
                                        RT_RCDATA);

    if (!bitmapResource)
        return NULL;

    // Read the size of the embedded file.
    DWORD bitmapSize = SizeofResource(g_hInst, bitmapResource);

    if (!bitmapSize)
        return NULL;

    // Load file resource.
    HGLOBAL resource = LoadResource(g_hInst, bitmapResource);

    if (!resource)
        return NULL;

    // Lock resource for reading.
    LPVOID bitmapData = LockResource(resource);

    if (!bitmapData)
        return NULL;

    // Get a handler to the resource data.
    HANDLE bitmapBuffer = GlobalAlloc(GMEM_MOVEABLE, bitmapSize);

    if (!bitmapBuffer)
        return NULL;

    // Get a buffer for reading the raw resource data.
    Gdiplus::Bitmap *bitmap = NULL;
    LPVOID buffer = GlobalLock(bitmapBuffer);

    if (buffer) {
        // Copy the resource data to the buffer.
        CopyMemory(buffer, bitmapData, bitmapSize);
        IStream *stream = NULL;

        // Create a stream for reading the bitmap from the buffer.
        if (CreateStreamOnHGlobal(bitmapBuffer, FALSE, &stream) == S_OK) {
            bitmap = Gdiplus::Bitmap::FromStream(stream);
            stream->Release();
        }

        GlobalUnlock(bitmapBuffer);
    }

    GlobalFree(bitmapBuffer);

    if (!bitmap)
        return NULL;

    return bitmap;
}

Gdiplus::Bitmap *VirtualCameraSourceStream::loadBitmapIPC(LPCTSTR lpName)
{
    IpcBridge ipcBridge;

    if (!ipcBridge.open(lpName))
        return NULL;

    GUID format;
    DWORD width;
    DWORD height;
    BYTE *buffer = NULL;
    size_t frameSize = ipcBridge.read(&format, &width, &height, &buffer);

    if (!buffer || frameSize < 1)
        return NULL;

    Gdiplus::Bitmap *bitmap = new Gdiplus::Bitmap(INT(width),
                                                  INT(height),
                                                  PixelFormat24bppRGB);
    Gdiplus::BitmapData bitmapData;
    Gdiplus::Rect rect(0,
                       0,
                       INT(bitmap->GetWidth()),
                       INT(bitmap->GetHeight()));

    if (bitmap->LockBits(&rect,
                         Gdiplus::ImageLockModeWrite,
                         bitmap->GetPixelFormat(),
                         &bitmapData) == Gdiplus::Ok) {
        rgb3_to_bgr3(bitmapData.Scan0,
                     buffer,
                     INT(bitmap->GetWidth()),
                     INT(bitmap->GetHeight()));
        bitmap->UnlockBits(&bitmapData);
    }

    delete [] buffer;

    return bitmap;
}

bool VirtualCameraSourceStream::readBitmap(LPCTSTR lpName,
                                           LPCTSTR lpAltName,
                                           CMediaType *mediaType,
                                           BYTE *data)
{
    Gdiplus::Bitmap *bitmap = this->loadBitmapIPC(lpName);
    bool deleteBitmap = true;

    if (!bitmap) {
        if (!this->m_bitmap)
            this->m_bitmap = this->loadBitmapRC(lpAltName);

        bitmap = this->m_bitmap;
        deleteBitmap = false;
    }

    // Create a bitmap of the desired format and size.
    VIDEOINFO *fmt = reinterpret_cast<VIDEOINFO *>(mediaType->Format());
    Gdiplus::Bitmap *scaledBitmap = new Gdiplus::Bitmap(fmt->bmiHeader.biWidth,
                                                        fmt->bmiHeader.biHeight,
                                                        PixelFormat24bppRGB);

    if (!scaledBitmap) {
        if (deleteBitmap)
            delete bitmap;

        return false;
    }

    // Draw the loaded picture to the output with the desired size.
    Gdiplus::Graphics graphics(scaledBitmap);
    graphics.DrawImage(bitmap,
                       0,
                       0,
                       INT(scaledBitmap->GetWidth()),
                       INT(scaledBitmap->GetHeight()));

    if (bitmap && deleteBitmap)
        delete bitmap;

    // All RGB formats must be Y-mirrored.
    if (this->m_gdiFormat != PixelFormatUndefined)
        scaledBitmap->RotateFlip(Gdiplus::RotateNoneFlipY);

    // Copy pixel data to the buffer.
    Gdiplus::BitmapData oBitmapData;
    Gdiplus::Rect rect(0,
                       0,
                       INT(scaledBitmap->GetWidth()),
                       INT(scaledBitmap->GetHeight()));

    if (scaledBitmap->LockBits(&rect,
                               Gdiplus::ImageLockModeRead,
                               scaledBitmap->GetPixelFormat(),
                               &oBitmapData) == Gdiplus::Ok) {
        size_t bufferSize = UINT(oBitmapData.Stride) * oBitmapData.Height;
        BYTE *buffer = new BYTE[bufferSize];
        CopyMemory(buffer, oBitmapData.Scan0, bufferSize);

        adjust_image(buffer, buffer,
                     INT(scaledBitmap->GetWidth()),
                     INT(scaledBitmap->GetHeight()),
                     imgprop_hue,
                     imgprop_saturation,
                     imgprop_brightness,
                     imgprop_gamma,
                     imgprop_contrast,
                     !imgprop_colorEnable);

        if (this->m_convert) {
            this->m_convert(data, buffer,
                            INT(scaledBitmap->GetWidth()),
                            INT(scaledBitmap->GetHeight()));
        } else {
            CopyMemory(data, buffer, mediaType->GetSampleSize());
        }

        delete [] buffer;
        scaledBitmap->UnlockBits(&oBitmapData);
    }

    delete scaledBitmap;

    return true;
}
