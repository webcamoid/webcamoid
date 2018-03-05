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

#include <cwchar>
#include <map>
#include <sstream>
#include <combaseapi.h>
#include <fileapi.h>
#include <wincrypt.h>
#include <comdef.h>
#include <initguid.h>
#include <mscoree.h>
#include <inspectable.h>
#include <strmif.h>
#include <amvideo.h>
#include <dvdmedia.h>
#include <uuids.h>
#include <control.h>

#include "utils.h"
#include "referenceclock.h"
#include "VCamUtils/src/image/videoformat.h"

namespace AkVCam
{
    class VideoFormatSpecsPrivate
    {
        public:
            FourCC pixelFormat;
            DWORD compression;
            GUID guid;

            inline static const std::vector<VideoFormatSpecsPrivate> &formats()
            {
                static const std::vector<VideoFormatSpecsPrivate> formats {
                    {PixelFormatRGB32, BI_RGB                        , MEDIASUBTYPE_RGB32 },
                    {PixelFormatRGB24, BI_RGB                        , MEDIASUBTYPE_RGB24 },
                    {PixelFormatRGB16, BI_BITFIELDS                  , MEDIASUBTYPE_RGB565},
                    {PixelFormatRGB15, BI_BITFIELDS                  , MEDIASUBTYPE_RGB555},
                    {PixelFormatUYVY , MAKEFOURCC('U', 'Y', 'V', 'Y'), MEDIASUBTYPE_UYVY  },
                    {PixelFormatYUY2 , MAKEFOURCC('Y', 'U', 'Y', '2'), MEDIASUBTYPE_YUY2  },
                    {PixelFormatNV12 , MAKEFOURCC('N', 'V', '1', '2'), MEDIASUBTYPE_NV12  }
                };

                return formats;
            }

            static inline const VideoFormatSpecsPrivate *byGuid(const GUID &guid)
            {
                for (auto &format: formats())
                    if (IsEqualGUID(format.guid, guid))
                        return &format;

                return nullptr;
            }

            static inline const VideoFormatSpecsPrivate *byPixelFormat(FourCC pixelFormat)
            {
                for (auto &format: formats())
                    if (format.pixelFormat == pixelFormat)
                        return &format;

                return nullptr;
            }
    };
}

bool operator <(const CLSID &a, const CLSID &b)
{
    return AkVCam::stringFromIid(a) < AkVCam::stringFromIid(b);
}

std::string AkVCam::tempPath()
{
    WCHAR tempPath[MAX_PATH];
    GetTempPath(MAX_PATH, tempPath);
    std::wstring wTempPath(tempPath);

    return std::string(wTempPath.begin(), wTempPath.end());
}

std::wstring AkVCam::moduleFileNameW(HINSTANCE hinstDLL)
{
    WCHAR fileName[MAX_PATH];
    GetModuleFileName(hinstDLL, fileName, MAX_PATH);

    return std::wstring(fileName);
}

std::string AkVCam::moduleFileName(HINSTANCE hinstDLL)
{
    auto fileName = moduleFileNameW(hinstDLL);

    return std::string(fileName.begin(), fileName.end());
}

// Converts a human redable string to a CLSID using MD5 hash.
CLSID AkVCam::createClsidFromStr(const std::string &str)
{
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    CLSID clsid;
    DWORD clsidLen = sizeof(CLSID);
    memset(&clsid, 0, sizeof(CLSID));

    if (!CryptAcquireContext(&provider,
                             nullptr,
                             nullptr,
                             PROV_RSA_FULL,
                             CRYPT_VERIFYCONTEXT))
        goto clsidFromStr_failed;

    if (!CryptCreateHash(provider, CALG_MD5, 0, 0, &hash))
        goto clsidFromStr_failed;

    if (!CryptHashData(hash,
                       reinterpret_cast<const BYTE *>(str.c_str()),
                       DWORD(str.size()),
                       0))
        goto clsidFromStr_failed;

    CryptGetHashParam(hash,
                      HP_HASHVAL,
                      reinterpret_cast<BYTE *>(&clsid),
                      &clsidLen,
                      0);

clsidFromStr_failed:
    if (hash)
        CryptDestroyHash(hash);

    if (provider)
        CryptReleaseContext(provider, 0);

    return clsid;
}

std::wstring AkVCam::createClsidWStrFromStr(const std::string &str)
{
    auto clsid = createClsidFromStr(str);
    OLECHAR *clsidWStr = nullptr;

    if (StringFromCLSID(clsid, &clsidWStr) != S_OK)
        return std::wstring();

    std::wstring wstr(clsidWStr);
    CoTaskMemFree(clsidWStr);

    return wstr;
}

std::string AkVCam::stringFromIid(const IID &iid)
{
    auto wstr = wstringFromIid(iid);

    return std::string(wstr.begin(), wstr.end());
}

std::wstring AkVCam::wstringFromIid(const IID &iid)
{
    WCHAR *strIID = nullptr;
    StringFromIID(iid, &strIID);
    std::wstring wstr(strIID);
    CoTaskMemFree(strIID);

    return wstr;
}

std::string AkVCam::stringFromResult(HRESULT result)
{
    auto msg = std::wstring(_com_error(result).ErrorMessage());

    return std::string(msg.begin(), msg.end());
}

std::string AkVCam::stringFromClsid(const CLSID &clsid)
{
    static const std::map<CLSID, std::string> clsidToString {
        {IID_IAgileObject                , "IAgileObject"                }, // x
        {IID_IAMAnalogVideoDecoder       , "IAMAnalogVideoDecoder"       }, // x
        {IID_IAMAudioInputMixer          , "IAMAudioInputMixer"          }, // x
        {IID_IAMAudioRendererStats       , "IAMAudioRendererStats"       }, // x
        {IID_IAMBufferNegotiation        , "IAMBufferNegotiation"        }, // x
        {IID_IAMCameraControl            , "IAMCameraControl"            }, // x
        {IID_IAMClockAdjust              , "IAMClockAdjust"              }, // x
        {IID_IAMCrossbar                 , "IAMCrossbar"                 }, // x
        {IID_IAMDeviceRemoval            , "IAMDeviceRemoval"            }, // Not required
        {IID_IAMFilterMiscFlags          , "IAMFilterMiscFlags"          },
        {IID_IAMOpenProgress             , "IAMOpenProgress"             }, // Not required
        {IID_IAMPushSource               , "IAMPushSource"               },
        {IID_IAMStreamConfig             , "IAMStreamConfig"             },
        {IID_IAMVfwCompressDialogs       , "IAMVfwCompressDialogs"       }, // x
        {IID_IAMVideoCompression         , "IAMVideoCompression"         }, // x
        {IID_IAMVideoControl             , "IAMVideoControl"             }, // x
        {IID_IAMVideoProcAmp             , "IAMVideoProcAmp"             },
        {IID_IBaseFilter                 , "IBaseFilter"                 },
        {IID_IBasicAudio                 , "IBasicAudio"                 }, // Not required
        {IID_IBasicVideo                 , "IBasicVideo"                 }, // Not required
        {IID_IClassFactory               , "IClassFactory"               },
        {IID_IEnumMediaTypes             , "IEnumMediaTypes"             },
        {IID_IEnumPins                   , "IEnumPins"                   },
        {IID_IFileSinkFilter             , "IFileSinkFilter"             }, // x
        {IID_IFileSinkFilter2            , "IFileSinkFilter2"            }, // x
        {IID_IFileSourceFilter           , "IFileSourceFilter"           }, // x
        {IID_IInspectable                , "IInspectable"                }, // x
        {IID_IKsPropertySet              , "IKsPropertySet"              }, // Not required
        {IID_IManagedObject              , "IManagedObject"              }, // x
        {IID_IMarshal                    , "IMarshal"                    }, // x
        {IID_IMediaFilter                , "IMediaFilter"                }, // x
        {IID_IMediaPosition              , "IMediaPosition"              }, // Not required
        {IID_IMediaSample                , "IMediaSample"                },
        {IID_IMediaSample2               , "IMediaSample2"               }, // Defined but not used
        {IID_IMediaSeeking               , "IMediaSeeking"               }, // Not required
        {IID_IMediaEventSink             , "IMediaEventSink"             },
        {IID_IMemAllocator               , "IMemAllocator"               },
        {IID_INoMarshal                  , "INoMarshal"                  }, // x
        {IID_IPersist                    , "IPersist"                    },
        {IID_IPersistPropertyBag         , "IPersistPropertyBag"         },
        {IID_IPin                        , "IPin"                        },
        {IID_IProvideClassInfo           , "IProvideClassInfo"           }, // x
        {IID_IQualityControl             , "IQualityControl"             }, // x
        {IID_IReferenceClock             , "IReferenceClock"             },
        {IID_IRpcOptions                 , "IRpcOptions"                 }, // x
        {IID_ISpecifyPropertyPages       , "ISpecifyPropertyPages"       }, // x
        {IID_IVideoWindow                , "IVideoWindow"                }, // Not required
        {IID_IUnknown                    , "IUnknown"                    },
    };

    /* The interface query tree:
     *
     * IBaseFilter -> IAgileObject
     * IBaseFilter -> IAMCrossbar
     * IBaseFilter -> IAMCameraControl
     * IBaseFilter -> IAMFilterMiscFlags
     * IBaseFilter -> IAMVideoControl
     * IBaseFilter -> IAMVideoProcAmp
     * IBaseFilter -> IFileSinkFilter
     * IBaseFilter -> IFileSinkFilter2
     * IBaseFilter -> IFileSourceFilter
     * IBaseFilter -> IInspectable
     * IBaseFilter -> IManagedObject
     * IBaseFilter -> IMarshal
     * IBaseFilter -> IMediaFilter
     * IBaseFilter -> INoMarshal
     * IBaseFilter -> IProvideClassInfo
     * IBaseFilter -> IReferenceClock
     * IBaseFilter -> IRpcOptions
     * IBaseFilter -> ISpecifyPropertyPages
     * IClassFactory -> IBaseFilter
     * IClassFactory -> IPersistPropertyBag
     * IPersistPropertyBag -> IBaseFilter
     * IPin -> IAMPushSource
     * IPin -> IAMStreamConfig
     * IPin -> IKsPropertySet
     * IPin -> IQualityControl
     */

    for (auto &id: clsidToString)
        if (IsEqualCLSID(id.first, clsid))
            return id.second;

    return stringFromIid(clsid);
}

wchar_t *AkVCam::wcharStrFromWStr(const std::wstring &wstr)
{
    if (wstr.size() < 1)
        return nullptr;

    auto wcstrSize = wstr.size() * sizeof(wchar_t);
    auto wcstr = reinterpret_cast<wchar_t *>(CoTaskMemAlloc(wcstrSize + 1));
    wcstr[wstr.size()] = 0;
    memcpy(wcstr, wstr.data(), wcstrSize);

    return wcstr;
}

AkVCam::FourCC AkVCam::formatFromGuid(const GUID &guid)
{
    auto formatSpec = VideoFormatSpecsPrivate::byGuid(guid);

    if (!formatSpec)
        return 0;

    return formatSpec->pixelFormat;
}

const GUID &AkVCam::guidFromFormat(FourCC format)
{
    auto formatSpec = VideoFormatSpecsPrivate::byPixelFormat(format);

    if (!formatSpec)
        return GUID_NULL;

    return formatSpec->guid;
}

DWORD AkVCam::compressionFromFormat(FourCC format)
{
    auto formatSpec = VideoFormatSpecsPrivate::byPixelFormat(format);

    if (!formatSpec)
        return 0;

    return formatSpec->compression;
}

bool AkVCam::isSubTypeSupported(const GUID &subType)
{
    for (auto &format: VideoFormatSpecsPrivate::formats())
        if (IsEqualGUID(format.guid, subType))
            return true;

    return false;
}

AM_MEDIA_TYPE *AkVCam::mediaTypeFromFormat(const AkVCam::VideoFormat &format)
{
    auto subtype = guidFromFormat(format.fourcc());

    if (IsEqualGUID(subtype, GUID_NULL))
        return nullptr;

    auto frameSize = format.size();

    if (!frameSize)
        return nullptr;

    auto videoInfo =
            reinterpret_cast<VIDEOINFOHEADER *>(CoTaskMemAlloc(sizeof(VIDEOINFOHEADER)));
    memset(videoInfo, 0, sizeof(VIDEOINFOHEADER));

    // Initialize info header.
    videoInfo->rcSource = {0, 0, 0, 0};
    videoInfo->rcTarget = videoInfo->rcSource;
    videoInfo->dwBitRate = DWORD(8 * frameSize * format.minimumFrameRate());
    videoInfo->AvgTimePerFrame =
            REFERENCE_TIME(TIME_BASE / format.minimumFrameRate());

    // Initialize bitmap header.
    videoInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    videoInfo->bmiHeader.biWidth = format.width();
    videoInfo->bmiHeader.biHeight = format.height();
    videoInfo->bmiHeader.biPlanes = 1;
    videoInfo->bmiHeader.biBitCount = WORD(format.bpp());
    videoInfo->bmiHeader.biCompression = compressionFromFormat(format.fourcc());
    videoInfo->bmiHeader.biSizeImage = DWORD(format.size());

    auto mediaType =
            reinterpret_cast<AM_MEDIA_TYPE *>(CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)));
    memset(mediaType, 0, sizeof(AM_MEDIA_TYPE));

    // Initialize media type.
    mediaType->majortype = MEDIATYPE_Video;
    mediaType->subtype = subtype;
    mediaType->bFixedSizeSamples = TRUE;
    mediaType->bTemporalCompression = FALSE;
    mediaType->lSampleSize = ULONG(frameSize);
    mediaType->formattype = FORMAT_VideoInfo;
    mediaType->cbFormat = sizeof(VIDEOINFOHEADER);
    mediaType->pbFormat = reinterpret_cast<BYTE *>(videoInfo);

    return mediaType;
}

AkVCam::VideoFormat AkVCam::formatFromMediaType(const AM_MEDIA_TYPE *mediaType)
{
    if (!mediaType)
        return VideoFormat();

    if (!IsEqualGUID(mediaType->majortype, MEDIATYPE_Video))
        return VideoFormat();

    if (!isSubTypeSupported(mediaType->subtype))
        return VideoFormat();

    if (!mediaType->pbFormat)
        return VideoFormat();

    if (IsEqualGUID(mediaType->formattype, FORMAT_VideoInfo)) {
        auto format = reinterpret_cast<VIDEOINFOHEADER *>(mediaType->pbFormat);

        return VideoFormat(formatFromGuid(mediaType->subtype),
                           format->bmiHeader.biWidth,
                           format->bmiHeader.biHeight,
                           {double(TIME_BASE) / format->AvgTimePerFrame});
    } else if (IsEqualGUID(mediaType->formattype, FORMAT_VideoInfo2)) {
        auto format = reinterpret_cast<VIDEOINFOHEADER2 *>(mediaType->pbFormat);

        return VideoFormat(formatFromGuid(mediaType->subtype),
                           format->bmiHeader.biWidth,
                           format->bmiHeader.biHeight,
                           {double(TIME_BASE) / format->AvgTimePerFrame});
    }

    return VideoFormat();
}

bool AkVCam::isEqualMediaType(const AM_MEDIA_TYPE *mediaType1,
                              const AM_MEDIA_TYPE *mediaType2,
                              bool exact)
{
    if (mediaType1 == mediaType2)
        return true;

    if (!mediaType1 || !mediaType2)
        return false;

    if (!IsEqualGUID(mediaType1->majortype, mediaType2->majortype)
        || !IsEqualGUID(mediaType1->subtype, mediaType2->subtype)
        || !IsEqualGUID(mediaType1->formattype, mediaType2->formattype))
        return false;

    if (mediaType1->pbFormat == mediaType2->pbFormat)
        return true;

    if (exact)
        return memcmp(mediaType1->pbFormat,
                      mediaType2->pbFormat,
                      mediaType1->cbFormat) == 0;

    if (IsEqualGUID(mediaType1->formattype, FORMAT_VideoInfo)) {
        auto format1 = reinterpret_cast<VIDEOINFOHEADER *>(mediaType1->pbFormat);
        auto format2 = reinterpret_cast<VIDEOINFOHEADER *>(mediaType2->pbFormat);

        if (format1->bmiHeader.biWidth == format2->bmiHeader.biWidth
            && format1->bmiHeader.biHeight == format2->bmiHeader.biHeight)
            return true;
    } else if (IsEqualGUID(mediaType1->formattype, FORMAT_VideoInfo2)) {
        auto format1 = reinterpret_cast<VIDEOINFOHEADER2 *>(mediaType1->pbFormat);
        auto format2 = reinterpret_cast<VIDEOINFOHEADER2 *>(mediaType2->pbFormat);

        if (format1->bmiHeader.biWidth == format2->bmiHeader.biWidth
            && format1->bmiHeader.biHeight == format2->bmiHeader.biHeight)
            return true;
    }

    return false;
}

bool AkVCam::copyMediaType(AM_MEDIA_TYPE *dstMediaType,
                           const AM_MEDIA_TYPE *srcMediaType)
{
    if (!dstMediaType)
        return false;

    if (!srcMediaType) {
        memset(dstMediaType, 0, sizeof(AM_MEDIA_TYPE));

        return false;
    }

    memcpy(dstMediaType, srcMediaType, sizeof(AM_MEDIA_TYPE));

    if (dstMediaType->cbFormat && dstMediaType->pbFormat) {
        dstMediaType->pbFormat =
                reinterpret_cast<BYTE *>(CoTaskMemAlloc(dstMediaType->cbFormat));
        memcpy(dstMediaType->pbFormat,
               srcMediaType->pbFormat,
               dstMediaType->cbFormat);
    }

    return true;
}

AM_MEDIA_TYPE *AkVCam::createMediaType(const AM_MEDIA_TYPE *mediaType)
{
    if (!mediaType)
        return nullptr;

    auto newMediaType =
            reinterpret_cast<AM_MEDIA_TYPE *>(CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)));
    memcpy(newMediaType, mediaType, sizeof(AM_MEDIA_TYPE));

    if (newMediaType->cbFormat && newMediaType->pbFormat) {
        newMediaType->pbFormat =
                reinterpret_cast<BYTE *>(CoTaskMemAlloc(newMediaType->cbFormat));
        memcpy(newMediaType->pbFormat,
               mediaType->pbFormat,
               newMediaType->cbFormat);
    }

    return newMediaType;
}

void AkVCam::deleteMediaType(AM_MEDIA_TYPE **mediaType)
{
    if (!mediaType || !*mediaType)
        return;

    auto format = (*mediaType)->pbFormat;

    if (format)
        CoTaskMemFree(format);

    CoTaskMemFree(*mediaType);
    *mediaType = nullptr;
}

bool AkVCam::containsMediaType(const AM_MEDIA_TYPE *mediaType,
                               IEnumMediaTypes *mediaTypes)
{
    AM_MEDIA_TYPE *mt = nullptr;
    mediaTypes->Reset();
    auto isEqual = false;

    while (mediaTypes->Next(1, &mt, nullptr) == S_OK) {
        isEqual = isEqualMediaType(mt, mediaType);
        deleteMediaType(&mt);

        if (isEqual)
            break;
    }

    return isEqual;
}

std::string AkVCam::stringFromMajorType(const GUID &majorType)
{
    static const std::map<GUID, std::string> mtToStr {
        {GUID_NULL               , "GUID_NULL"               },
        {MEDIATYPE_AnalogAudio   , "MEDIATYPE_AnalogAudio"   },
        {MEDIATYPE_AnalogVideo   , "MEDIATYPE_AnalogVideo"   },
        {MEDIATYPE_Audio         , "MEDIATYPE_Audio"         },
        {MEDIATYPE_AUXLine21Data , "MEDIATYPE_AUXLine21Data" },
        {MEDIATYPE_File          , "MEDIATYPE_File"          },
        {MEDIATYPE_Interleaved   , "MEDIATYPE_Interleaved"   },
        {MEDIATYPE_LMRT          , "MEDIATYPE_LMRT"          },
        {MEDIATYPE_Midi          , "MEDIATYPE_Midi"          },
        {MEDIATYPE_MPEG2_PES     , "MEDIATYPE_MPEG2_PES"     },
        {MEDIATYPE_ScriptCommand , "MEDIATYPE_ScriptCommand" },
        {MEDIATYPE_Stream        , "MEDIATYPE_Stream"        },
        {MEDIATYPE_Text          , "MEDIATYPE_Text"          },
        {MEDIATYPE_Timecode      , "MEDIATYPE_Timecode"      },
        {MEDIATYPE_URL_STREAM    , "MEDIATYPE_URL_STREAM"    },
        {MEDIATYPE_VBI           , "MEDIATYPE_VBI"           },
        {MEDIATYPE_Video         , "MEDIATYPE_Video"         }
    };

    for (auto &mediaType: mtToStr)
        if (IsEqualGUID(mediaType.first, majorType))
            return mediaType.second;

    return stringFromIid(majorType);
}

std::string AkVCam::stringFromSubType(const GUID &subType)
{
    static const std::map<GUID, std::string> mstToStr {
        {GUID_NULL               , "GUID_NULL"               },
        {MEDIASUBTYPE_RGB1       , "MEDIASUBTYPE_RGB1"       },
        {MEDIASUBTYPE_RGB4       , "MEDIASUBTYPE_RGB4"       },
        {MEDIASUBTYPE_RGB8       , "MEDIASUBTYPE_RGB8"       },
        {MEDIASUBTYPE_RGB555     , "MEDIASUBTYPE_RGB555"     },
        {MEDIASUBTYPE_RGB565     , "MEDIASUBTYPE_RGB565"     },
        {MEDIASUBTYPE_RGB24      , "MEDIASUBTYPE_RGB24"      },
        {MEDIASUBTYPE_RGB32      , "MEDIASUBTYPE_RGB32"      },
        {MEDIASUBTYPE_ARGB1555   , "MEDIASUBTYPE_ARGB1555"   },
        {MEDIASUBTYPE_ARGB32     , "MEDIASUBTYPE_ARGB32"     },
        {MEDIASUBTYPE_ARGB4444   , "MEDIASUBTYPE_ARGB4444"   },
        {MEDIASUBTYPE_A2R10G10B10, "MEDIASUBTYPE_A2R10G10B10"},
        {MEDIASUBTYPE_A2B10G10R10, "MEDIASUBTYPE_A2B10G10R10"},
        {MEDIASUBTYPE_AYUV       , "MEDIASUBTYPE_AYUV"       },
        {MEDIASUBTYPE_YUY2       , "MEDIASUBTYPE_YUY2"       },
        {MEDIASUBTYPE_UYVY       , "MEDIASUBTYPE_UYVY"       },
        {MEDIASUBTYPE_IMC1       , "MEDIASUBTYPE_IMC1"       },
        {MEDIASUBTYPE_IMC3       , "MEDIASUBTYPE_IMC3"       },
        {MEDIASUBTYPE_IMC2       , "MEDIASUBTYPE_IMC2"       },
        {MEDIASUBTYPE_IMC4       , "MEDIASUBTYPE_IMC4"       },
        {MEDIASUBTYPE_YV12       , "MEDIASUBTYPE_YV12"       },
        {MEDIASUBTYPE_NV12       , "MEDIASUBTYPE_NV12"       },
        {MEDIASUBTYPE_IF09       , "MEDIASUBTYPE_IF09"       },
        {MEDIASUBTYPE_IYUV       , "MEDIASUBTYPE_IYUV"       },
        {MEDIASUBTYPE_Y211       , "MEDIASUBTYPE_Y211"       },
        {MEDIASUBTYPE_Y411       , "MEDIASUBTYPE_Y411"       },
        {MEDIASUBTYPE_Y41P       , "MEDIASUBTYPE_Y41P"       },
        {MEDIASUBTYPE_YVU9       , "MEDIASUBTYPE_YVU9"       },
        {MEDIASUBTYPE_YVYU       , "MEDIASUBTYPE_YVYU"       }
    };

    for (auto &mediaType: mstToStr)
        if (IsEqualGUID(mediaType.first, subType))
            return mediaType.second;

    return stringFromIid(subType);
}

std::string AkVCam::stringFromFormatType(const GUID &formatType)
{
    static const std::map<GUID, std::string> ftToStr {
        {GUID_NULL          , "GUID_NULL"          },
        {FORMAT_DvInfo      , "FORMAT_DvInfo"      },
        {FORMAT_MPEG2Video  , "FORMAT_MPEG2Video"  },
        {FORMAT_MPEGStreams , "FORMAT_MPEGStreams" },
        {FORMAT_MPEGVideo   , "FORMAT_MPEGVideo"   },
        {FORMAT_None        , "FORMAT_None"        },
        {FORMAT_VideoInfo   , "FORMAT_VideoInfo"   },
        {FORMAT_VideoInfo2  , "FORMAT_VideoInfo2"  },
        {FORMAT_WaveFormatEx, "FORMAT_WaveFormatEx"}
    };

    for (auto &mediaType: ftToStr)
        if (IsEqualGUID(mediaType.first, formatType))
            return mediaType.second;

    return stringFromIid(formatType);
}

std::string AkVCam::stringFromMediaType(const AM_MEDIA_TYPE *mediaType)
{
    if (!mediaType)
        return std::string("MediaType(NULL)");

    auto mtStr =
            "MediaType("
            + stringFromMajorType(mediaType->majortype)
            + ", "
            + stringFromSubType(mediaType->subtype)
            + ", "
            + stringFromFormatType(mediaType->formattype)
            + ")";

    return mtStr;
}

std::string AkVCam::stringFromMediaSample(IMediaSample *mediaSample)
{
    if (!mediaSample)
        return std::string("MediaSample(NULL)");

    BYTE *buffer = nullptr;
    mediaSample->GetPointer(&buffer);
    auto bufferSize = mediaSample->GetSize();
    AM_MEDIA_TYPE *mediaType = nullptr;
    mediaSample->GetMediaType(&mediaType);
    REFERENCE_TIME timeStart = 0;
    REFERENCE_TIME timeEnd = 0;
    mediaSample->GetTime(&timeStart, &timeEnd);
    REFERENCE_TIME mediaTimeStart = 0;
    REFERENCE_TIME mediaTimeEnd = 0;
    mediaSample->GetMediaTime(&mediaTimeStart, &mediaTimeEnd);
    auto discontinuity = mediaSample->IsDiscontinuity() == S_OK;
    auto preroll = mediaSample->IsPreroll() == S_OK;
    auto syncPoint = mediaSample->IsSyncPoint() == S_OK;
    auto dataLength = mediaSample->GetActualDataLength();

    std::stringstream ss;
    ss << "MediaSample(" << std::endl;
    ss << "    Buffer: " << size_t(buffer) << std::endl;
    ss << "    Buffer Size: " << bufferSize << std::endl;
    ss << "    Media Type: " << stringFromMediaType(mediaType) << std::endl;
    ss << "    Time: (" << timeStart << ", " << timeEnd << ")" << std::endl;
    ss << "    Media Time: (" << mediaTimeStart << ", " << mediaTimeEnd << ")" << std::endl;
    ss << "    Discontinuity: " << discontinuity << std::endl;
    ss << "    Preroll: " << preroll << std::endl;
    ss << "    Sync Point: " << syncPoint << std::endl;
    ss << "    Data Length: " << dataLength << std::endl;
    ss << ")";

    deleteMediaType(&mediaType);

    return ss.str();
}
