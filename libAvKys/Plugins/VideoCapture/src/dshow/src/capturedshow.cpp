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

#include <QCoreApplication>
#include <QDateTime>

#include "capturedshow.h"

#define TIME_BASE 1.0e7
#define SOURCE_FILTER_NAME L"Source"

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
        {MEDIASUBTYPE_RGB4               , "RGB4"},
        {MEDIASUBTYPE_RGB8               , "RGB8"},
        {MEDIASUBTYPE_RGB565             , "RGBP"},
        {MEDIASUBTYPE_RGB555             , "RGBO"},
        {MEDIASUBTYPE_RGB24              , "RGB3"},
        {MEDIASUBTYPE_RGB32              , "RGB4"},
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

CaptureDShow::CaptureDShow(QObject *parent):
    Capture(parent),
    QAbstractNativeEventFilter()
{
    this->m_id = -1;
    this->m_ioMethod = IoMethodGrabSample;
    this->m_graph = NULL;

    QObject::connect(&this->m_frameGrabber,
                     &FrameGrabber::frameReady,
                     this,
                     &CaptureDShow::frameReceived,
                     Qt::DirectConnection);

    qApp->installNativeEventFilter(this);
}

CaptureDShow::~CaptureDShow()
{
    qApp->removeNativeEventFilter(this);
}

QStringList CaptureDShow::webcams() const
{
    return this->listMonikers().keys();
}

QString CaptureDShow::device() const
{
    return this->m_device;
}

QList<int> CaptureDShow::streams() const
{
    if (!this->m_streams.isEmpty())
        return this->m_streams;

    QVariantList caps = this->caps(this->m_device);

    if (caps.isEmpty())
        return QList<int>();

    return QList<int>() << 0;
}

QList<int> CaptureDShow::listTracks(const QString &mimeType)
{
    if (mimeType != "video/x-raw"
        && !mimeType.isEmpty())
        return QList<int>();

    QVariantList caps = this->caps(this->m_device);
    QList<int> streams;

    for (int i = 0; i < caps.count(); i++)
        streams << i;

    return streams;
}

QString CaptureDShow::ioMethod() const
{
    return ioMethodToStr->value(this->m_ioMethod, "any");
}

int CaptureDShow::nBuffers() const
{
    return 0;
}

QString CaptureDShow::description(const QString &webcam) const
{
    if (webcam.isEmpty())
        return QString();

    MonikerPtr moniker = this->findMoniker(webcam);

    if (!moniker)
        return QString();

    IPropertyBag *pPropBag = NULL;
    HRESULT hr = moniker->BindToStorage(0,
                                        0,
                                        IID_IPropertyBag,
                                        reinterpret_cast<void **>(&pPropBag));

    if (FAILED(hr))
        return QString();

    VARIANT var;
    VariantInit(&var);

    // Get description or friendly name.
    hr = pPropBag->Read(L"Description", &var, 0);

    if (FAILED(hr))
        hr = pPropBag->Read(L"FriendlyName", &var, 0);

    QString description;

    if (SUCCEEDED(hr))
        description = QString::fromWCharArray(var.bstrVal);

    pPropBag->Release();

    return description;
}

QVariantList CaptureDShow::caps(const QString &webcam) const
{
    QVariantList caps;
    MediaTypesList mediaTypes = this->listMediaTypes(webcam);

    for (const MediaTypePtr &mediaType: mediaTypes) {
        AkCaps videoCaps = this->capsFromMediaType(mediaType);

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
    return this->m_globalImageControls;
}

bool CaptureDShow::setImageControls(const QVariantMap &imageControls)
{
    this->m_controlsMutex.lock();
    QVariantList globalImageControls = this->m_globalImageControls;
    this->m_controlsMutex.unlock();

    for (int i = 0; i < globalImageControls.count(); i++) {
        QVariantList control = globalImageControls[i].toList();
        QString controlName = control[0].toString();

        if (imageControls.contains(controlName)) {
            control[6] = imageControls[controlName];
            globalImageControls[i] = control;
        }
    }

    this->m_controlsMutex.lock();

    if (this->m_globalImageControls == globalImageControls) {
        this->m_controlsMutex.unlock();

        return false;
    }

    this->m_globalImageControls = globalImageControls;
    this->m_controlsMutex.unlock();

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
    return this->m_globalCameraControls;
}

bool CaptureDShow::setCameraControls(const QVariantMap &cameraControls)
{
    this->m_controlsMutex.lock();
    QVariantList globalCameraControls = this->m_globalCameraControls;
    this->m_controlsMutex.unlock();

    for (int i = 0; i < globalCameraControls.count(); i++) {
        QVariantList control = globalCameraControls[i].toList();
        QString controlName = control[0].toString();

        if (cameraControls.contains(controlName)) {
            control[6] = cameraControls[controlName];
            globalCameraControls[i] = control;
        }
    }

    this->m_controlsMutex.lock();

    if (this->m_globalCameraControls == globalCameraControls) {
        this->m_controlsMutex.unlock();

        return false;
    }

    this->m_globalCameraControls = globalCameraControls;
    this->m_controlsMutex.unlock();
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
    IBaseFilter *source = NULL;
    this->m_graph->FindFilterByName(SOURCE_FILTER_NAME, &source);

    if (source) {
        this->m_controlsMutex.lock();
        QVariantMap imageControls = this->controlStatus(this->m_globalImageControls);
        this->m_controlsMutex.unlock();

        if (this->m_localImageControls != imageControls) {
            QVariantMap controls = this->mapDiff(this->m_localImageControls,
                                                 imageControls);
            this->setImageControls(source, controls);
            this->m_localImageControls = imageControls;
        }

        this->m_controlsMutex.lock();
        QVariantMap cameraControls = this->controlStatus(this->m_globalCameraControls);
        this->m_controlsMutex.unlock();

        if (this->m_localCameraControls != cameraControls) {
            QVariantMap controls = this->mapDiff(this->m_localCameraControls,
                                                 cameraControls);
            this->setCameraControls(source, controls);
            this->m_localCameraControls = cameraControls;
        }

        source->Release();
    }

    AM_MEDIA_TYPE mediaType;
    ZeroMemory(&mediaType, sizeof(AM_MEDIA_TYPE));
    this->m_grabber->GetConnectedMediaType(&mediaType);
    AkCaps caps = this->capsFromMediaType(&mediaType);

    AkPacket packet;

    auto timestamp = QDateTime::currentMSecsSinceEpoch();

    qint64 pts = qint64(timestamp
                        * this->m_timeBase.invert().value()
                        / 1e3);

    if (this->m_ioMethod != IoMethodDirectRead) {
        this->m_mutex.lock();

        if (this->m_curBuffer.isEmpty())
            this->m_waitCondition.wait(&this->m_mutex, 1000);

        if (!this->m_curBuffer.isEmpty()) {
            int bufferSize = this->m_curBuffer.size();
            QByteArray oBuffer(bufferSize, Qt::Uninitialized);
            memcpy(oBuffer.data(),
                   this->m_curBuffer.constData(),
                   size_t(bufferSize));

            packet = AkPacket(caps, oBuffer);
            packet.setPts(pts);
            packet.setTimeBase(this->m_timeBase);
            packet.setIndex(0);
            packet.setId(this->m_id);
            this->m_curBuffer.clear();
        }

        this->m_mutex.unlock();
    } else {
        long bufferSize;

        HRESULT hr = this->m_grabber->GetCurrentBuffer(&bufferSize, NULL);

        if (FAILED(hr))
            return AkPacket();

        QByteArray oBuffer(bufferSize, Qt::Uninitialized);
        hr = this->m_grabber->GetCurrentBuffer(&bufferSize,
                                               reinterpret_cast<long *>(oBuffer.data()));

        if (FAILED(hr))
            return AkPacket();

        packet = AkPacket(caps, oBuffer);
        packet.setPts(pts);
        packet.setTimeBase(this->m_timeBase);
        packet.setIndex(0);
        packet.setId(this->m_id);
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

            if (webcams != this->m_webcams) {
                emit this->webcamsChanged(webcams);

                this->m_webcams = webcams;
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

AkCaps CaptureDShow::capsFromMediaType(const AM_MEDIA_TYPE *mediaType) const
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

AkCaps CaptureDShow::capsFromMediaType(const MediaTypePtr &mediaType) const
{
    return this->capsFromMediaType(mediaType.data());
}

HRESULT CaptureDShow::enumerateCameras(IEnumMoniker **ppEnum) const
{
    // Create the System Device Enumerator.
    ICreateDevEnum *pDevEnum = NULL;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum,
                                  NULL,
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

MonikersMap CaptureDShow::listMonikers() const
{
    MonikersMap monikers;
    IEnumMoniker *pEnum = NULL;
    HRESULT hr = this->enumerateCameras(&pEnum);

    if (SUCCEEDED(hr)) {
        IMoniker *pMoniker = NULL;

        for (int i = 0; pEnum->Next(1, &pMoniker, NULL) == S_OK; i++) {
            IPropertyBag *pPropBag = NULL;
            HRESULT hr = pMoniker->BindToStorage(0,
                                                 0,
                                                 IID_IPropertyBag,
                                                 reinterpret_cast<void **>(&pPropBag));

            if (FAILED(hr)) {
                pMoniker->Release();

                continue;
            }

            VARIANT var;
            VariantInit(&var);
            hr = pPropBag->Read(L"DevicePath", &var, 0);

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

MonikerPtr CaptureDShow::findMoniker(const QString &webcam) const
{
    MonikersMap monikers = this->listMonikers();

    if (monikers.contains(webcam))
        return monikers[webcam];
    else
        return MonikerPtr();
}

IBaseFilter *CaptureDShow::findFilterP(const QString &webcam) const
{
    MonikerPtr moniker = this->findMoniker(webcam);

    if (!moniker)
        return NULL;

    IBaseFilter *filter = NULL;

    HRESULT hr = moniker->BindToObject(NULL,
                                       NULL,
                                       IID_IBaseFilter,
                                       reinterpret_cast<void **>(&filter));

    if (FAILED(hr))
        return NULL;

    return filter;
}

BaseFilterPtr CaptureDShow::findFilter(const QString &webcam) const
{
    IBaseFilter *filter = this->findFilterP(webcam);

    if (!filter)
        return BaseFilterPtr();

    return BaseFilterPtr(filter, this->deleteUnknown);
}

MediaTypesList CaptureDShow::listMediaTypes(const QString &webcam) const
{
    BaseFilterPtr filter = this->findFilter(webcam);

    return this->listMediaTypes(filter.data());
}

MediaTypesList CaptureDShow::listMediaTypes(IBaseFilter *filter) const
{
    PinList pins = this->enumPins(filter, PINDIR_OUTPUT);
    MediaTypesList mediaTypes;

    for (const PinPtr &pin: pins) {
        IEnumMediaTypes *pEnum = NULL;
        pin->EnumMediaTypes(&pEnum);
        AM_MEDIA_TYPE *mediaType = NULL;

        while (pEnum->Next(1, &mediaType, NULL) == S_OK)
            if (mediaType->formattype == FORMAT_VideoInfo
                && mediaType->cbFormat >= sizeof(VIDEOINFOHEADER)
                && mediaType->pbFormat != NULL
                && guidToStr->contains(mediaType->subtype)) {
                mediaTypes << MediaTypePtr(mediaType, this->deleteMediaType);
            } else {
                this->deleteMediaType(mediaType);
            }

        pEnum->Release();
    }

    return mediaTypes;
}

bool CaptureDShow::isPinConnected(IPin *pPin, bool *ok) const
{
    IPin *pTmp = NULL;
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

PinPtr CaptureDShow::findUnconnectedPin(IBaseFilter *pFilter,
                                        PIN_DIRECTION PinDir) const
{
    IEnumPins *pEnum = NULL;

    if (FAILED(pFilter->EnumPins(&pEnum)))
        return PinPtr();

    PinPtr matchedPin;
    IPin *pPin = NULL;

    while (pEnum->Next(1, &pPin, NULL) == S_OK) {
        PIN_DIRECTION pinDir;

        if (FAILED(pPin->QueryDirection(&pinDir))
            || pinDir != PinDir)
            continue;

        bool ok;
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

bool CaptureDShow::connectFilters(IGraphBuilder *pGraph,
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

PinList CaptureDShow::enumPins(IBaseFilter *filter,
                               PIN_DIRECTION direction) const
{
    if (!filter)
        return PinList();

    PinList pinList;
    IEnumPins *enumPins = NULL;

    if (SUCCEEDED(filter->EnumPins(&enumPins))) {
        IPin *pin = NULL;

        while (S_OK == enumPins->Next(1, &pin, NULL)) {
            PIN_DIRECTION pinDir;

            if (SUCCEEDED(pin->QueryDirection(&pinDir))
                && pinDir == direction) {
                pinList << PinPtr(pin, this->deleteUnknown);

                continue;
            }

            pin->Release();
            pin = NULL;
        }
    }

    enumPins->Release();

    return pinList;
}

void CaptureDShow::deleteUnknown(IUnknown *unknown)
{
    unknown->Release();
}

void CaptureDShow::deleteMediaType(AM_MEDIA_TYPE *mediaType)
{
    if (!mediaType)
        return;

    if (mediaType->cbFormat != 0) {
        CoTaskMemFree(PVOID(mediaType->pbFormat));
        mediaType->cbFormat = 0;
        mediaType->pbFormat = NULL;
    }

    if (mediaType->pUnk != NULL) {
        // pUnk should not be used.
        mediaType->pUnk->Release();
        mediaType->pUnk = NULL;
    }

    CoTaskMemFree(mediaType);
}

void CaptureDShow::deletePin(IPin *pin)
{
    pin->Release();
}

QVariantList CaptureDShow::imageControls(IBaseFilter *filter) const
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
    IAMVideoProcAmp *pProcAmp = NULL;

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

                    if (value == defaultValue)
                        defaultValue = (min + max) / 2;

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

bool CaptureDShow::setImageControls(IBaseFilter *filter,
                               const QVariantMap &imageControls) const
{
    if (!filter)
        return false;

    IAMVideoProcAmp *pProcAmp = NULL;

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

QVariantList CaptureDShow::cameraControls(IBaseFilter *filter) const
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
    IAMCameraControl *pCameraControl = NULL;

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

bool CaptureDShow::setCameraControls(IBaseFilter *filter,
                                const QVariantMap &cameraControls) const
{
    if (!filter)
        return false;

    IAMCameraControl *pCameraControl = NULL;

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

QVariantMap CaptureDShow::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    for (const QVariant &control: controls) {
        QVariantList params = control.toList();
        QString controlName = params[0].toString();
        controlStatus[controlName] = params[6];
    }

    return controlStatus;
}

QVariantMap CaptureDShow::mapDiff(const QVariantMap &map1,
                                  const QVariantMap &map2) const
{
    QVariantMap map;

    for (const QString &control: map2.keys())
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
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IGraphBuilder,
                                reinterpret_cast<void **>(&this->m_graph))))
        return false;

    // Create the webcam filter.
    IBaseFilter *webcamFilter = this->findFilterP(this->m_device);

    if (!webcamFilter) {
        this->m_graph->Release();
        this->m_graph = NULL;

        return false;
    }

    if (FAILED(this->m_graph->AddFilter(webcamFilter, SOURCE_FILTER_NAME))) {
        this->m_graph->Release();
        this->m_graph = NULL;

        return false;
    }

    // Create the Sample Grabber filter.
    IBaseFilter *grabberFilter = NULL;

    if (FAILED(CoCreateInstance(CLSID_SampleGrabber,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IBaseFilter,
                                reinterpret_cast<void **>(&grabberFilter)))) {
        this->m_graph->Release();
        this->m_graph = NULL;

        return false;
    }

    if (FAILED(this->m_graph->AddFilter(grabberFilter, L"Grabber"))) {
        this->m_graph->Release();
        this->m_graph = NULL;

        return false;
    }

    ISampleGrabber *grabberPtr = NULL;

    if (FAILED(grabberFilter->QueryInterface(IID_ISampleGrabber,
                                             reinterpret_cast<void **>(&grabberPtr)))) {
        this->m_graph->Release();
        this->m_graph = NULL;

        return false;
    }

    if (FAILED(grabberPtr->SetOneShot(FALSE))) {
        this->m_graph->Release();
        this->m_graph = NULL;

        return false;
    }

    HRESULT hr = grabberPtr->SetBufferSamples(TRUE);

    if (FAILED(hr)) {
        this->m_graph->Release();
        this->m_graph = NULL;

        return false;
    }

    if (this->m_ioMethod != IoMethodDirectRead) {
        int type = this->m_ioMethod == IoMethodGrabSample? 0: 1;
        hr = grabberPtr->SetCallback(&this->m_frameGrabber, type);
    }

    this->m_grabber = SampleGrabberPtr(grabberPtr, this->deleteUnknown);

    if (!this->connectFilters(this->m_graph, webcamFilter, grabberFilter)) {
        this->m_graph->Release();
        this->m_graph = NULL;

        return false;
    }

    // Create null filter.
    IBaseFilter *nullFilter = NULL;

    if (FAILED(CoCreateInstance(CLSID_NullRenderer,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IBaseFilter,
                                reinterpret_cast<void **>(&nullFilter)))) {
        this->m_graph->Release();
        this->m_graph = NULL;

        return false;
    }

    if (FAILED(this->m_graph->AddFilter(nullFilter, L"NullFilter"))) {
        this->m_graph->Release();
        this->m_graph = NULL;

        return false;
    }

    if (!this->connectFilters(this->m_graph, grabberFilter, nullFilter)) {
        this->m_graph->Release();
        this->m_graph = NULL;

        return false;
    }

    // Set capture format
    QList<int> streams = this->streams();

    if (streams.isEmpty()) {
        this->m_graph->Release();
        this->m_graph = NULL;

        return false;
    }

    MediaTypesList mediaTypes = this->listMediaTypes(webcamFilter);

    if (mediaTypes.isEmpty()) {
        this->m_graph->Release();
        this->m_graph = NULL;

        return false;
    }

    MediaTypePtr mediaType = streams[0] < mediaTypes.size()?
                                mediaTypes[streams[0]]:
                                mediaTypes.first();

    if (FAILED(grabberPtr->SetMediaType(mediaType.data()))) {
        this->m_graph->Release();
        this->m_graph = NULL;

        return false;
    }

    PinList pins = this->enumPins(webcamFilter, PINDIR_OUTPUT);

    for (const PinPtr &pin: pins) {
        IAMStreamConfig *pStreamConfig = NULL;
        HRESULT hr =
                pin->QueryInterface(IID_IAMStreamConfig,
                                    reinterpret_cast<void **>(&pStreamConfig));

        if (SUCCEEDED(hr))
            pStreamConfig->SetFormat(mediaType.data());

        if (pStreamConfig)
            pStreamConfig->Release();
    }

    // Run the pipeline
    IMediaControl *control = NULL;

    if (FAILED(this->m_graph->QueryInterface(IID_IMediaControl,
                                             reinterpret_cast<void **>(&control)))) {
        this->m_graph->Release();
        this->m_graph = NULL;

        return false;
    }

    this->m_id = Ak::id();
    AkCaps caps = this->capsFromMediaType(mediaType);
    this->m_timeBase = AkFrac(caps.property("fps").toString()).invert();

    if (FAILED(control->Run())) {
        control->Release();
        this->m_graph->Release();
        this->m_graph = NULL;

        return false;
    }

    control->Release();

    this->m_localImageControls.clear();
    this->m_localImageControls.clear();

    return true;
}

void CaptureDShow::uninit()
{
    IMediaControl *control = NULL;

    if (SUCCEEDED(this->m_graph->QueryInterface(IID_IMediaControl,
                                                reinterpret_cast<void **>(&control)))) {
        control->Stop();
        control->Release();
    }

    this->m_grabber.clear();
    this->m_graph->Release();
    this->m_graph = NULL;
}

void CaptureDShow::setDevice(const QString &device)
{
    if (this->m_device == device)
        return;

    this->m_device = device;

    if (device.isEmpty()) {
        this->m_controlsMutex.lock();
        this->m_globalImageControls.clear();
        this->m_globalCameraControls.clear();
        this->m_controlsMutex.unlock();
    } else {
        this->m_controlsMutex.lock();
        auto camera = this->findFilterP(device);

        if (camera) {
            this->m_globalImageControls = this->imageControls(camera);
            this->m_globalCameraControls = this->cameraControls(camera);
            camera->Release();
        }

        this->m_controlsMutex.unlock();
    }

    this->m_controlsMutex.lock();
    QVariantMap imageStatus = this->controlStatus(this->m_globalImageControls);
    QVariantMap cameraStatus = this->controlStatus(this->m_globalCameraControls);
    this->m_controlsMutex.unlock();

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

    QVariantList supportedCaps = this->caps(this->m_device);

    if (stream >= supportedCaps.length())
        return;

    QList<int> inputStreams;
    inputStreams << stream;

    if (this->streams() == inputStreams)
        return;

    this->m_streams = inputStreams;
    emit this->streamsChanged(inputStreams);
}

void CaptureDShow::setIoMethod(const QString &ioMethod)
{
    IoMethod ioMethodEnum = ioMethodToStr->key(ioMethod, IoMethodGrabSample);

    if (this->m_ioMethod == ioMethodEnum)
        return;

    this->m_ioMethod = ioMethodEnum;
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
    QVariantList supportedCaps = this->caps(this->m_device);
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

    this->m_mutex.lock();
    this->m_curBuffer = buffer;
    this->m_waitCondition.wakeAll();
    this->m_mutex.unlock();
}
