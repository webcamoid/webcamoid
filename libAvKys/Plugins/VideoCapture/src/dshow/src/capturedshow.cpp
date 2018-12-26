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

typedef QMap<VideoProcAmpProperty, QString> VideoProcAmpPropertyMap;

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

typedef QMap<CameraControlProperty, QString> CameraControlMap;

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

typedef QMap<GUID, QString> GuidToStrMap;

inline GuidToStrMap initGuidToStrMap()
{
    GuidToStrMap guidToStr = {
        {MEDIASUBTYPE_CLPL               , "CLPL"},
        {MEDIASUBTYPE_YUYV               , "YUYV"},
        {MEDIASUBTYPE_IYUV               , "IYUV"},
        {MEDIASUBTYPE_YVU9               , "YVU9"},
        {MEDIASUBTYPE_Y411               , "Y411"},
        {MEDIASUBTYPE_Y41P               , "Y41P"},
        {MEDIASUBTYPE_YUY2               , "YUY2"},
        {MEDIASUBTYPE_YVYU               , "YVYU"},
        {MEDIASUBTYPE_UYVY               , "UYVY"},
        {MEDIASUBTYPE_Y211               , "Y211"},
        {MEDIASUBTYPE_CLJR               , "CLJR"},
        {MEDIASUBTYPE_IF09               , "IF09"},
        {MEDIASUBTYPE_CPLA               , "CPLA"},
        {MEDIASUBTYPE_MJPG               , "MJPG"},
        {MEDIASUBTYPE_TVMJ               , "TVMJ"},
        {MEDIASUBTYPE_WAKE               , "WAKE"},
        {MEDIASUBTYPE_CFCC               , "CFCC"},
        {MEDIASUBTYPE_IJPG               , "IJPG"},
        {MEDIASUBTYPE_Plum               , "Plum"},
        {MEDIASUBTYPE_DVCS               , "DVCS"},
        {MEDIASUBTYPE_DVSD               , "DVSD"},
        {MEDIASUBTYPE_MDVF               , "MDVF"},
        {MEDIASUBTYPE_RGB1               , "RGB1"},
        {MEDIASUBTYPE_RGB4               , "BGR0"},
        {MEDIASUBTYPE_RGB8               , "RGB8"},
        {MEDIASUBTYPE_RGB565             , "RGBP"},
        {MEDIASUBTYPE_RGB555             , "RGBO"},
        {MEDIASUBTYPE_RGB24              , "RGB3"},
        {MEDIASUBTYPE_RGB32              , "BGR0"},
        {MEDIASUBTYPE_ARGB1555           , "AR15"},
        {MEDIASUBTYPE_ARGB4444           , "AR12"},
        {MEDIASUBTYPE_ARGB32             , "BA24"},
        {MEDIASUBTYPE_AYUV               , "AYUV"},
        {MEDIASUBTYPE_AI44               , "AI44"},
        {MEDIASUBTYPE_IA44               , "IA44"},
        {MEDIASUBTYPE_RGB32_D3D_DX7_RT   , "7R32"},
        {MEDIASUBTYPE_RGB16_D3D_DX7_RT   , "7R16"},
        {MEDIASUBTYPE_ARGB32_D3D_DX7_RT  , "7A88"},
        {MEDIASUBTYPE_ARGB4444_D3D_DX7_RT, "7A44"},
        {MEDIASUBTYPE_ARGB1555_D3D_DX7_RT, "7A15"},
        {MEDIASUBTYPE_RGB32_D3D_DX9_RT   , "9R32"},
        {MEDIASUBTYPE_RGB16_D3D_DX9_RT   , "9R16"},
        {MEDIASUBTYPE_ARGB32_D3D_DX9_RT  , "9A88"},
        {MEDIASUBTYPE_ARGB4444_D3D_DX9_RT, "9A44"},
        {MEDIASUBTYPE_ARGB1555_D3D_DX9_RT, "9A15"},
        {MEDIASUBTYPE_YV12               , "YV12"},
        {MEDIASUBTYPE_NV12               , "NV12"},
        {MEDIASUBTYPE_IMC1               , "IMC1"},
        {MEDIASUBTYPE_IMC2               , "IMC2"},
        {MEDIASUBTYPE_IMC3               , "IMC3"},
        {MEDIASUBTYPE_IMC4               , "IMC4"},
        {MEDIASUBTYPE_S340               , "S340"},
        {MEDIASUBTYPE_S342               , "S342"},
        {MEDIASUBTYPE_QTRpza             , "rpza"},
        {MEDIASUBTYPE_QTSmc              , "smc "},
        {MEDIASUBTYPE_QTRle              , "rle "},
        {MEDIASUBTYPE_QTJpeg             , "jpeg"},
        {MEDIASUBTYPE_dvsd               , "dvsd"},
        {MEDIASUBTYPE_dvhd               , "dvhd"},
        {MEDIASUBTYPE_dvsl               , "dvsl"},
        {MEDIASUBTYPE_dv25               , "dv25"},
        {MEDIASUBTYPE_dv50               , "dv50"},
        {MEDIASUBTYPE_dvh1               , "dvh1"}
    };

    return guidToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(GuidToStrMap, guidToStr, (initGuidToStrMap()))

typedef QMap<CaptureDShow::IoMethod, QString> IoMethodMap;

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

typedef QSharedPointer<IGraphBuilder> GraphBuilderPtr;
typedef QSharedPointer<IBaseFilter> BaseFilterPtr;
typedef QSharedPointer<ISampleGrabber> SampleGrabberPtr;
typedef QSharedPointer<IAMStreamConfig> StreamConfigPtr;
typedef QSharedPointer<FrameGrabber> FrameGrabberPtr;
typedef QSharedPointer<IMoniker> MonikerPtr;
typedef QMap<QString, MonikerPtr> MonikersMap;
typedef QSharedPointer<AM_MEDIA_TYPE> MediaTypePtr;
typedef QList<MediaTypePtr> MediaTypesList;
typedef QSharedPointer<IPin> PinPtr;
typedef QList<PinPtr> PinList;

class CaptureDShowPrivate
{
    public:
        QStringList m_webcams;
        QString m_device;
        QList<int> m_streams;
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

        AkCaps capsFromMediaType(const AM_MEDIA_TYPE *mediaType) const;
        AkCaps capsFromMediaType(const MediaTypePtr &mediaType) const;
        HRESULT enumerateCameras(IEnumMoniker **ppEnum) const;
        MonikersMap listMonikers() const;
        MonikerPtr findMoniker(const QString &webcam) const;
        IBaseFilter *findFilterP(const QString &webcam) const;
        BaseFilterPtr findFilter(const QString &webcam) const;
        MediaTypesList listMediaTypes(const QString &webcam) const;
        MediaTypesList listMediaTypes(IBaseFilter *filter) const;
        bool isPinConnected(IPin *pPin, bool *ok=nullptr) const;
        PinPtr findUnconnectedPin(IBaseFilter *pFilter,
                                  PIN_DIRECTION PinDir) const;
        bool connectFilters(IGraphBuilder *pGraph,
                            IBaseFilter *pSrc,
                            IBaseFilter *pDest) const;
        PinList enumPins(IBaseFilter *filter,
                         PIN_DIRECTION direction) const;
        static void deleteUnknown(IUnknown *unknown);
        static void freeMediaType(AM_MEDIA_TYPE &mediaType);
        static void deleteMediaType(AM_MEDIA_TYPE *mediaType);
        static void deletePin(IPin *pin);
        QVariantList imageControls(IBaseFilter *filter) const;
        bool setImageControls(IBaseFilter *filter,
                              const QVariantMap &imageControls) const;
        QVariantList cameraControls(IBaseFilter *filter) const;
        bool setCameraControls(IBaseFilter *filter,
                               const QVariantMap &cameraControls) const;
        QVariantMap controlStatus(const QVariantList &controls) const;
        QVariantMap mapDiff(const QVariantMap &map1,
                            const QVariantMap &map2) const;
};

CaptureDShow::CaptureDShow(QObject *parent):
    Capture(parent),
    QAbstractNativeEventFilter()
{
    this->d = new CaptureDShowPrivate;

    QObject::connect(&this->d->m_frameGrabber,
                     &FrameGrabber::frameReady,
                     this,
                     &CaptureDShow::frameReceived,
                     Qt::DirectConnection);

    qApp->installNativeEventFilter(this);
}

CaptureDShow::~CaptureDShow()
{
    qApp->removeNativeEventFilter(this);
    delete this->d;
}

QStringList CaptureDShow::webcams() const
{
    return this->d->listMonikers().keys();
}

QString CaptureDShow::device() const
{
    return this->d->m_device;
}

QList<int> CaptureDShow::streams()
{
    if (!this->d->m_streams.isEmpty())
        return this->d->m_streams;

    QVariantList caps = this->caps(this->d->m_device);

    if (caps.isEmpty())
        return QList<int>();

    return QList<int>() << 0;
}

QList<int> CaptureDShow::listTracks(const QString &mimeType)
{
    if (mimeType != "video/x-raw"
        && !mimeType.isEmpty())
        return QList<int>();

    QVariantList caps = this->caps(this->d->m_device);
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
    if (webcam.isEmpty())
        return QString();

    MonikerPtr moniker = this->d->findMoniker(webcam);

    if (!moniker)
        return QString();

    IPropertyBag *pPropBag = nullptr;
    HRESULT hr = moniker->BindToStorage(nullptr,
                                        nullptr,
                                        IID_IPropertyBag,
                                        reinterpret_cast<void **>(&pPropBag));

    if (FAILED(hr))
        return QString();

    VARIANT var;
    VariantInit(&var);

    // Get description or friendly name.
    hr = pPropBag->Read(L"Description", &var, nullptr);

    if (FAILED(hr))
        hr = pPropBag->Read(L"FriendlyName", &var, nullptr);

    QString description;

    if (SUCCEEDED(hr))
        description = QString::fromWCharArray(var.bstrVal);

    pPropBag->Release();

    return description;
}

QVariantList CaptureDShow::caps(const QString &webcam) const
{
    QVariantList caps;
    MediaTypesList mediaTypes = this->d->listMediaTypes(webcam);

    for (const MediaTypePtr &mediaType: mediaTypes) {
        AkCaps videoCaps = this->d->capsFromMediaType(mediaType);

        if (!videoCaps)
            continue;

        caps << QVariant::fromValue(videoCaps);
    }

    return caps;
}

QString CaptureDShow::capsDescription(const AkCaps &caps) const
{
    if (caps.mimeType() != "video/unknown")
        return QString();

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
        QVariantList control = globalImageControls[i].toList();
        QString controlName = control[0].toString();

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

    for (const QVariant &control: this->imageControls()) {
        QVariantList params = control.toList();
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
        QVariantList control = globalCameraControls[i].toList();
        QString controlName = control[0].toString();

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

    for (const QVariant &control: this->cameraControls()) {
        QVariantList params = control.toList();

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

            packet = AkPacket(caps, oBuffer);
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
            return AkPacket();

        QByteArray oBuffer(bufferSize, 0);
        hr = this->d->m_grabber->GetCurrentBuffer(&bufferSize,
                                                  reinterpret_cast<long *>(oBuffer.data()));

        if (FAILED(hr))
            return AkPacket();

        packet = AkPacket(caps, oBuffer);
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
    Q_UNUSED(eventType);

    if (!message)
        return false;

    auto msg = reinterpret_cast<MSG *>(message);

    if (msg->message == WM_DEVICECHANGE) {
        switch (msg->wParam) {
        case DBT_DEVICEARRIVAL:
        case DBT_DEVICEREMOVECOMPLETE:
        case DBT_DEVNODES_CHANGED: {
            auto webcams = this->webcams();

            if (webcams != this->d->m_webcams) {
                emit this->webcamsChanged(webcams);

                this->d->m_webcams = webcams;
            }

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

AkCaps CaptureDShowPrivate::capsFromMediaType(const AM_MEDIA_TYPE *mediaType) const
{
    if (!mediaType)
        return AkCaps();

    VIDEOINFOHEADER *videoInfoHeader =
            reinterpret_cast<VIDEOINFOHEADER *>(mediaType->pbFormat);
    QString fourcc = guidToStr->value(mediaType->subtype);

    if (fourcc.isEmpty())
        return AkCaps();

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

            VARIANT var;
            VariantInit(&var);
            hr = pPropBag->Read(L"DevicePath", &var, nullptr);

            QString devicePath;

            if (SUCCEEDED(hr))
                devicePath = QString::fromWCharArray(var.bstrVal);
            else
                devicePath = QString("/dev/video%1").arg(i);

            monikers[devicePath] = MonikerPtr(pMoniker, this->deleteUnknown);

            VariantClear(&var);
            pPropBag->Release();
        }

        pEnum->Release();
    }

    return monikers;
}

MonikerPtr CaptureDShowPrivate::findMoniker(const QString &webcam) const
{
    MonikersMap monikers = this->listMonikers();

    if (monikers.contains(webcam))
        return monikers[webcam];
    else
        return MonikerPtr();
}

IBaseFilter *CaptureDShowPrivate::findFilterP(const QString &webcam) const
{
    MonikerPtr moniker = this->findMoniker(webcam);

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
    IBaseFilter *filter = this->findFilterP(webcam);

    if (!filter)
        return BaseFilterPtr();

    return BaseFilterPtr(filter, this->deleteUnknown);
}

MediaTypesList CaptureDShowPrivate::listMediaTypes(const QString &webcam) const
{
    BaseFilterPtr filter = this->findFilter(webcam);

    return this->listMediaTypes(filter.data());
}

MediaTypesList CaptureDShowPrivate::listMediaTypes(IBaseFilter *filter) const
{
    PinList pins = this->enumPins(filter, PINDIR_OUTPUT);
    MediaTypesList mediaTypes;

    for (const PinPtr &pin: pins) {
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
        return PinPtr();

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

        matchedPin = PinPtr(pPin, this->deletePin);
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
                pinList << PinPtr(pin, this->deleteUnknown);

                continue;
            }

            pin->Release();
        }

        enumPins->Release();
    }

    return pinList;
}

void CaptureDShowPrivate::deleteUnknown(IUnknown *unknown)
{
    unknown->Release();
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

void CaptureDShowPrivate::deletePin(IPin *pin)
{
    pin->Release();
}

QVariantList CaptureDShowPrivate::imageControls(IBaseFilter *filter) const
{
    if (!filter)
        return QVariantList();

    qint32 min;
    qint32 max;
    qint32 step;
    qint32 defaultValue;
    qint32 flags;
    qint32 value;

    QVariantList controls;
    IAMVideoProcAmp *pProcAmp = nullptr;

    if (SUCCEEDED(filter->QueryInterface(IID_IAMVideoProcAmp,
                                         reinterpret_cast<void **>(&pProcAmp)))) {
        for (const VideoProcAmpProperty &property: vpapToStr->keys()) {
            if (SUCCEEDED(pProcAmp->GetRange(property,
                                             reinterpret_cast<LONG *>(&min),
                                             reinterpret_cast<LONG *>(&max),
                                             reinterpret_cast<LONG *>(&step),
                                             reinterpret_cast<LONG *>(&defaultValue),
                                             reinterpret_cast<LONG *>(&flags))))
                if (SUCCEEDED(pProcAmp->Get(property,
                                            reinterpret_cast<LONG *>(&value),
                                            reinterpret_cast<LONG *>(&flags)))) {
                    QVariantList control;

                    QString type;

                    if (property == VideoProcAmp_ColorEnable
                        || property == VideoProcAmp_BacklightCompensation)
                        type = "boolean";
                    else
                        type = "integer";

                    control << vpapToStr->value(property)
                            << type
                            << min
                            << max
                            << step
                            << defaultValue
                            << value
                            << QStringList();

                    controls << QVariant(control);
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
        for (const VideoProcAmpProperty &property: vpapToStr->keys()) {
            QString propertyStr = vpapToStr->value(property);

            if (imageControls.contains(propertyStr))
                pProcAmp->Set(property,
                              imageControls[propertyStr].toInt(),
                              VideoProcAmp_Flags_Manual);
        }

        pProcAmp->Release();
    }

    return true;
}

QVariantList CaptureDShowPrivate::cameraControls(IBaseFilter *filter) const
{
    if (!filter)
        return QVariantList();

    qint32 min;
    qint32 max;
    qint32 step;
    qint32 defaultValue;
    qint32 flags;
    qint32 value;

    QVariantList controls;
    IAMCameraControl *pCameraControl = nullptr;

    if (SUCCEEDED(filter->QueryInterface(IID_IAMCameraControl,
                                         reinterpret_cast<void **>(&pCameraControl)))) {
        for (const CameraControlProperty &cameraControl: ccToStr->keys()) {
            if (SUCCEEDED(pCameraControl->GetRange(cameraControl,
                                                   reinterpret_cast<LONG *>(&min),
                                                   reinterpret_cast<LONG *>(&max),
                                                   reinterpret_cast<LONG *>(&step),
                                                   reinterpret_cast<LONG *>(&defaultValue),
                                                   reinterpret_cast<LONG *>(&flags))))
                if (SUCCEEDED(pCameraControl->Get(cameraControl,
                                                  reinterpret_cast<LONG *>(&value),
                                                  reinterpret_cast<LONG *>(&flags)))) {
                    QVariantList control;

                    control << ccToStr->value(cameraControl)
                            << QString("integer")
                            << min
                            << max
                            << step
                            << defaultValue
                            << value
                            << QStringList();

                    controls << QVariant(control);
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
        for (const CameraControlProperty &cameraControl: ccToStr->keys()) {
            QString cameraControlStr = ccToStr->value(cameraControl);

            if (cameraControls.contains(cameraControlStr))
                pCameraControl->Set(cameraControl,
                                    cameraControls[cameraControlStr].toInt(),
                                    CameraControl_Flags_Manual);
        }

        pCameraControl->Release();
    }

    return true;
}

QVariantMap CaptureDShowPrivate::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    for (auto &control: controls) {
        QVariantList params = control.toList();
        QString controlName = params[0].toString();
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

bool CaptureDShow::init()
{
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

    this->d->m_grabber = SampleGrabberPtr(grabberPtr, this->d->deleteUnknown);

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
        HRESULT hr =
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

    this->d->m_localImageControls.clear();
    this->d->m_localCameraControls.clear();

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
    Q_UNUSED(nBuffers);
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

void CaptureDShow::frameReceived(qreal time, const QByteArray &buffer)
{
    Q_UNUSED(time)

    this->d->m_mutex.lock();
    this->d->m_curBuffer = buffer;
    this->d->m_waitCondition.wakeAll();
    this->d->m_mutex.unlock();
}

#include "moc_capturedshow.cpp"
