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
#include <QSharedPointer>
#include <QMap>
#include <QSize>
#include <QDateTime>
#include <QVariant>
#include <QMutex>
#include <QWaitCondition>
#include <ak.h>
#include <akfrac.h>
#include <akcaps.h>
#include <akpacket.h>
#include <dshow.h>
#include <dbt.h>
#include <usbiodef.h>

#include "capturedshow.h"
#include "framegrabber.h"

#define TIME_BASE 1.0e7
#define SOURCE_FILTER_NAME L"Source"

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
    VideoProcAmpPropertyMap vpapToStr = {
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
    CameraControlMap ccToStr = {
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

using GuidToStrMap = QMap<GUID, QString>;

inline GuidToStrMap initGuidToStrMap()
{
    GuidToStrMap guidToStr = {
        {MEDIASUBTYPE_CLPL               , "CLPL"    },
        {MEDIASUBTYPE_YUYV               , "YUYV"    },
        {MEDIASUBTYPE_IYUV               , "IYUV"    },
        {MEDIASUBTYPE_YVU9               , "YVU9"    },
        {MEDIASUBTYPE_Y411               , "Y411"    },
        {MEDIASUBTYPE_Y41P               , "Y41P"    },
        {MEDIASUBTYPE_YUY2               , "YUY2"    },
        {MEDIASUBTYPE_YVYU               , "YVYU"    },
        {MEDIASUBTYPE_UYVY               , "UYVY"    },
        {MEDIASUBTYPE_Y211               , "Y211"    },
        {MEDIASUBTYPE_CLJR               , "CLJR"    },
        {MEDIASUBTYPE_IF09               , "IF09"    },
        {MEDIASUBTYPE_CPLA               , "CPLA"    },
        {MEDIASUBTYPE_MJPG               , "MJPG"    },
        {MEDIASUBTYPE_TVMJ               , "TVMJ"    },
        {MEDIASUBTYPE_WAKE               , "WAKE"    },
        {MEDIASUBTYPE_CFCC               , "CFCC"    },
        {MEDIASUBTYPE_IJPG               , "IJPG"    },
        {MEDIASUBTYPE_Plum               , "Plum"    },
        {MEDIASUBTYPE_DVCS               , "DVCS"    },
        {MEDIASUBTYPE_DVSD               , "DVSD"    },
        {MEDIASUBTYPE_MDVF               , "MDVF"    },
        {MEDIASUBTYPE_RGB1               , "RGB1"    },
        {MEDIASUBTYPE_RGB4               , "BGRX"    },
        {MEDIASUBTYPE_RGB8               , "RGB8"    },
        {MEDIASUBTYPE_RGB565             , "RGB565"  },
        {MEDIASUBTYPE_RGB555             , "RGB555"  },
        {MEDIASUBTYPE_RGB24              , "RGB"     },
        {MEDIASUBTYPE_RGB32              , "BGRX"    },
        {MEDIASUBTYPE_ARGB1555           , "ARGB555" },
        {MEDIASUBTYPE_ARGB4444           , "ARGB4444"},
        {MEDIASUBTYPE_ARGB32             , "ARGB"    },
        {MEDIASUBTYPE_AYUV               , "AYUV"    },
        {MEDIASUBTYPE_AI44               , "AI44"    },
        {MEDIASUBTYPE_IA44               , "IA44"    },
        {MEDIASUBTYPE_RGB32_D3D_DX7_RT   , "7R32"    },
        {MEDIASUBTYPE_RGB16_D3D_DX7_RT   , "7R16"    },
        {MEDIASUBTYPE_ARGB32_D3D_DX7_RT  , "7A88"    },
        {MEDIASUBTYPE_ARGB4444_D3D_DX7_RT, "7A44"    },
        {MEDIASUBTYPE_ARGB1555_D3D_DX7_RT, "7A15"    },
        {MEDIASUBTYPE_RGB32_D3D_DX9_RT   , "9R32"    },
        {MEDIASUBTYPE_RGB16_D3D_DX9_RT   , "9R16"    },
        {MEDIASUBTYPE_ARGB32_D3D_DX9_RT  , "9A88"    },
        {MEDIASUBTYPE_ARGB4444_D3D_DX9_RT, "9A44"    },
        {MEDIASUBTYPE_ARGB1555_D3D_DX9_RT, "9A15"    },
        {MEDIASUBTYPE_YV12               , "YV12"    },
        {MEDIASUBTYPE_NV12               , "NV12"    },
        {MEDIASUBTYPE_IMC1               , "IMC1"    },
        {MEDIASUBTYPE_IMC2               , "IMC2"    },
        {MEDIASUBTYPE_IMC3               , "IMC3"    },
        {MEDIASUBTYPE_IMC4               , "IMC4"    },
        {MEDIASUBTYPE_S340               , "S340"    },
        {MEDIASUBTYPE_S342               , "S342"    },
        {MEDIASUBTYPE_QTRpza             , "rpza"    },
        {MEDIASUBTYPE_QTSmc              , "smc "    },
        {MEDIASUBTYPE_QTRle              , "rle "    },
        {MEDIASUBTYPE_QTJpeg             , "jpeg"    },
        {MEDIASUBTYPE_dvsd               , "dvsd"    },
        {MEDIASUBTYPE_dvhd               , "dvhd"    },
        {MEDIASUBTYPE_dvsl               , "dvsl"    },
        {MEDIASUBTYPE_dv25               , "dv25"    },
        {MEDIASUBTYPE_dv50               , "dv50"    },
        {MEDIASUBTYPE_dvh1               , "dvh1"    },
    };

    return guidToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(GuidToStrMap, guidToStr, (initGuidToStrMap()))

using IoMethodMap = QMap<CaptureDShow::IoMethod, QString>;

inline IoMethodMap initIoMethodMap()
{
    IoMethodMap ioMethodToStr = {
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
        QMap<QString, QVariantList> m_devicesCaps;
        qint64 m_id {-1};
        AkFrac m_timeBase;
        CaptureDShow::IoMethod m_ioMethod {CaptureDShow::IoMethodGrabSample};
        QMap<QString, QSize> m_resolution;
        BaseFilterPtr m_webcamFilter;
        IGraphBuilder *m_graph {nullptr};
        SampleGrabberPtr m_grabber;
        FrameGrabber m_frameGrabber;
        QByteArray m_curBuffer;
        QMutex m_mutex;
        QMutex m_controlsMutex;
        QWaitCondition m_waitCondition;
        QVariantList m_globalImageControls;
        QVariantList m_globalCameraControls;
        QVariantMap m_localImageControls;
        QVariantMap m_localCameraControls;

        explicit CaptureDShowPrivate(CaptureDShow *self);
        QString devicePath(IPropertyBag *propertyBag) const;
        QString deviceDescription(IPropertyBag *propertyBag) const;
        QVariantList caps(IBaseFilter *baseFilter) const;
        AkCaps capsFromMediaType(const AM_MEDIA_TYPE *mediaType) const;
        AkCaps capsFromMediaType(const MediaTypePtr &mediaType) const;
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
    qApp->installNativeEventFilter(this);
    this->d->updateDevices();
}

CaptureDShow::~CaptureDShow()
{
    qApp->removeNativeEventFilter(this);
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

QList<int> CaptureDShow::listTracks(const QString &mimeType)
{
    if (mimeType != "video/x-raw"
        && !mimeType.isEmpty())
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

QVariantList CaptureDShow::caps(const QString &webcam) const
{
    return this->d->m_devicesCaps.value(webcam);
}

QString CaptureDShow::capsDescription(const AkCaps &caps) const
{
    if (caps.mimeType() != "video/unknown")
        return {};

    AkFrac fps = caps.property("fps").toString();

    return QString("%1, %2x%3, %4 FPS")
                .arg(caps.property("fourcc").toString())
                .arg(caps.property("width").toString())
                .arg(caps.property("height").toString())
                .arg(qRound(fps.value()));
}

QVariantList CaptureDShow::imageControls() const
{
    return this->d->m_globalImageControls;
}

bool CaptureDShow::setImageControls(const QVariantMap &imageControls)
{
    this->d->m_controlsMutex.lock();
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

    this->d->m_controlsMutex.lock();

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
    this->d->m_controlsMutex.lock();
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

    this->d->m_controlsMutex.lock();

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
        this->d->m_controlsMutex.lock();
        auto imageControls = this->d->controlStatus(this->d->m_globalImageControls);
        this->d->m_controlsMutex.unlock();

        if (this->d->m_localImageControls != imageControls) {
            auto controls = this->d->mapDiff(this->d->m_localImageControls,
                                             imageControls);
            this->d->setImageControls(source, controls);
            this->d->m_localImageControls = imageControls;
        }

        this->d->m_controlsMutex.lock();
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

    AM_MEDIA_TYPE mediaType;
    ZeroMemory(&mediaType, sizeof(AM_MEDIA_TYPE));
    this->d->m_grabber->GetConnectedMediaType(&mediaType);
    AkCaps caps = this->d->capsFromMediaType(&mediaType);
    this->d->freeMediaType(mediaType);

    AkPacket packet;
    auto timestamp = QDateTime::currentMSecsSinceEpoch();
    auto pts =
            qint64(timestamp
                   * this->d->m_timeBase.invert().value()
                   / 1e3);

    if (this->d->m_ioMethod != IoMethodDirectRead) {
        this->d->m_mutex.lock();

        if (this->d->m_curBuffer.isEmpty())
            this->d->m_waitCondition.wait(&this->d->m_mutex, 1000);

        if (!this->d->m_curBuffer.isEmpty()) {
            int bufferSize = this->d->m_curBuffer.size();
            QByteArray oBuffer(bufferSize, 0);
            memcpy(oBuffer.data(),
                   this->d->m_curBuffer.constData(),
                   size_t(bufferSize));

            packet = AkPacket(caps);
            packet.setBuffer(oBuffer);
            packet.setPts(pts);
            packet.setTimeBase(this->d->m_timeBase);
            packet.setIndex(0);
            packet.setId(this->d->m_id);
            this->d->m_curBuffer.clear();
        }

        this->d->m_mutex.unlock();
    } else {
        long bufferSize;

        HRESULT hr = this->d->m_grabber->GetCurrentBuffer(&bufferSize, nullptr);

        if (FAILED(hr))
            return {};

        QByteArray oBuffer(bufferSize, 0);
        hr = this->d->m_grabber->GetCurrentBuffer(&bufferSize,
                                                  reinterpret_cast<long *>(oBuffer.data()));

        if (FAILED(hr))
            return {};

        packet = AkPacket(caps);
        packet.setBuffer(oBuffer);
        packet.setPts(pts);
        packet.setTimeBase(this->d->m_timeBase);
        packet.setIndex(0);
        packet.setId(this->d->m_id);
    }

    return packet;
}

bool CaptureDShow::nativeEventFilter(const QByteArray &eventType,
                                     void *message,
                                     long *result)
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

QVariantList CaptureDShowPrivate::caps(IBaseFilter *baseFilter) const
{
    auto pins = this->enumPins(baseFilter, PINDIR_OUTPUT);
    QVariantList caps;

    for (auto &pin: pins) {
        IEnumMediaTypes *pEnum = nullptr;

        if (FAILED(pin->EnumMediaTypes(&pEnum)))
            continue;

        pEnum->Reset();
        AM_MEDIA_TYPE *mediaType = nullptr;

        while (pEnum->Next(1, &mediaType, nullptr) == S_OK) {
            if (mediaType->formattype == FORMAT_VideoInfo
                && mediaType->cbFormat >= sizeof(VIDEOINFOHEADER)
                && mediaType->pbFormat != nullptr
                && guidToStr->contains(mediaType->subtype)) {
                auto videoCaps = this->capsFromMediaType(mediaType);

                if (videoCaps)
                    caps << QVariant::fromValue(videoCaps);
            }

            this->deleteMediaType(mediaType);
        }

        pEnum->Release();
    }

    return caps;
}

AkCaps CaptureDShowPrivate::capsFromMediaType(const AM_MEDIA_TYPE *mediaType) const
{
    if (!mediaType)
        return {};

    VIDEOINFOHEADER *videoInfoHeader =
            reinterpret_cast<VIDEOINFOHEADER *>(mediaType->pbFormat);
    QString fourcc = guidToStr->value(mediaType->subtype);

    if (fourcc.isEmpty())
        return {};

    AkCaps videoCaps;
    videoCaps.setMimeType("video/unknown");
    videoCaps.setProperty("fourcc", fourcc);
    videoCaps.setProperty("width", int(videoInfoHeader->bmiHeader.biWidth));
    videoCaps.setProperty("height", int(videoInfoHeader->bmiHeader.biHeight));
    AkFrac fps(TIME_BASE, videoInfoHeader->AvgTimePerFrame);
    videoCaps.setProperty("fps", fps.toString());

    return videoCaps;
}

AkCaps CaptureDShowPrivate::capsFromMediaType(const MediaTypePtr &mediaType) const
{
    return this->capsFromMediaType(mediaType.data());
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
    moniker->GetDisplayName(bind_ctx, nullptr, &olestr);
    bind_ctx->Release();

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
                && guidToStr->contains(mediaType->subtype)) {
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
                qint32 value = 0;
                qint32 flags = 0;
                pProcAmp->Get(it.key(),
                              reinterpret_cast<LONG *>(&value),
                              reinterpret_cast<LONG *>(&flags));
                value = imageControls[key].toInt();
                pProcAmp->Set(it.key(), value, flags);
            } else if (imageControls.contains(key + " (Auto)")) {
                qint32 value = 0;
                qint32 flags = 0;
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
                qint32 value = 0;
                qint32 flags = 0;
                pCameraControl->Get(it.key(),
                                    reinterpret_cast<LONG *>(&value),
                                    reinterpret_cast<LONG *>(&flags));
                value = cameraControls[key].toInt();
                pCameraControl->Set(it.key(), value, flags);
            } else if (cameraControls.contains(key + " (Auto)")) {
                qint32 value = 0;
                qint32 flags = 0;
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

    this->m_mutex.lock();
    this->m_curBuffer = buffer;
    this->m_waitCondition.wakeAll();
    this->m_mutex.unlock();
}

void CaptureDShowPrivate::updateDevices()
{
    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesCaps) devicesCaps;

    MonikersMap monikers;
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

    // Create the pipeline.
    if (FAILED(CoCreateInstance(CLSID_FilterGraph,
                                nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_IGraphBuilder,
                                reinterpret_cast<void **>(&this->d->m_graph))))
        return false;

    // Create the webcam filter.
    this->d->m_webcamFilter = this->d->findFilter(this->d->m_device);

    if (!this->d->m_webcamFilter) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;

        return false;
    }

    if (FAILED(this->d->m_graph->AddFilter(this->d->m_webcamFilter.data(),
                                           SOURCE_FILTER_NAME))) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();

        return false;
    }

    // Create the Sample Grabber filter.
    IBaseFilter *grabberFilter = nullptr;

    if (FAILED(CoCreateInstance(CLSID_SampleGrabber,
                                nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_IBaseFilter,
                                reinterpret_cast<void **>(&grabberFilter)))) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();

        return false;
    }

    if (FAILED(this->d->m_graph->AddFilter(grabberFilter, L"Grabber"))) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();

        return false;
    }

    ISampleGrabber *grabberPtr = nullptr;

    if (FAILED(grabberFilter->QueryInterface(IID_ISampleGrabber,
                                             reinterpret_cast<void **>(&grabberPtr)))) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();

        return false;
    }

    if (FAILED(grabberPtr->SetOneShot(FALSE))) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();

        return false;
    }

    HRESULT hr = grabberPtr->SetBufferSamples(TRUE);

    if (FAILED(hr)) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();

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

    if (!this->d->connectFilters(this->d->m_graph,
                                 this->d->m_webcamFilter.data(),
                                 grabberFilter)) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();

        return false;
    }

    // Create null filter.
    IBaseFilter *nullFilter = nullptr;

    if (FAILED(CoCreateInstance(CLSID_NullRenderer,
                                nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_IBaseFilter,
                                reinterpret_cast<void **>(&nullFilter)))) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();

        return false;
    }

    if (FAILED(this->d->m_graph->AddFilter(nullFilter, L"NullFilter"))) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();

        return false;
    }

    if (!this->d->connectFilters(this->d->m_graph,
                                 grabberFilter,
                                 nullFilter)) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();

        return false;
    }

    // Set capture format
    auto streams = this->streams();

    if (streams.isEmpty()) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();

        return false;
    }

    auto mediaTypes = this->d->listMediaTypes(this->d->m_webcamFilter.data());

    if (mediaTypes.isEmpty()) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();

        return false;
    }

    MediaTypePtr mediaType = streams[0] < mediaTypes.size()?
                                mediaTypes[streams[0]]:
                                mediaTypes.first();

    if (FAILED(grabberPtr->SetMediaType(mediaType.data()))) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();

        return false;
    }

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

    // Run the pipeline
    IMediaControl *control = nullptr;

    if (FAILED(this->d->m_graph->QueryInterface(IID_IMediaControl,
                                             reinterpret_cast<void **>(&control)))) {
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();

        return false;
    }

    this->d->m_id = Ak::id();
    AkCaps caps = this->d->capsFromMediaType(mediaType);
    this->d->m_timeBase = AkFrac(caps.property("fps").toString()).invert();

    if (FAILED(control->Run())) {
        control->Release();
        this->d->m_graph->Release();
        this->d->m_graph = nullptr;
        this->d->m_webcamFilter.clear();

        return false;
    }

    control->Release();

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
        this->d->m_controlsMutex.lock();
        this->d->m_globalImageControls.clear();
        this->d->m_globalCameraControls.clear();
        this->d->m_controlsMutex.unlock();
    } else {
        this->d->m_controlsMutex.lock();
        auto camera = this->d->findFilterP(device);

        if (camera) {
            this->d->m_globalImageControls = this->d->imageControls(camera);
            this->d->m_globalCameraControls = this->d->cameraControls(camera);
            camera->Release();
        }

        this->d->m_controlsMutex.unlock();
    }

    this->d->m_controlsMutex.lock();
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
    QVariantList supportedCaps = this->caps(this->d->m_device);
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
