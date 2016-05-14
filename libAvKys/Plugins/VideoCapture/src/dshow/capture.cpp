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

#include "capture.h"

typedef QMap<VideoProcAmpProperty, QString> VideoProcAmpPropertyMap;

inline VideoProcAmpPropertyMap initVideoProcAmpPropertyMap()
{
    VideoProcAmpPropertyMap vpapToStr;
    vpapToStr[VideoProcAmp_Brightness] = "Brightness";
    vpapToStr[VideoProcAmp_Contrast] = "Contrast";
    vpapToStr[VideoProcAmp_Hue] = "Hue";
    vpapToStr[VideoProcAmp_Saturation] = "Saturation";
    vpapToStr[VideoProcAmp_Sharpness] = "Sharpness";
    vpapToStr[VideoProcAmp_Gamma] = "Gamma";
    vpapToStr[VideoProcAmp_ColorEnable] = "Color Enable";
    vpapToStr[VideoProcAmp_WhiteBalance] = "White Balance";
    vpapToStr[VideoProcAmp_BacklightCompensation] = "Backlight Compensation";
    vpapToStr[VideoProcAmp_Gain] = "Gain";

    return vpapToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(VideoProcAmpPropertyMap, vpapToStr, (initVideoProcAmpPropertyMap()))

typedef QMap<CameraControlProperty, QString> CameraControlMap;

inline CameraControlMap initCameraControlMap()
{
    CameraControlMap ccToStr;
    ccToStr[CameraControl_Pan] = "Pan";
    ccToStr[CameraControl_Tilt] = "Tilt";
    ccToStr[CameraControl_Roll] = "Roll";
    ccToStr[CameraControl_Zoom] = "Zoom";
    ccToStr[CameraControl_Exposure] = "Exposure";
    ccToStr[CameraControl_Iris] = "Iris";
    ccToStr[CameraControl_Focus] = "Focus";

    return ccToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(CameraControlMap, ccToStr, (initCameraControlMap()))

typedef QMap<GUID, QString> GuidToStrMap;

inline GuidToStrMap initGuidToStrMap()
{
    GuidToStrMap guidToStr;
    guidToStr[MEDIASUBTYPE_CLPL] = "CLPL";
    guidToStr[MEDIASUBTYPE_YUYV] = "YUYV";
    guidToStr[MEDIASUBTYPE_IYUV] = "IYUV";
    guidToStr[MEDIASUBTYPE_YVU9] = "YVU9";
    guidToStr[MEDIASUBTYPE_Y411] = "Y411";
    guidToStr[MEDIASUBTYPE_Y41P] = "Y41P";
    guidToStr[MEDIASUBTYPE_YUY2] = "YUY2";
    guidToStr[MEDIASUBTYPE_YVYU] = "YVYU";
    guidToStr[MEDIASUBTYPE_UYVY] = "UYVY";
    guidToStr[MEDIASUBTYPE_Y211] = "Y211";
    guidToStr[MEDIASUBTYPE_CLJR] = "CLJR";
    guidToStr[MEDIASUBTYPE_IF09] = "IF09";
    guidToStr[MEDIASUBTYPE_CPLA] = "CPLA";
    guidToStr[MEDIASUBTYPE_MJPG] = "MJPG";
    guidToStr[MEDIASUBTYPE_TVMJ] = "TVMJ";
    guidToStr[MEDIASUBTYPE_WAKE] = "WAKE";
    guidToStr[MEDIASUBTYPE_CFCC] = "CFCC";
    guidToStr[MEDIASUBTYPE_IJPG] = "IJPG";
    guidToStr[MEDIASUBTYPE_Plum] = "Plum";
    guidToStr[MEDIASUBTYPE_DVCS] = "DVCS";
    guidToStr[MEDIASUBTYPE_DVSD] = "DVSD";
    guidToStr[MEDIASUBTYPE_MDVF] = "MDVF";
    guidToStr[MEDIASUBTYPE_RGB1] = "RGB1";
    guidToStr[MEDIASUBTYPE_RGB4] = "RGB4";
    guidToStr[MEDIASUBTYPE_RGB8] = "RGB8";
    guidToStr[MEDIASUBTYPE_RGB565] = "RGBP";
    guidToStr[MEDIASUBTYPE_RGB555] = "RGBO";
    guidToStr[MEDIASUBTYPE_RGB24] = "RGB3";
    guidToStr[MEDIASUBTYPE_RGB32] = "RGB4";
    guidToStr[MEDIASUBTYPE_ARGB1555] = "AR15";
    guidToStr[MEDIASUBTYPE_ARGB4444] = "AR12";
    guidToStr[MEDIASUBTYPE_ARGB32] = "BA24";
    guidToStr[MEDIASUBTYPE_AYUV] = "AYUV";
    guidToStr[MEDIASUBTYPE_AI44] = "AI44";
    guidToStr[MEDIASUBTYPE_IA44] = "IA44";
    guidToStr[MEDIASUBTYPE_RGB32_D3D_DX7_RT] = "7R32";
    guidToStr[MEDIASUBTYPE_RGB16_D3D_DX7_RT] = "7R16";
    guidToStr[MEDIASUBTYPE_ARGB32_D3D_DX7_RT] = "7A88";
    guidToStr[MEDIASUBTYPE_ARGB4444_D3D_DX7_RT] = "7A44";
    guidToStr[MEDIASUBTYPE_ARGB1555_D3D_DX7_RT] = "7A15";
    guidToStr[MEDIASUBTYPE_RGB32_D3D_DX9_RT] = "9R32";
    guidToStr[MEDIASUBTYPE_RGB16_D3D_DX9_RT] = "9R16";
    guidToStr[MEDIASUBTYPE_ARGB32_D3D_DX9_RT] = "9A88";
    guidToStr[MEDIASUBTYPE_ARGB4444_D3D_DX9_RT] = "9A44";
    guidToStr[MEDIASUBTYPE_ARGB1555_D3D_DX9_RT] = "9A15";
    guidToStr[MEDIASUBTYPE_YV12] = "YV12";
    guidToStr[MEDIASUBTYPE_NV12] = "NV12";
    guidToStr[MEDIASUBTYPE_IMC1] = "IMC1";
    guidToStr[MEDIASUBTYPE_IMC2] = "IMC2";
    guidToStr[MEDIASUBTYPE_IMC3] = "IMC3";
    guidToStr[MEDIASUBTYPE_IMC4] = "IMC4";
    guidToStr[MEDIASUBTYPE_S340] = "S340";
    guidToStr[MEDIASUBTYPE_S342] = "S342";
    guidToStr[MEDIASUBTYPE_QTRpza] = "rpza";
    guidToStr[MEDIASUBTYPE_QTSmc] = "smc ";
    guidToStr[MEDIASUBTYPE_QTRle] = "rle ";
    guidToStr[MEDIASUBTYPE_QTJpeg] = "jpeg";
    guidToStr[MEDIASUBTYPE_dvsd] = "dvsd";
    guidToStr[MEDIASUBTYPE_dvhd] = "dvhd";
    guidToStr[MEDIASUBTYPE_dvsl] = "dvsl";
    guidToStr[MEDIASUBTYPE_dv25] = "dv25";
    guidToStr[MEDIASUBTYPE_dv50] = "dv50";
    guidToStr[MEDIASUBTYPE_dvh1] = "dvh1";

    return guidToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(GuidToStrMap, guidToStr, (initGuidToStrMap()))

typedef QMap<Capture::IoMethod, QString> IoMethodMap;

inline IoMethodMap initIoMethodMap()
{
    IoMethodMap ioMethodToStr;
    ioMethodToStr[Capture::IoMethodDirectRead] = "directRead";
    ioMethodToStr[Capture::IoMethodGrabSample] = "grabSample";
    ioMethodToStr[Capture::IoMethodGrabBuffer] = "grabBuffer";

    return ioMethodToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(IoMethodMap, ioMethodToStr, (initIoMethodMap()))

Capture::Capture(): QObject()
{
    this->m_id = -1;
    this->m_ioMethod = IoMethodGrabSample;
    this->m_graph = NULL;

    QObject::connect(&this->m_frameGrabber,
                     &FrameGrabber::frameReady,
                     this,
                     &Capture::frameReceived,
                     Qt::DirectConnection);

    this->createDeviceNotifier();
}

QStringList Capture::webcams() const
{
    return this->listMonikers().keys();
}

QString Capture::device() const
{
    return this->m_device;
}

QList<int> Capture::streams() const
{
    if (!this->m_streams.isEmpty())
        return this->m_streams;

    QVariantList caps = this->caps(this->m_device);

    if (caps.isEmpty())
        return QList<int>();

    return QList<int>() << 0;
}

QList<int> Capture::listTracks(const QString &mimeType)
{
    if (mimeType != "video/x-raw"
        || !mimeType.isEmpty())
        return QList<int>();

    QVariantList caps = this->caps(this->m_device);
    QList<int> streams;

    for (int i = 0; i < caps.count(); i++)
        streams << i;

    return streams;
}

QString Capture::ioMethod() const
{
    return ioMethodToStr->value(this->m_ioMethod, "any");
}

int Capture::nBuffers() const
{
    return 0;
}

QString Capture::description(const QString &webcam) const
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

QVariantList Capture::caps(const QString &webcam) const
{
    QVariantList caps;
    MediaTypesList mediaTypes = this->listMediaTypes(webcam);

    foreach (MediaTypePtr mediaType, mediaTypes) {
        AkCaps videoCaps = this->capsFromMediaType(mediaType);

        if (!videoCaps)
            continue;

        caps << QVariant::fromValue(videoCaps);
    }

    return caps;
}

QString Capture::capsDescription(const AkCaps &caps) const
{
    if (caps.mimeType() != "video/unknown")
        return QString();

    return QString("%1, %2x%3 %4 fps")
                .arg(caps.property("fourcc").toString())
                .arg(caps.property("width").toInt())
                .arg(caps.property("height").toInt())
                .arg(caps.property("fps").toString());
}

QVariantList Capture::imageControls() const
{
    BaseFilterPtr filter = this->findFilter(this->m_device);

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
        foreach (VideoProcAmpProperty property, vpapToStr->keys()) {
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

bool Capture::setImageControls(const QVariantMap &imageControls) const
{
    BaseFilterPtr filter = this->findFilter(this->m_device);

    if (!filter)
        return false;

    IAMVideoProcAmp *pProcAmp = NULL;

    if (SUCCEEDED(filter->QueryInterface(IID_IAMVideoProcAmp,
                                         reinterpret_cast<void **>(&pProcAmp)))) {
        foreach (VideoProcAmpProperty property, vpapToStr->keys()) {
            QString propertyStr = vpapToStr->value(property);

            if (imageControls.contains(propertyStr))
                pProcAmp->Set(property,
                              imageControls[propertyStr].toInt(),
                              VideoProcAmp_Flags_Manual);
        }

        pProcAmp->Release();
    }

    emit this->imageControlsChanged(imageControls);

    return true;
}

bool Capture::resetImageControls() const
{
    QVariantMap controls;

    foreach (QVariant control, this->imageControls()) {
        QVariantList params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setImageControls(controls);
}

QVariantList Capture::cameraControls() const
{
    BaseFilterPtr filter = this->findFilter(this->m_device);

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
        foreach (CameraControlProperty cameraControl, ccToStr->keys()) {
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

bool Capture::setCameraControls(const QVariantMap &cameraControls) const
{
    BaseFilterPtr filter = this->findFilter(this->m_device);

    if (!filter)
        return false;

    IAMCameraControl *pCameraControl = NULL;

    if (SUCCEEDED(filter->QueryInterface(IID_IAMCameraControl,
                                         reinterpret_cast<void **>(&pCameraControl)))) {
        foreach (CameraControlProperty cameraControl, ccToStr->keys()) {
            QString cameraControlStr = ccToStr->value(cameraControl);

            if (cameraControls.contains(cameraControlStr))
                pCameraControl->Set(cameraControl,
                                    cameraControls[cameraControlStr].toInt(),
                                    CameraControl_Flags_Manual);
        }

        pCameraControl->Release();
    }

    emit this->cameraControlsChanged(cameraControls);

    return true;
}

bool Capture::resetCameraControls() const
{
    QVariantMap controls;

    foreach (QVariant control, this->cameraControls()) {
        QVariantList params = control.toList();

        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setCameraControls(controls);
}

AkPacket Capture::readFrame()
{
    AkPacket packet;

    timeval timestamp;
    gettimeofday(&timestamp, NULL);

    qint64 pts = qint64((timestamp.tv_sec
                         + 1e-6 * timestamp.tv_usec)
                        * this->m_timeBase.invert().value());

    if (this->m_ioMethod != IoMethodDirectRead) {
        this->m_mutex.lock();

        if (this->m_curBuffer.isEmpty())
            this->m_waitCondition.wait(&this->m_mutex, 1000);

        if (!this->m_curBuffer.isEmpty()) {
            int bufferSize = this->m_curBuffer.size();
            QByteArray oBuffer(bufferSize, Qt::Uninitialized);
            memcpy(oBuffer.data(), this->m_curBuffer.constData(), size_t(bufferSize));

            packet = AkPacket(this->m_caps, oBuffer);
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

        packet = AkPacket(this->m_caps, oBuffer);
        packet.setPts(pts);
        packet.setTimeBase(this->m_timeBase);
        packet.setIndex(0);
        packet.setId(this->m_id);
    }

    return packet;
}

AkCaps Capture::capsFromMediaType(const MediaTypePtr &mediaType) const
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
    AkFrac fps(1.0e8, videoInfoHeader->AvgTimePerFrame);
    videoCaps.setProperty("fps", fps.toString());

    return videoCaps;
}

HRESULT Capture::enumerateCameras(IEnumMoniker **ppEnum) const
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
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, ppEnum, 0);

        if (hr == S_FALSE)
            hr = VFW_E_NOT_FOUND;

        pDevEnum->Release();
    }

    return hr;
}

MonikersMap Capture::listMonikers() const
{
    MonikersMap monikers;
    IEnumMoniker *pEnum = NULL;
    HRESULT hr = this->enumerateCameras(&pEnum);

    if (SUCCEEDED(hr)) {
        IMoniker *pMoniker = NULL;

        for (int i = 0; pEnum->Next(1, &pMoniker, NULL) == S_OK; i++) {
            IPropertyBag *pPropBag;
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

MonikerPtr Capture::findMoniker(const QString &webcam) const
{
    MonikersMap monikers = this->listMonikers();

    if (monikers.contains(webcam))
        return monikers[webcam];
    else
        return MonikerPtr();
}

IBaseFilter *Capture::findFilterP(const QString &webcam) const
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

BaseFilterPtr Capture::findFilter(const QString &webcam) const
{
    IBaseFilter *filter = this->findFilterP(webcam);

    if (!filter)
        return BaseFilterPtr();

    return BaseFilterPtr(filter, this->deleteUnknown);
}

MediaTypesList Capture::listMediaTypes(const QString &webcam) const
{
    BaseFilterPtr filter = this->findFilter(webcam);

    return this->listMediaTypes(filter.data());
}

MediaTypesList Capture::listMediaTypes(IBaseFilter *filter) const
{
    PinList pins = this->enumPins(filter, PINDIR_OUTPUT);
    MediaTypesList mediaTypes;

    foreach (PinPtr pin, pins) {
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

bool Capture::isPinConnected(IPin *pPin, bool *ok) const
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

PinPtr Capture::findUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir) const
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

bool Capture::connectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IBaseFilter *pDest) const
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

PinList Capture::enumPins(IBaseFilter *filter, PIN_DIRECTION direction) const
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

bool Capture::createDeviceNotifier()
{
    QString className("HiddenWindow");
    HINSTANCE instance = qWinAppInst();

    WNDCLASS wc;
    wc.style = 0;
    wc.lpfnWndProc = this->deviceEvents;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hIcon = 0;
    wc.hCursor = 0;
    wc.hbrBackground = 0;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = reinterpret_cast<const wchar_t *>(className.utf16());
    RegisterClass(&wc);

    HWND hwnd = CreateWindow(wc.lpszClassName,
                             wc.lpszClassName,
                             0,
                             0, 0, 0, 0,
                             0,
                             0,
                             instance,
                             0);

    if (!hwnd)
        return false;

    SetWindowLongPtr(hwnd, GWLP_USERDATA, LONG_PTR(this));

    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
    ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));

    NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;

    HDEVNOTIFY result = RegisterDeviceNotification(hwnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);

    if (!result)
        return false;

    return true;
}

LRESULT Capture::deviceEvents(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (wParam == DBT_DEVICEARRIVAL
        || wParam == DBT_DEVICEREMOVECOMPLETE) {
        Capture *thisPtr = reinterpret_cast<Capture *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        QStringList webcams = thisPtr->webcams();

        if (webcams != thisPtr->m_webcams) {
            emit thisPtr->webcamsChanged(webcams);

            thisPtr->m_webcams = webcams;
        }
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

void Capture::deleteUnknown(IUnknown *unknown)
{
    unknown->Release();
}

void Capture::deleteMediaType(AM_MEDIA_TYPE *mediaType)
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

void Capture::deletePin(IPin *pin)
{
    pin->Release();
}

bool Capture::init()
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

    if (FAILED(this->m_graph->AddFilter(webcamFilter, L"Source"))) {
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

    MediaTypePtr mediaType = mediaTypes[streams[0]];

    if (FAILED(grabberPtr->SetMediaType(mediaType.data()))) {
        this->m_graph->Release();
        this->m_graph = NULL;

        return false;
    }

    PinList pins = this->enumPins(webcamFilter, PINDIR_OUTPUT);

    foreach (PinPtr pin, pins) {
        IAMStreamConfig *pStreamConfig = NULL;
        HRESULT hr = pin->QueryInterface(IID_IAMStreamConfig,
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
    this->m_caps = this->capsFromMediaType(mediaType);
    this->m_timeBase = AkFrac(this->m_caps.property("fps").toString()).invert();

    if (FAILED(control->Run())) {
        control->Release();
        this->m_graph->Release();
        this->m_graph = NULL;

        return false;
    }

    control->Release();

    return true;
}

void Capture::uninit()
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

void Capture::setDevice(const QString &device)
{
    if (this->m_device == device)
        return;

    this->m_device = device;
    emit this->deviceChanged(device);
}

void Capture::setStreams(const QList<int> &streams)
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

void Capture::setIoMethod(const QString &ioMethod)
{
    IoMethod ioMethodEnum = ioMethodToStr->key(ioMethod, IoMethodGrabSample);

    if (this->m_ioMethod == ioMethodEnum)
        return;

    this->m_ioMethod = ioMethodEnum;
    emit this->ioMethodChanged(ioMethod);
}

void Capture::setNBuffers(int nBuffers)
{
    Q_UNUSED(nBuffers);
}

void Capture::resetDevice()
{
    this->setDevice("");
}

void Capture::resetStreams()
{
    QVariantList supportedCaps = this->caps(this->m_device);
    QList<int> streams;

    if (!supportedCaps.isEmpty())
        streams << 0;

    this->setStreams(streams);
}

void Capture::resetIoMethod()
{
    this->setIoMethod("any");
}

void Capture::resetNBuffers()
{
}

void Capture::reset()
{
    this->resetStreams();
    this->resetImageControls();
    this->resetCameraControls();
}

void Capture::frameReceived(qreal time, const QByteArray &buffer)
{
    Q_UNUSED(time)

    this->m_mutex.lock();
    this->m_curBuffer = buffer;
    this->m_waitCondition.wakeAll();
    this->m_mutex.unlock();
}
