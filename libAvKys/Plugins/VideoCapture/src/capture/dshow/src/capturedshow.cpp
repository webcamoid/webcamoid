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

#include <QCoreApplication>
#include <QDateTime>
#include <QMap>
#include <QReadWriteLock>
#include <QSharedPointer>
#include <QSize>
#include <QVariant>
#include <QWaitCondition>
#include <ak.h>
#include <akcaps.h>
#include <akcompressedvideocaps.h>
#include <akcompressedvideopacket.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideopacket.h>
#include <dshow.h>
#include <dbt.h>
#include <dvdmedia.h>
#include <aviriff.h>
#include <mmsystem.h>
#include <usbiodef.h>
#include <uuids.h>
#include <wmcodecdsp.h>

#include "capturedshow.h"
#include "framegrabber.h"

#define TIME_BASE 1.0e7
#define SOURCE_FILTER_NAME L"Source"

#define AK_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    static const GUID name = {l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}

AK_DEFINE_GUID(AK_MEDIASUBTYPE_AVC1, 0x31435641, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
AK_DEFINE_GUID(AK_MEDIASUBTYPE_H264, 0x34363248, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
AK_DEFINE_GUID(AK_MEDIASUBTYPE_h264, 0x34363268, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
AK_DEFINE_GUID(AK_MEDIASUBTYPE_X264, 0x34363258, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
AK_DEFINE_GUID(AK_MEDIASUBTYPE_x264, 0x34363278, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

DEFINE_GUID(CLSID_SampleGrabber, 0xc1f400a0, 0x3f08, 0x11d3, 0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37);
DEFINE_GUID(CLSID_NullRenderer, 0xc1f400a4, 0x3f08, 0x11d3, 0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37);

Q_CORE_EXPORT HINSTANCE qWinAppInst();

__inline bool operator <(REFGUID guid1, REFGUID guid2)
{
    return guid1.Data1 < guid2.Data1;
}

using VideoProcAmpPropertyMap = QMap<VideoProcAmpProperty, QString>;

inline VideoProcAmpPropertyMap initVideoProcAmpPropertyMap()
{
    VideoProcAmpPropertyMap vpapToStr {
        {VideoProcAmp_Brightness           , "Brightness"            },
        {VideoProcAmp_Contrast             , "Contrast"              },
        {VideoProcAmp_Hue                  , "Hue"                   },
        {VideoProcAmp_Saturation           , "Saturation"            },
        {VideoProcAmp_Sharpness            , "Sharpness"             },
        {VideoProcAmp_Gamma                , "Gamma"                 },
        {VideoProcAmp_ColorEnable          , "Color Enable"          },
        {VideoProcAmp_WhiteBalance         , "White Balance"         },
        {VideoProcAmp_BacklightCompensation, "Backlight Compensation"},
        {VideoProcAmp_Gain                 , "Gain"                  }
    };

    return vpapToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(VideoProcAmpPropertyMap, vpapToStr, (initVideoProcAmpPropertyMap()))

using CameraControlMap = QMap<CameraControlProperty, QString>;

inline CameraControlMap initCameraControlMap()
{
    CameraControlMap ccToStr {
        {CameraControl_Pan     , "Pan"     },
        {CameraControl_Tilt    , "Tilt"    },
        {CameraControl_Roll    , "Roll"    },
        {CameraControl_Zoom    , "Zoom"    },
        {CameraControl_Exposure, "Exposure"},
        {CameraControl_Iris    , "Iris"    },
        {CameraControl_Focus   , "Focus"   }
    };

    return ccToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(CameraControlMap, ccToStr, (initCameraControlMap()))

using RawFmtToAkFmtMap = QMap<GUID, AkVideoCaps::PixelFormat>;

inline RawFmtToAkFmtMap initRawFmtToAkFmt()
{
    RawFmtToAkFmtMap rawFmtToAkFmt {
        {MEDIASUBTYPE_ARGB1555, AkVideoCaps::Format_argb1555},
        {MEDIASUBTYPE_ARGB32  , AkVideoCaps::Format_argbpack},
        {MEDIASUBTYPE_ARGB4444, AkVideoCaps::Format_argb4444},
        {MEDIASUBTYPE_AYUV    , AkVideoCaps::Format_ayuvpack},
        {MEDIASUBTYPE_IF09    , AkVideoCaps::Format_yvu410p },
        {MEDIASUBTYPE_IYUV    , AkVideoCaps::Format_yuv420p },
        {MEDIASUBTYPE_NV12    , AkVideoCaps::Format_nv12    },
        {MEDIASUBTYPE_RGB24   , AkVideoCaps::Format_bgr24   },
        {MEDIASUBTYPE_RGB32   , AkVideoCaps::Format_xrgbpack},
        {MEDIASUBTYPE_RGB555  , AkVideoCaps::Format_rgb555  },
        {MEDIASUBTYPE_RGB565  , AkVideoCaps::Format_rgb565  },
        {MEDIASUBTYPE_UYVY    , AkVideoCaps::Format_uyvy422 },
        {MEDIASUBTYPE_Y211    , AkVideoCaps::Format_yuyv211 },
        {MEDIASUBTYPE_Y41P    , AkVideoCaps::Format_uyvy411 },
        {MEDIASUBTYPE_YUY2    , AkVideoCaps::Format_yuyv422 },
        {MEDIASUBTYPE_YUYV    , AkVideoCaps::Format_yuyv422 },
        {MEDIASUBTYPE_YV12    , AkVideoCaps::Format_yvu420p },
        {MEDIASUBTYPE_YVU9    , AkVideoCaps::Format_yvu410p },
        {MEDIASUBTYPE_YVYU    , AkVideoCaps::Format_yvyu422 },
    };

    return rawFmtToAkFmt;
}

Q_GLOBAL_STATIC_WITH_ARGS(RawFmtToAkFmtMap, rawFmtToAkFmt, (initRawFmtToAkFmt()))

using CompressedFormatToStrMap = QMap<GUID, QString>;

inline CompressedFormatToStrMap initCompressedFormatToStr()
{
    CompressedFormatToStrMap compressedFormatToStr {
        {MEDIASUBTYPE_CFCC   , "mjpg"  },
        {MEDIASUBTYPE_IJPG   , "jpeg"  },
        {MEDIASUBTYPE_MDVF   , "dv"    },
        {MEDIASUBTYPE_MJPG   , "mjpg"  },
        {MEDIASUBTYPE_Plum   , "mjpg"  },
        {MEDIASUBTYPE_QTJpeg , "jpeg"  },
        {MEDIASUBTYPE_QTRle  , "qtrle" },
        {MEDIASUBTYPE_QTRpza , "qtrpza"},
        {MEDIASUBTYPE_QTSmc  , "qtsmc" },
        {MEDIASUBTYPE_TVMJ   , "mjpg"  },
        {MEDIASUBTYPE_WAKE   , "mjpg"  },
        {MEDIASUBTYPE_dv25   , "dv25"  },
        {MEDIASUBTYPE_dv50   , "dv50"  },
        {MEDIASUBTYPE_dvh1   , "dvh1"  },
        {MEDIASUBTYPE_dvhd   , "dvhd"  },
        {MEDIASUBTYPE_dvsd   , "dvsd"  },
        {MEDIASUBTYPE_dvsl   , "dvsl"  },
        {AK_MEDIASUBTYPE_AVC1, "h264"  },
        {AK_MEDIASUBTYPE_H264, "h264"  },
        {AK_MEDIASUBTYPE_h264, "h264"  },
        {AK_MEDIASUBTYPE_X264, "h264"  },
        {AK_MEDIASUBTYPE_x264, "h264"  },
    };

    return compressedFormatToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(CompressedFormatToStrMap,
                          compressedFormatToStr,
                          (initCompressedFormatToStr()))

using IoMethodMap = QMap<CaptureDShow::IoMethod, QString>;

inline IoMethodMap initIoMethodMap()
{
    IoMethodMap ioMethodToStr {
        {CaptureDShow::IoMethodDirectRead, "directRead"},
        {CaptureDShow::IoMethodGrabSample, "grabSample"},
        {CaptureDShow::IoMethodGrabBuffer, "grabBuffer"}
    };

    return ioMethodToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(IoMethodMap, ioMethodToStr, (initIoMethodMap()))

using BaseFilterPtr = QSharedPointer<IBaseFilter>;
using SampleGrabberPtr = QSharedPointer<ISampleGrabber>;
using MonikerPtr = QSharedPointer<IMoniker>;
using MonikersMap = QMap<QString, MonikerPtr>;
using MediaTypePtr = QSharedPointer<AM_MEDIA_TYPE>;
using MediaTypesList = QList<MediaTypePtr>;
using PinPtr = QSharedPointer<IPin>;
using PinList = QList<PinPtr>;

class CaptureDShowPrivate
{
    public:
        CaptureDShow *self;
        QString m_device;
        QList<int> m_streams;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, CaptureVideoCaps> m_devicesCaps;
        qint64 m_id {-1};
        AkFrac m_timeBase;
        CaptureDShow::IoMethod m_ioMethod {CaptureDShow::IoMethodGrabSample};
        QMap<QString, QSize> m_resolution;
        BaseFilterPtr m_webcamFilter;
        IGraphBuilder *m_graph {nullptr};
        SampleGrabberPtr m_grabber;
        FrameGrabber m_frameGrabber;
        QByteArray m_curBuffer;
        AM_MEDIA_TYPE *m_curMediaType {nullptr};
        QReadWriteLock m_mutex;
        QReadWriteLock m_controlsMutex;
        QWaitCondition m_waitCondition;
        QVariantList m_globalImageControls;
        QVariantList m_globalCameraControls;
        QVariantMap m_localImageControls;
        QVariantMap m_localCameraControls;

        explicit CaptureDShowPrivate(CaptureDShow *self);
        QString devicePath(IPropertyBag *propertyBag) const;
        QString deviceDescription(IPropertyBag *propertyBag) const;
        CaptureVideoCaps caps(IBaseFilter *baseFilter) const;
        AkVideoCaps::PixelFormat nearestFormat(const BITMAPINFOHEADER *bitmapHeader) const;
        AkCaps capsFromMediaType(const AM_MEDIA_TYPE *mediaType,
                                 bool *isRaw=nullptr,
                                 size_t *lineSize=nullptr,
                                 bool *mirror=nullptr) const;
        AkCaps capsFromMediaType(const MediaTypePtr &mediaType,
                                 bool *isRaw=nullptr,
                                 size_t *lineSize=nullptr,
                                 bool *mirror=nullptr) const;
        HRESULT enumerateCameras(IEnumMoniker **ppEnum) const;
        MonikersMap listMonikers() const;
        MonikerPtr findMoniker(const QString &webcam) const;
        QString monikerDisplayName(IMoniker *moniker) const;
        IBaseFilter *findFilterP(const QString &webcam) const;
        BaseFilterPtr findFilter(const QString &webcam) const;
        MediaTypesList listMediaTypes(IBaseFilter *filter) const;
        bool isPinConnected(IPin *pPin, bool *ok=nullptr) const;
        PinPtr findUnconnectedPin(IBaseFilter *pFilter,
                                  PIN_DIRECTION PinDir) const;
        bool connectFilters(IGraphBuilder *pGraph,
                            IBaseFilter *pSrc,
                            IBaseFilter *pDest) const;
        PinList enumPins(IBaseFilter *filter,
                         PIN_DIRECTION direction) const;
        static void freeMediaType(AM_MEDIA_TYPE &mediaType);
        static void deleteMediaType(AM_MEDIA_TYPE *mediaType);
        QVariantList imageControls(IBaseFilter *filter) const;
        bool setImageControls(IBaseFilter *filter,
                              const QVariantMap &imageControls) const;
        QVariantList cameraControls(IBaseFilter *filter) const;
        bool setCameraControls(IBaseFilter *filter,
                               const QVariantMap &cameraControls) const;
        QVariantMap controlStatus(const QVariantList &controls) const;
        QVariantMap mapDiff(const QVariantMap &map1,
                            const QVariantMap &map2) const;
        void frameReceived(qreal time, const QByteArray &buffer);
        void sampleReceived(qreal time, IMediaSample *sample);
        AkPacket processFrame(const AM_MEDIA_TYPE *mediaType,
                              const QByteArray &buffer) const;
        void updateDevices();
};

CaptureDShow::CaptureDShow(QObject *parent):
    Capture(parent),
    QAbstractNativeEventFilter()
{
    this->d = new CaptureDShowPrivate(this);
    QObject::connect(&this->d->m_frameGrabber,
                     &FrameGrabber::frameReady,
                     [this] (qreal time, const QByteArray &packet) {
                        this->d->frameReceived(time, packet);
                     });
    QObject::connect(&this->d->m_frameGrabber,
                     &FrameGrabber::sampleReady,
                     [this] (qreal time, IMediaSample *sample) {
                        this->d->sampleReceived(time, sample);
                     });
    qApp->installNativeEventFilter(this);
    this->d->updateDevices();
}

CaptureDShow::~CaptureDShow()
{
    qApp->removeNativeEventFilter(this);

    if (this->d->m_curMediaType) {
        this->d->freeMediaType(*this->d->m_curMediaType);
        this->d->m_curMediaType = nullptr;
    }

    delete this->d;
}

QStringList CaptureDShow::webcams() const
{
    return this->d->m_devices;
}

QString CaptureDShow::device() const
{
    return this->d->m_device;
}

QList<int> CaptureDShow::streams()
{
    if (!this->d->m_streams.isEmpty())
        return this->d->m_streams;

    auto caps = this->caps(this->d->m_device);

    if (caps.isEmpty())
        return {};

    return {0};
}

QList<int> CaptureDShow::listTracks(AkCaps::CapsType type)
{
    if (type != AkCaps::CapsVideo && type != AkCaps::CapsUnknown)
        return {};

    auto caps = this->caps(this->d->m_device);
    QList<int> streams;

    for (int i = 0; i < caps.count(); i++)
        streams << i;

    return streams;
}

QString CaptureDShow::ioMethod() const
{
    return ioMethodToStr->value(this->d->m_ioMethod, "any");
}

int CaptureDShow::nBuffers() const
{
    return 0;
}

QString CaptureDShow::description(const QString &webcam) const
{
    return this->d->m_descriptions.value(webcam);
}

CaptureVideoCaps CaptureDShow::caps(const QString &webcam) const
{
    return this->d->m_devicesCaps.value(webcam);
}

QVariantList CaptureDShow::imageControls() const
{
    return this->d->m_globalImageControls;
}

bool CaptureDShow::setImageControls(const QVariantMap &imageControls)
{
    this->d->m_controlsMutex.lockForRead();
    auto globalImageControls = this->d->m_globalImageControls;
    this->d->m_controlsMutex.unlock();

    for (int i = 0; i < globalImageControls.count(); i++) {
        auto control = globalImageControls[i].toList();
        auto controlName = control[0].toString();

        if (imageControls.contains(controlName)) {
            control[6] = imageControls[controlName];
            globalImageControls[i] = control;
        }
    }

    this->d->m_controlsMutex.lockForWrite();

    if (this->d->m_globalImageControls == globalImageControls) {
        this->d->m_controlsMutex.unlock();

        return false;
    }

    this->d->m_globalImageControls = globalImageControls;
    this->d->m_controlsMutex.unlock();

    emit this->imageControlsChanged(imageControls);

    return true;
}

bool CaptureDShow::resetImageControls()
{
    QVariantMap controls;

    for (auto &control: this->imageControls()) {
        auto params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setImageControls(controls);
}

QVariantList CaptureDShow::cameraControls() const
{
    return this->d->m_globalCameraControls;
}

bool CaptureDShow::setCameraControls(const QVariantMap &cameraControls)
{
    this->d->m_controlsMutex.lockForRead();
    auto globalCameraControls = this->d->m_globalCameraControls;
    this->d->m_controlsMutex.unlock();

    for (int i = 0; i < globalCameraControls.count(); i++) {
        auto control = globalCameraControls[i].toList();
        auto controlName = control[0].toString();

        if (cameraControls.contains(controlName)) {
            control[6] = cameraControls[controlName];
            globalCameraControls[i] = control;
        }
    }

    this->d->m_controlsMutex.lockForWrite();

    if (this->d->m_globalCameraControls == globalCameraControls) {
        this->d->m_controlsMutex.unlock();

        return false;
    }

    this->d->m_globalCameraControls = globalCameraControls;
    this->d->m_controlsMutex.unlock();
    emit this->cameraControlsChanged(cameraControls);

    return true;
}

bool CaptureDShow::resetCameraControls()
{
    QVariantMap controls;

    for (auto &control: this->cameraControls()) {
        auto params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setCameraControls(controls);
}

AkPacket CaptureDShow::readFrame()
{
    IBaseFilter *source = nullptr;
    this->d->m_graph->FindFilterByName(SOURCE_FILTER_NAME, &source);

    if (source) {
        this->d->m_controlsMutex.lockForRead();
        auto imageControls = this->d->controlStatus(this->d->m_globalImageControls);
        this->d->m_controlsMutex.unlock();

        if (this->d->m_localImageControls != imageControls) {
            auto controls = this->d->mapDiff(this->d->m_localImageControls,
                                             imageControls);
            this->d->setImageControls(source, controls);
            this->d->m_localImageControls = imageControls;
        }

        this->d->m_controlsMutex.lockForRead();
        auto cameraControls = this->d->controlStatus(this->d->m_globalCameraControls);
        this->d->m_controlsMutex.unlock();

        if (this->d->m_localCameraControls != cameraControls) {
            auto controls = this->d->mapDiff(this->d->m_localCameraControls,
                                             cameraControls);
            this->d->setCameraControls(source, controls);
            this->d->m_localCameraControls = cameraControls;
        }

        source->Release();
    }

    AkPacket packet;

    if (this->d->m_ioMethod != IoMethodDirectRead) {
        this->d->m_mutex.lockForWrite();

        if (this->d->m_curBuffer.isEmpty())
            this->d->m_waitCondition.wait(&this->d->m_mutex, 1000);

        if (!this->d->m_curBuffer.isEmpty()) {
            if (this->d->m_curMediaType) {
                packet = this->d->processFrame(this->d->m_curMediaType,
                                               this->d->m_curBuffer);
            } else {
                AM_MEDIA_TYPE mediaType;
                ZeroMemory(&mediaType, sizeof(AM_MEDIA_TYPE));
                this->d->m_grabber->GetConnectedMediaType(&mediaType);
                packet = this->d->processFrame(&mediaType,
                                               this->d->m_curBuffer);
                this->d->freeMediaType(mediaType);
            }

            this->d->m_curBuffer.clear();
        }

        this->d->m_mutex.unlock();
    } else {
        IMediaSample *mediaSample = nullptr;

        if (SUCCEEDED(this->d->m_grabber->GetCurrentSample(&mediaSample))) {
            BYTE *data = nullptr;

            if (SUCCEEDED(mediaSample->GetPointer(&data))){
                AM_MEDIA_TYPE *sampleMediaType = nullptr;

                if (SUCCEEDED(mediaSample->GetMediaType(&sampleMediaType))) {
                    QByteArray oBuffer(reinterpret_cast<char *>(data),
                                       int(mediaSample->GetSize()));

                    if (sampleMediaType) {
                        packet = this->d->processFrame(sampleMediaType, oBuffer);
                        CaptureDShowPrivate::deleteMediaType(sampleMediaType);
                    } else {
                        AM_MEDIA_TYPE mediaType;
                        ZeroMemory(&mediaType, sizeof(AM_MEDIA_TYPE));
                        this->d->m_grabber->GetConnectedMediaType(&mediaType);
                        packet = this->d->processFrame(&mediaType, oBuffer);
                        this->d->freeMediaType(mediaType);
                    }
                }
            }

            mediaSample->Release();
        }
    }

    return packet;
}

bool CaptureDShow::nativeEventFilter(const QByteArray &eventType,
                                     void *message,
                                     qintptr *result)
{
    Q_UNUSED(eventType)

    if (!message)
        return false;

    auto msg = reinterpret_cast<MSG *>(message);

    if (msg->message == WM_DEVICECHANGE) {
        switch (msg->wParam) {
        case DBT_DEVICEARRIVAL:
        case DBT_DEVICEREMOVECOMPLETE:
        case DBT_DEVNODES_CHANGED: {
            this->d->updateDevices();

            if (result)
                *result = TRUE;

            return true;
        }
        default:
            break;
        }
    }

    return false;
}

CaptureDShowPrivate::CaptureDShowPrivate(CaptureDShow *self):
    self(self)
{
}

QString CaptureDShowPrivate::devicePath(IPropertyBag *propertyBag) const
{
    VARIANT var;
    VariantInit(&var);
    auto hr = propertyBag->Read(L"DevicePath", &var, nullptr);
    QString devicePath;

    if (SUCCEEDED(hr))
        devicePath = QString::fromWCharArray(var.bstrVal);

    VariantClear(&var);

    return devicePath;
}

QString CaptureDShowPrivate::deviceDescription(IPropertyBag *propertyBag) const
{

    VARIANT var;
    VariantInit(&var);
    auto hr = propertyBag->Read(L"Description", &var, nullptr);

    if (FAILED(hr))
        hr = propertyBag->Read(L"FriendlyName", &var, nullptr);

    QString description;

    if (SUCCEEDED(hr))
        description = QString::fromWCharArray(var.bstrVal);

    VariantClear(&var);

    return description;
}

CaptureVideoCaps CaptureDShowPrivate::caps(IBaseFilter *baseFilter) const
{
    auto pins = this->enumPins(baseFilter, PINDIR_OUTPUT);
    CaptureVideoCaps caps;

    for (auto &pin: pins) {
        IEnumMediaTypes *pEnum = nullptr;

        if (FAILED(pin->EnumMediaTypes(&pEnum)))
            continue;

        pEnum->Reset();
        AM_MEDIA_TYPE *mediaType = nullptr;

        while (pEnum->Next(1, &mediaType, nullptr) == S_OK) {
            if (mediaType->formattype == FORMAT_VideoInfo
                && mediaType->cbFormat >= sizeof(VIDEOINFOHEADER)
                && mediaType->pbFormat != nullptr) {
                auto videoCaps = this->capsFromMediaType(mediaType);

                if (videoCaps)
                    caps << videoCaps;
            }

            this->deleteMediaType(mediaType);
        }

        pEnum->Release();
    }

    return caps;
}

AkVideoCaps::PixelFormat CaptureDShowPrivate::nearestFormat(const BITMAPINFOHEADER *bitmapHeader) const
{
    static const QMap<quint32, AkVideoCaps::PixelFormat> fourccToAk {
        {MAKEFOURCC('A', 'Y', 'U', 'V'), AkVideoCaps::Format_ayuvpack},
        {MAKEFOURCC('I', 'F', '0', '9'), AkVideoCaps::Format_yvu410p },
        {MAKEFOURCC('I', 'Y', 'U', 'V'), AkVideoCaps::Format_yuv420p },
        {MAKEFOURCC('N', 'V', '1', '2'), AkVideoCaps::Format_nv12    },
        {MAKEFOURCC('U', 'Y', 'V', 'Y'), AkVideoCaps::Format_uyvy422 },
        {MAKEFOURCC('Y', '2', '1', '1'), AkVideoCaps::Format_yuyv211 },
        {MAKEFOURCC('Y', '4', '1', 'P'), AkVideoCaps::Format_uyvy411 },
        {MAKEFOURCC('Y', 'U', 'Y', '2'), AkVideoCaps::Format_yuyv422 },
        {MAKEFOURCC('Y', 'U', 'Y', 'V'), AkVideoCaps::Format_yuyv422 },
        {MAKEFOURCC('Y', 'V', '1', '2'), AkVideoCaps::Format_yvu420p },
        {MAKEFOURCC('Y', 'V', 'U', '9'), AkVideoCaps::Format_yvu410p },
        {MAKEFOURCC('Y', 'V', 'Y', 'U'), AkVideoCaps::Format_yvyu422 },
    };

    if (bitmapHeader->biCompression != BI_RGB
        && bitmapHeader->biCompression != BI_BITFIELDS) {
        return fourccToAk.value(bitmapHeader->biCompression,
                                AkVideoCaps::Format_none);
    }

    static const DWORD mask555[] = {0x007c00, 0x0003e0, 0x00001f};
    static const DWORD mask565[] = {0x007c00, 0x0003e0, 0x00001f};

    switch(bitmapHeader->biBitCount) {
    case 16: {
        if (bitmapHeader->biCompression == BI_RGB)
            return AkVideoCaps::Format_rgb555;

        auto bitmapInfo = reinterpret_cast<const BITMAPINFO *>(bitmapHeader);
        auto mask = reinterpret_cast<const DWORD *>(bitmapInfo->bmiColors);

        if (memcmp(mask, mask555, 3 * sizeof(DWORD)) == 0)
            return AkVideoCaps::Format_rgb555;

        if (memcmp(mask, mask565, 3 * sizeof(DWORD)) == 0)
            return AkVideoCaps::Format_rgb565;
    }
    case 24:
        return AkVideoCaps::Format_bgr24;
    case 32:
        return AkVideoCaps::Format_xrgbpack;
    default:
        break;
    }

    return AkVideoCaps::Format_none;
}

AkCaps CaptureDShowPrivate::capsFromMediaType(const AM_MEDIA_TYPE *mediaType,
                                              bool *isRaw,
                                              size_t *lineSize,
                                              bool *mirror) const
{
    if (!mediaType || !mediaType->pbFormat)
        return {};

    AkVideoCaps::PixelFormat format = AkVideoCaps::Format_none;
    DWORD biCompression = 0;
    WORD biBitCount = 0;
    LONG biWidth = 0;
    LONG biHeight = 0;
    RECT rcTarget;
    memset(&rcTarget, 0, sizeof(RECT));
    REFERENCE_TIME AvgTimePerFrame = 0;
    bool isRawFmt = false;

    if (IsEqualGUID(mediaType->formattype, FORMAT_VideoInfo)) {
        auto videoInfoHeader =
                reinterpret_cast<VIDEOINFOHEADER *>(mediaType->pbFormat);
        biCompression = videoInfoHeader->bmiHeader.biCompression;
        biBitCount = videoInfoHeader->bmiHeader.biBitCount;
        biWidth = videoInfoHeader->bmiHeader.biWidth;
        biHeight = videoInfoHeader->bmiHeader.biHeight;
        memcpy(&rcTarget, &videoInfoHeader->rcTarget, sizeof(RECT));
        AvgTimePerFrame = videoInfoHeader->AvgTimePerFrame;
        format = this->nearestFormat(&videoInfoHeader->bmiHeader);
        isRawFmt = format != AkVideoCaps::Format_none;
    } else if (IsEqualGUID(mediaType->formattype, FORMAT_VideoInfo2)) {
        auto videoInfoHeader =
                reinterpret_cast<VIDEOINFOHEADER2 *>(mediaType->pbFormat);
        biCompression = videoInfoHeader->bmiHeader.biCompression;
        biBitCount = videoInfoHeader->bmiHeader.biBitCount;
        biWidth = videoInfoHeader->bmiHeader.biWidth;
        biHeight = videoInfoHeader->bmiHeader.biHeight;
        memcpy(&rcTarget, &videoInfoHeader->rcTarget, sizeof(RECT));
        AvgTimePerFrame = videoInfoHeader->AvgTimePerFrame;
        isRawFmt = format != AkVideoCaps::Format_none;
        format = this->nearestFormat(&videoInfoHeader->bmiHeader);
        isRawFmt = format != AkVideoCaps::Format_none;
    } else {
        return {};
    }

    if (isRaw)
        *isRaw = isRawFmt;

    if (lineSize)
        *lineSize = ((((biWidth * biBitCount) + 31) & ~31) >> 3);

    if (mirror)
        *mirror =
            (biCompression == BI_RGB || biCompression == BI_BITFIELDS)
            && biHeight > 0;

    int width = rcTarget.right - rcTarget.left;

    if (width < 1)
        width = int(qAbs(biWidth));

    int height = rcTarget.bottom - rcTarget.top;

    if (height < 1)
        height = int(qAbs(biHeight));

    AkFrac fps = AvgTimePerFrame < 1? AkFrac(30, 1): AkFrac(TIME_BASE, AvgTimePerFrame);

    if (isRawFmt) {
        return AkVideoCaps(format, width, height, fps);
    } else if (compressedFormatToStr->contains(mediaType->subtype)) {
        return AkCompressedVideoCaps(compressedFormatToStr->value(mediaType->subtype),
                                     width,
                                     height,
                                     fps);
    }

    return {};
}

AkCaps CaptureDShowPrivate::capsFromMediaType(const MediaTypePtr &mediaType,
                                              bool *isRaw,
                                              size_t *lineSize,
                                              bool *mirror) const
{
    return this->capsFromMediaType(mediaType.data(), isRaw, lineSize, mirror);
}

HRESULT CaptureDShowPrivate::enumerateCameras(IEnumMoniker **ppEnum) const
{
    // Create the System Device Enumerator.
    ICreateDevEnum *pDevEnum = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum,
                                  nullptr,
                                  CLSCTX_INPROC_SERVER,
                                  IID_ICreateDevEnum,
                                  reinterpret_cast<void **>(&pDevEnum));

    if (SUCCEEDED(hr)) {
        // Create an enumerator for the category.
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
                                             ppEnum,
                                             0);

        if (hr == S_FALSE)
            hr = VFW_E_NOT_FOUND;

        pDevEnum->Release();
    }

    return hr;
}

MonikersMap CaptureDShowPrivate::listMonikers() const
{
    MonikersMap monikers;
    IEnumMoniker *pEnum = nullptr;
    HRESULT hr = this->enumerateCameras(&pEnum);

    if (SUCCEEDED(hr)) {
        pEnum->Reset();
        IMoniker *pMoniker = nullptr;

        for (int i = 0; pEnum->Next(1, &pMoniker, nullptr) == S_OK; i++) {
            IPropertyBag *pPropBag = nullptr;
            HRESULT hr = pMoniker->BindToStorage(nullptr,
                                                 nullptr,
                                                 IID_IPropertyBag,
                                                 reinterpret_cast<void **>(&pPropBag));

            if (FAILED(hr)) {
                pMoniker->Release();

                continue;
            }

            auto devicePath = this->devicePath(pPropBag);

            if (devicePath.isEmpty())
                devicePath = this->monikerDisplayName(pMoniker);

            monikers[devicePath] =
                    MonikerPtr(pMoniker, [] (IMoniker *moniker) {
                        moniker->Release();
                    });
            pPropBag->Release();
        }

        pEnum->Release();
    }

    return monikers;
}

MonikerPtr CaptureDShowPrivate::findMoniker(const QString &webcam) const
{
    auto monikers = this->listMonikers();

    if (monikers.contains(webcam))
        return monikers[webcam];

    return {};
}

QString CaptureDShowPrivate::monikerDisplayName(IMoniker *moniker) const
{
    IBindCtx *bind_ctx = nullptr;

    if (FAILED(CreateBindCtx(0, &bind_ctx)))
        return {};

    LPOLESTR olestr = nullptr;
    auto result = moniker->GetDisplayName(bind_ctx, nullptr, &olestr);
    bind_ctx->Release();

    if (FAILED(result))
        return QString();

    return QString::fromWCharArray(olestr);
}

IBaseFilter *CaptureDShowPrivate::findFilterP(const QString &webcam) const
{
    auto moniker = this->findMoniker(webcam);

    if (!moniker)
        return nullptr;

    IBaseFilter *filter = nullptr;
    HRESULT hr = moniker->BindToObject(nullptr,
                                       nullptr,
                                       IID_IBaseFilter,
                                       reinterpret_cast<void **>(&filter));

    if (FAILED(hr))
        return nullptr;

    return filter;
}

BaseFilterPtr CaptureDShowPrivate::findFilter(const QString &webcam) const
{
    auto filter = this->findFilterP(webcam);

    if (!filter)
        return {};

    return BaseFilterPtr(filter, [] (IBaseFilter *filter) {
        filter->Release();
    });
}

MediaTypesList CaptureDShowPrivate::listMediaTypes(IBaseFilter *filter) const
{
    auto pins = this->enumPins(filter, PINDIR_OUTPUT);
    MediaTypesList mediaTypes;

    for (auto &pin: pins) {
        IEnumMediaTypes *pEnum = nullptr;

        if (FAILED(pin->EnumMediaTypes(&pEnum)))
            continue;

        pEnum->Reset();
        AM_MEDIA_TYPE *mediaType = nullptr;

        while (pEnum->Next(1, &mediaType, nullptr) == S_OK)
            if (mediaType->formattype == FORMAT_VideoInfo
                && mediaType->cbFormat >= sizeof(VIDEOINFOHEADER)
                && mediaType->pbFormat != nullptr
                && (rawFmtToAkFmt->contains(mediaType->subtype)
                    || compressedFormatToStr->contains(mediaType->subtype))) {
                mediaTypes << MediaTypePtr(mediaType, this->deleteMediaType);
            } else {
                this->deleteMediaType(mediaType);
            }

        pEnum->Release();
    }

    return mediaTypes;
}

bool CaptureDShowPrivate::isPinConnected(IPin *pPin, bool *ok) const
{
    IPin *pTmp = nullptr;
    HRESULT hr = pPin->ConnectedTo(&pTmp);

    if (ok)
        *ok = true;

    if (hr == VFW_E_NOT_CONNECTED)
        return false;

    if (FAILED(hr)) {
        if (ok)
            *ok = false;

        return false;
    }

    if (!pTmp)
        return false;

    pTmp->Release();

    return true;
}

PinPtr CaptureDShowPrivate::findUnconnectedPin(IBaseFilter *pFilter,
                                               PIN_DIRECTION PinDir) const
{
    IEnumPins *pEnum = nullptr;

    if (FAILED(pFilter->EnumPins(&pEnum)))
        return {};

    pEnum->Reset();
    PinPtr matchedPin;
    IPin *pPin = nullptr;

    while (pEnum->Next(1, &pPin, nullptr) == S_OK) {
        PIN_DIRECTION pinDir;

        if (FAILED(pPin->QueryDirection(&pinDir))
            || pinDir != PinDir)
            continue;

        bool ok = false;
        bool connected = this->isPinConnected(pPin, &ok);

        if (!ok || connected)
            continue;

        matchedPin = PinPtr(pPin, [] (IPin *pin) {
            pin->Release();
        });
        pPin->AddRef();

        break;
    }

    pEnum->Release();

    return matchedPin;
}

bool CaptureDShowPrivate::connectFilters(IGraphBuilder *pGraph,
                                         IBaseFilter *pSrc,
                                         IBaseFilter *pDest) const
{
    // Find source pin.
    PinPtr srcPin = this->findUnconnectedPin(pSrc, PINDIR_OUTPUT);

    if (!srcPin)
        return false;

    // Find dest pin.
    PinPtr dstPin = this->findUnconnectedPin(pDest, PINDIR_INPUT);

    if (!dstPin)
        return false;

    if (FAILED(pGraph->Connect(srcPin.data(), dstPin.data())))
        return false;

    return true;
}

PinList CaptureDShowPrivate::enumPins(IBaseFilter *filter,
                                      PIN_DIRECTION direction) const
{
    if (!filter)
        return PinList();

    PinList pinList;
    IEnumPins *enumPins = nullptr;

    if (SUCCEEDED(filter->EnumPins(&enumPins))) {
        enumPins->Reset();
        IPin *pin = nullptr;

        while (enumPins->Next(1, &pin, nullptr) == S_OK) {
            PIN_DIRECTION pinDir;

            if (SUCCEEDED(pin->QueryDirection(&pinDir))
                && pinDir == direction) {
                pinList << PinPtr(pin, [] (IPin *pin) {
                    pin->Release();
                });

                continue;
            }

            pin->Release();
        }

        enumPins->Release();
    }

    return pinList;
}

void CaptureDShowPrivate::freeMediaType(AM_MEDIA_TYPE &mediaType)
{
    if (mediaType.cbFormat) {
        CoTaskMemFree(PVOID(mediaType.pbFormat));
        mediaType.cbFormat = 0;
        mediaType.pbFormat = nullptr;
    }

    if (mediaType.pUnk) {
        // pUnk should not be used.
        mediaType.pUnk->Release();
        mediaType.pUnk = nullptr;
    }
}

void CaptureDShowPrivate::deleteMediaType(AM_MEDIA_TYPE *mediaType)
{
    if (!mediaType)
        return;

    CaptureDShowPrivate::freeMediaType(*mediaType);
    CoTaskMemFree(mediaType);
}

QVariantList CaptureDShowPrivate::imageControls(IBaseFilter *filter) const
{
    if (!filter)
        return QVariantList();

    qint32 min = 0;
    qint32 max = 0;
    qint32 step = 0;
    qint32 defaultValue = 0;
    qint32 value = 0;
    qint32 flags = 0;

    QVariantList controls;
    IAMVideoProcAmp *pProcAmp = nullptr;

    if (SUCCEEDED(filter->QueryInterface(IID_IAMVideoProcAmp,
                                         reinterpret_cast<void **>(&pProcAmp)))) {
        for (auto it = vpapToStr->begin(); it != vpapToStr->end(); it++) {
            if (SUCCEEDED(pProcAmp->GetRange(it.key(),
                                             reinterpret_cast<LONG *>(&min),
                                             reinterpret_cast<LONG *>(&max),
                                             reinterpret_cast<LONG *>(&step),
                                             reinterpret_cast<LONG *>(&defaultValue),
                                             reinterpret_cast<LONG *>(&flags)))) {
                bool autoSupport = flags & VideoProcAmp_Flags_Auto;
                bool manualSupport = flags & VideoProcAmp_Flags_Manual;

                if (SUCCEEDED(pProcAmp->Get(it.key(),
                                            reinterpret_cast<LONG *>(&value),
                                            reinterpret_cast<LONG *>(&flags)))) {
                    if (autoSupport) {
                        QVariantList control {
                            it.value() + " (Auto)",
                            QString("boolean"),
                            0,
                            1,
                            1,
                            1,
                            flags & VideoProcAmp_Flags_Auto,
                            QStringList()
                        };

                        controls << QVariant(control);
                    }

                    if (manualSupport) {
                        QString type;

                        if (min == 0 && max == 1)
                            type = "boolean";
                        else
                            type = "integer";

                        QVariantList control {
                            it.value(),
                            type,
                            min,
                            max,
                            step,
                            defaultValue,
                            value,
                            QStringList()
                        };

                        controls << QVariant(control);
                    }
                }
            }
        }

        pProcAmp->Release();
    }

    return controls;
}

bool CaptureDShowPrivate::setImageControls(IBaseFilter *filter,
                                           const QVariantMap &imageControls) const
{
    if (!filter)
        return false;

    IAMVideoProcAmp *pProcAmp = nullptr;

    if (SUCCEEDED(filter->QueryInterface(IID_IAMVideoProcAmp,
                                         reinterpret_cast<void **>(&pProcAmp)))) {
        for (auto it = vpapToStr->begin(); it != vpapToStr->end(); it++) {
            auto key = it.value();

            if (imageControls.contains(key)) {
                LONG value = 0;
                LONG flags = 0;
                pProcAmp->Get(it.key(),
                              reinterpret_cast<LONG *>(&value),
                              reinterpret_cast<LONG *>(&flags));
                value = imageControls[key].toInt();
                pProcAmp->Set(it.key(), value, flags);
            }

            if (imageControls.contains(key + " (Auto)")) {
                LONG value = 0;
                LONG flags = 0;
                pProcAmp->Get(it.key(),
                              reinterpret_cast<LONG *>(&value),
                              reinterpret_cast<LONG *>(&flags));

                if (imageControls[key + " (Auto)"].toBool())
                    flags |= VideoProcAmp_Flags_Auto;
                else
                    flags &= ~VideoProcAmp_Flags_Auto;

                pProcAmp->Set(it.key(), value, flags);
            }
        }

        pProcAmp->Release();
    }

    return true;
}

QVariantList CaptureDShowPrivate::cameraControls(IBaseFilter *filter) const
{
    if (!filter)
        return QVariantList();

    qint32 min = 0;
    qint32 max = 0;
    qint32 step = 0;
    qint32 defaultValue = 0;
    qint32 value = 0;
    qint32 flags = 0;

    QVariantList controls;
    IAMCameraControl *pCameraControl = nullptr;

    if (SUCCEEDED(filter->QueryInterface(IID_IAMCameraControl,
                                         reinterpret_cast<void **>(&pCameraControl)))) {
        for (auto it = ccToStr->begin(); it != ccToStr->end(); it++) {
            if (SUCCEEDED(pCameraControl->GetRange(it.key(),
                                                   reinterpret_cast<LONG *>(&min),
                                                   reinterpret_cast<LONG *>(&max),
                                                   reinterpret_cast<LONG *>(&step),
                                                   reinterpret_cast<LONG *>(&defaultValue),
                                                   reinterpret_cast<LONG *>(&flags)))) {
                bool autoSupport = flags & CameraControl_Flags_Auto;
                bool manualSupport = flags & CameraControl_Flags_Manual;

                if (SUCCEEDED(pCameraControl->Get(it.key(),
                                                  reinterpret_cast<LONG *>(&value),
                                                  reinterpret_cast<LONG *>(&flags)))) {
                    if (autoSupport) {
                        QVariantList control {
                            it.value() + " (Auto)",
                            QString("boolean"),
                            0,
                            1,
                            1,
                            1,
                            flags & CameraControl_Flags_Auto,
                            QStringList()
                        };

                        controls << QVariant(control);
                    }

                    if (manualSupport) {
                        QString type;

                        if (min == 0 && max == 1)
                            type = "boolean";
                        else
                            type = "integer";

                        QVariantList control {
                            it.value(),
                            type,
                            min,
                            max,
                            step,
                            defaultValue,
                            value,
                            QStringList()
                        };

                        controls << QVariant(control);
                    }
                }
            }
        }

        pCameraControl->Release();
    }

    return controls;
}

bool CaptureDShowPrivate::setCameraControls(IBaseFilter *filter,
                                            const QVariantMap &cameraControls) const
{
    if (!filter)
        return false;

    IAMCameraControl *pCameraControl = nullptr;

    if (SUCCEEDED(filter->QueryInterface(IID_IAMCameraControl,
                                         reinterpret_cast<void **>(&pCameraControl)))) {
        for (auto it = ccToStr->begin(); it != ccToStr->end(); it++) {
            auto key = it.value();

            if (cameraControls.contains(key)) {
                LONG value = 0;
                LONG flags = 0;
                pCameraControl->Get(it.key(),
                                    reinterpret_cast<LONG *>(&value),
                                    reinterpret_cast<LONG *>(&flags));
                value = cameraControls[key].toInt();
                pCameraControl->Set(it.key(), value, flags);
            }

            if (cameraControls.contains(key + " (Auto)")) {
                LONG value = 0;
                LONG flags = 0;
                pCameraControl->Get(it.key(),
                                    reinterpret_cast<LONG *>(&value),
                                    reinterpret_cast<LONG *>(&flags));

                if (cameraControls[key + " (Auto)"].toBool())
                    flags |= CameraControl_Flags_Auto;
                else
                    flags &= ~CameraControl_Flags_Auto;

                pCameraControl->Set(it.key(), value, flags);
            }
        }

        pCameraControl->Release();
    }

    return true;
}

QVariantMap CaptureDShowPrivate::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    for (auto &control: controls) {
        auto params = control.toList();
        auto controlName = params[0].toString();
        controlStatus[controlName] = params[6];
    }

    return controlStatus;
}

QVariantMap CaptureDShowPrivate::mapDiff(const QVariantMap &map1,
                                         const QVariantMap &map2) const
{
    QVariantMap map;

    for (auto &control: map2.keys())
        if (!map1.contains(control)
            || map1[control] != map2[control]) {
            map[control] = map2[control];
        }

    return map;
}

void CaptureDShowPrivate::frameReceived(qreal time, const QByteArray &buffer)
{
    Q_UNUSED(time)

    this->m_mutex.lockForWrite();
    this->m_curBuffer = buffer;
    this->m_waitCondition.wakeAll();
    this->m_mutex.unlock();
}

void CaptureDShowPrivate::sampleReceived(qreal time, IMediaSample *sample)
{
    Q_UNUSED(time)

    this->m_mutex.lockForWrite();
    BYTE *data = nullptr;

    if (SUCCEEDED(sample->GetPointer(&data))) {
        this->m_curBuffer = QByteArray(reinterpret_cast<char *>(data),
                                       int(sample->GetSize()));

        if (this->m_curMediaType) {
            this->freeMediaType(*this->m_curMediaType);
            this->m_curMediaType = nullptr;
        }

        AM_MEDIA_TYPE *mediaType = nullptr;

        if (SUCCEEDED(sample->GetMediaType(&mediaType)))
            this->m_curMediaType = mediaType;
    }

    this->m_waitCondition.wakeAll();
    this->m_mutex.unlock();

    sample->Release();
}

AkPacket CaptureDShowPrivate::processFrame(const AM_MEDIA_TYPE *mediaType,
                                           const QByteArray &buffer) const
{
    bool isRaw = false;
    size_t srcLineSize = 0;
    bool mirror = false;
    auto caps = this->capsFromMediaType(mediaType,
                                        &isRaw,
                                        &srcLineSize,
                                        &mirror);
    auto timestamp = QDateTime::currentMSecsSinceEpoch();
    auto pts =
            qint64(qreal(timestamp)
                   * this->m_timeBase.invert().value()
                   / 1e3);

    if (isRaw) {
        AkVideoPacket packet(caps);
        auto iData = buffer.constData();

        for (int plane = 0; plane < packet.planes(); ++plane) {
            auto iLineSize = packet.planes() > 1?
                        srcLineSize >> packet.widthDiv(plane):
                        srcLineSize;
            auto oLineSize = packet.lineSize(plane);
            auto lineSize = qMin<size_t>(iLineSize, oLineSize);
            auto heightDiv = packet.heightDiv(plane);

            if (mirror) {
                for (int y = 0; y < packet.caps().height(); ++y) {
                    int ys = y >> heightDiv;
                    memcpy(packet.line(plane, packet.caps().height() - y - 1),
                           iData + ys * iLineSize,
                           lineSize);
                }
            } else {
                for (int y = 0; y < packet.caps().height(); ++y) {
                    int ys = y >> heightDiv;
                    memcpy(packet.line(plane, y),
                           iData + ys * iLineSize,
                           lineSize);
                }
            }

            iData += (iLineSize * packet.caps().height()) >> heightDiv;
        }

        packet.setPts(pts);
        packet.setTimeBase(this->m_timeBase);
        packet.setIndex(0);
        packet.setId(this->m_id);

        return packet;
    }

    AkCompressedVideoPacket packet(caps, buffer.size());
    memcpy(packet.data(), buffer.constData(), buffer.size());
    packet.setPts(pts);
    packet.setTimeBase(this->m_timeBase);
    packet.setIndex(0);
    packet.setId(this->m_id);

    return packet;
}

void CaptureDShowPrivate::updateDevices()
{
    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesCaps) devicesCaps;

    IEnumMoniker *pEnum = nullptr;
    HRESULT hr = this->enumerateCameras(&pEnum);

    if (SUCCEEDED(hr)) {
        pEnum->Reset();
        IMoniker *moniker = nullptr;

        for (int i = 0; pEnum->Next(1, &moniker, nullptr) == S_OK; i++) {
            IPropertyBag *propertyBag = nullptr;
            HRESULT hr = moniker->BindToStorage(nullptr,
                                                nullptr,
                                                IID_IPropertyBag,
                                                reinterpret_cast<void **>(&propertyBag));

            if (FAILED(hr)) {
                moniker->Release();

                continue;
            }

            auto devicePath = this->devicePath(propertyBag);

            if (devicePath.isEmpty())
                devicePath = this->monikerDisplayName(moniker);

            auto description = this->deviceDescription(propertyBag);
            propertyBag->Release();

            IBaseFilter *baseFilter = nullptr;
            hr = moniker->BindToObject(nullptr,
                                       nullptr,
                                       IID_IBaseFilter,
                                       reinterpret_cast<void **>(&baseFilter));

            if (FAILED(hr)) {
                moniker->Release();

                continue;
            }

            auto caps = this->caps(baseFilter);
            baseFilter->Release();

            if (!caps.isEmpty()) {
                devices << devicePath;
                descriptions[devicePath] = description;
                devicesCaps[devicePath] = caps;
            }

            moniker->Release();
        }

        pEnum->Release();
    }

    if (devicesCaps.isEmpty()) {
        devices.clear();
        descriptions.clear();
    }

    this->m_descriptions = descriptions;
    this->m_devicesCaps = devicesCaps;

    if (this->m_devices != devices) {
        this->m_devices = devices;
        emit self->webcamsChanged(this->m_devices);
    }
}

bool CaptureDShow::init()
{
    this->d->m_localImageControls.clear();
    this->d->m_localCameraControls.clear();

    qDebug() << "Creating FilterGraph";

    if (FAILED(CoCreateInstance(CLSID_FilterGraph,
                                nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_IGraphBuilder,
                                reinterpret_cast<void **>(&this->d->m_graph)))) {
        qCritical() << "Error creating FilterGraph instance.";

        return false;
    }

    qDebug() << "Creating camera filter";
    this->d->m_webcamFilter = this->d->findFilter(this->d->m_device);

    if (!this->d->m_webcamFilter) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        qCritical() << "Error creating camera filter.";

        return false;
    }

    qDebug() << "Adding camera filter to the graph";

    if (FAILED(this->d->m_graph->AddFilter(this->d->m_webcamFilter.data(),
                                           SOURCE_FILTER_NAME))) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();
        qCritical() << "Error adding camera filter to the graph.";

        return false;
    }

    qDebug() << "Creating SampleGrabber instance.";
    IBaseFilter *grabberFilter = nullptr;

    if (FAILED(CoCreateInstance(CLSID_SampleGrabber,
                                nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_IBaseFilter,
                                reinterpret_cast<void **>(&grabberFilter)))) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();
        qCritical() << "Error creating SampleGrabber instance.";

        return false;
    }

    qDebug() << "Adding sample grabber to the graph.";

    if (FAILED(this->d->m_graph->AddFilter(grabberFilter, L"Grabber"))) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();
        qCritical() << "Error adding sample grabber to the graph.";

        return false;
    }

    qDebug() << "Querying SampleGrabber interface.";
    ISampleGrabber *grabberPtr = nullptr;

    if (FAILED(grabberFilter->QueryInterface(IID_ISampleGrabber,
                                             reinterpret_cast<void **>(&grabberPtr)))) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();
        qCritical() << "Error querying SampleGrabber interface.";

        return false;
    }

    qDebug() << "Setting sample grabber to one shot.";

    if (FAILED(grabberPtr->SetOneShot(FALSE))) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();
        qCritical() << "Error setting sample grabber to one shot.";

        return false;
    }

    qDebug() << "Setting sample grabber to sampling mode.";
    HRESULT hr = grabberPtr->SetBufferSamples(TRUE);

    if (FAILED(hr)) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();
        qCritical() << "Error setting sample grabber to sampling mode.";

        return false;
    }

    if (this->d->m_ioMethod != IoMethodDirectRead) {
        int type = this->d->m_ioMethod == IoMethodGrabSample? 0: 1;
        hr = grabberPtr->SetCallback(&this->d->m_frameGrabber, type);
    }

    this->d->m_grabber =
            SampleGrabberPtr(grabberPtr, [] (ISampleGrabber *sampleGrabber) {
        sampleGrabber->Release();
    });

    qDebug() << "Connecting filters.";

    if (!this->d->connectFilters(this->d->m_graph,
                                 this->d->m_webcamFilter.data(),
                                 grabberFilter)) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();
        qCritical() << "Error connecting filters.";

        return false;
    }

    qDebug() << "Creating NullRenderer instance.";
    IBaseFilter *nullFilter = nullptr;

    if (FAILED(CoCreateInstance(CLSID_NullRenderer,
                                nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_IBaseFilter,
                                reinterpret_cast<void **>(&nullFilter)))) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();
        qCritical() << "Error creating NullRenderer instance.";

        return false;
    }

    qDebug() << "Adding null filter to the graph.";

    if (FAILED(this->d->m_graph->AddFilter(nullFilter, L"NullFilter"))) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();
        qCritical() << "Error adding null filter to the graph.";

        return false;
    }

    qDebug() << "Connecting null filter.";

    if (!this->d->connectFilters(this->d->m_graph,
                                 grabberFilter,
                                 nullFilter)) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();
        qCritical() << "Error connecting null filter.";

        return false;
    }

    qDebug() << "Reading camera streams.";
    auto streams = this->streams();

    if (streams.isEmpty()) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();
        qCritical() << "Camera streams are empty.";

        return false;
    }

    qDebug() << "Reading media types.";
    auto mediaTypes = this->d->listMediaTypes(this->d->m_webcamFilter.data());

    if (mediaTypes.isEmpty()) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();
        qCritical() << "Can't get camera media types.";

        return false;
    }

    qDebug() << "Setting grabber media type.";
    MediaTypePtr mediaType = streams[0] < mediaTypes.size()?
                                mediaTypes[streams[0]]:
                                mediaTypes.first();

    if (FAILED(grabberPtr->SetMediaType(mediaType.data()))) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();
        qCritical() << "Error setting grabber media type.";

        return false;
    }

    qDebug() << "Setting the media type for the camera filter pins.";
    auto pins = this->d->enumPins(this->d->m_webcamFilter.data(),
                                  PINDIR_OUTPUT);

    for (const PinPtr &pin: pins) {
        IAMStreamConfig *pStreamConfig = nullptr;
        auto hr =
                pin->QueryInterface(IID_IAMStreamConfig,
                                    reinterpret_cast<void **>(&pStreamConfig));

        if (SUCCEEDED(hr))
            pStreamConfig->SetFormat(mediaType.data());

        if (pStreamConfig)
            pStreamConfig->Release();
    }

    qDebug() << "Querying MediaControl interface.";
    IMediaControl *control = nullptr;

    if (FAILED(this->d->m_graph->QueryInterface(IID_IMediaControl,
                                             reinterpret_cast<void **>(&control)))) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();
        qCritical() << "Error querying MediaControl interface.";

        return false;
    }

    this->d->m_id = Ak::id();
    auto caps = this->d->capsFromMediaType(mediaType);

    switch (caps.type()) {
    case AkCaps::CapsVideo: {
        AkVideoCaps videoCaps(caps);
        this->d->m_timeBase = videoCaps.fps().invert();

        break;
    }
    case AkCaps::CapsVideoCompressed: {
        AkCompressedVideoCaps videoCaps(caps);
        this->d->m_timeBase = videoCaps.fps().invert();

        break;
    }
    default:
        break;
    }

    if (this->d->m_curMediaType) {
        this->d->freeMediaType(*this->d->m_curMediaType);
        this->d->m_curMediaType = nullptr;
    }

    qDebug() << "Running the graph.";

    if (FAILED(control->Run())) {
        control->Release();
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();
        qCritical() << "Failed to run the graph.";

        return false;
    }

    control->Release();

    qDebug() << "Starting camera capture.";

    return true;
}

void CaptureDShow::uninit()
{
    IMediaControl *control = nullptr;

    if (SUCCEEDED(this->d->m_graph->QueryInterface(IID_IMediaControl,
                                                   reinterpret_cast<void **>(&control)))) {
        control->Stop();
        control->Release();
    }

    this->d->m_grabber.clear();
    this->d->m_graph->Release();
    this->d->m_graph = nullptr;
    this->d->m_webcamFilter.clear();
}

void CaptureDShow::setDevice(const QString &device)
{
    if (this->d->m_device == device)
        return;

    this->d->m_device = device;

    if (device.isEmpty()) {
        this->d->m_controlsMutex.lockForWrite();
        this->d->m_globalImageControls.clear();
        this->d->m_globalCameraControls.clear();
        this->d->m_controlsMutex.unlock();
    } else {
        this->d->m_controlsMutex.lockForWrite();
        auto camera = this->d->findFilterP(device);

        if (camera) {
            this->d->m_globalImageControls = this->d->imageControls(camera);
            this->d->m_globalCameraControls = this->d->cameraControls(camera);
            camera->Release();
        }

        this->d->m_controlsMutex.unlock();
    }

    this->d->m_controlsMutex.lockForRead();
    auto imageStatus = this->d->controlStatus(this->d->m_globalImageControls);
    auto cameraStatus = this->d->controlStatus(this->d->m_globalCameraControls);
    this->d->m_controlsMutex.unlock();

    emit this->deviceChanged(device);
    emit this->imageControlsChanged(imageStatus);
    emit this->cameraControlsChanged(cameraStatus);
}

void CaptureDShow::setStreams(const QList<int> &streams)
{
    if (streams.isEmpty())
        return;

    int stream = streams[0];

    if (stream < 0)
        return;

    auto supportedCaps = this->caps(this->d->m_device);

    if (stream >= supportedCaps.length())
        return;

    QList<int> inputStreams {stream};

    if (this->streams() == inputStreams)
        return;

    this->d->m_streams = inputStreams;
    emit this->streamsChanged(inputStreams);
}

void CaptureDShow::setIoMethod(const QString &ioMethod)
{
    IoMethod ioMethodEnum = ioMethodToStr->key(ioMethod, IoMethodGrabSample);

    if (this->d->m_ioMethod == ioMethodEnum)
        return;

    this->d->m_ioMethod = ioMethodEnum;
    emit this->ioMethodChanged(ioMethod);
}

void CaptureDShow::setNBuffers(int nBuffers)
{
    Q_UNUSED(nBuffers)
}

void CaptureDShow::resetDevice()
{
    this->setDevice("");
}

void CaptureDShow::resetStreams()
{
    auto supportedCaps = this->caps(this->d->m_device);
    QList<int> streams;

    if (!supportedCaps.isEmpty())
        streams << 0;

    this->setStreams(streams);
}

void CaptureDShow::resetIoMethod()
{
    this->setIoMethod("any");
}

void CaptureDShow::resetNBuffers()
{
}

void CaptureDShow::reset()
{
    this->resetStreams();
    this->resetImageControls();
    this->resetCameraControls();
}

#include "moc_capturedshow.cpp"
