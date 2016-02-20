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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
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

typedef QMap<GUID, AkVideoCaps::PixelFormat> GuidToStrMap;

inline GuidToStrMap initGuidToStrMap()
{
    GuidToStrMap guidToStr;
    guidToStr[MEDIASUBTYPE_RGB1] = AkVideoCaps::Format_monob;
    guidToStr[MEDIASUBTYPE_RGB4] = AkVideoCaps::Format_bgr4_byte;
    guidToStr[MEDIASUBTYPE_RGB8] = AkVideoCaps::Format_pal8;
    guidToStr[MEDIASUBTYPE_RGB555] = AkVideoCaps::Format_bgr555le;
    guidToStr[MEDIASUBTYPE_RGB565] = AkVideoCaps::Format_bgr565le;
    guidToStr[MEDIASUBTYPE_RGB24] = AkVideoCaps::Format_bgr24;
    guidToStr[MEDIASUBTYPE_RGB32] = AkVideoCaps::Format_bgr0;
//    guidToStr[MEDIASUBTYPE_ARGB1555] = AkVideoCaps::Format_none;
    guidToStr[MEDIASUBTYPE_ARGB32] = AkVideoCaps::Format_bgra;
//    guidToStr[MEDIASUBTYPE_ARGB4444] = AkVideoCaps::Format_none;
//    guidToStr[MEDIASUBTYPE_A2R10G10B10] = AkVideoCaps::Format_none;
//    guidToStr[MEDIASUBTYPE_A2B10G10R10] = AkVideoCaps::Format_none;
    guidToStr[MEDIASUBTYPE_AYUV] = AkVideoCaps::Format_yuva420p;
    guidToStr[MEDIASUBTYPE_YUY2] = AkVideoCaps::Format_yuyv422;
    guidToStr[MEDIASUBTYPE_UYVY] = AkVideoCaps::Format_uyvy422;
//    guidToStr[MEDIASUBTYPE_IMC1] = AkVideoCaps::Format_none;
//    guidToStr[MEDIASUBTYPE_IMC2] = AkVideoCaps::Format_none;
//    guidToStr[MEDIASUBTYPE_IMC3] = AkVideoCaps::Format_none;
//    guidToStr[MEDIASUBTYPE_IMC4] = AkVideoCaps::Format_none;
    guidToStr[MEDIASUBTYPE_YV12] = AkVideoCaps::Format_yuv420p;
    guidToStr[MEDIASUBTYPE_NV12] = AkVideoCaps::Format_nv12;

#ifdef MEDIASUBTYPE_I420
    guidToStr[MEDIASUBTYPE_I420] = AkVideoCaps::Format_yuv420p;
#endif

//    guidToStr[MEDIASUBTYPE_IF09] = AkVideoCaps::Format_none;
    guidToStr[MEDIASUBTYPE_IYUV] = AkVideoCaps::Format_yuv420p;
//    guidToStr[MEDIASUBTYPE_Y211] = AkVideoCaps::Format_none;
    guidToStr[MEDIASUBTYPE_Y411] = AkVideoCaps::Format_uyyvyy411;
    guidToStr[MEDIASUBTYPE_Y41P] = AkVideoCaps::Format_uyyvyy411;
    guidToStr[MEDIASUBTYPE_YVU9] = AkVideoCaps::Format_yuv410p;
    guidToStr[MEDIASUBTYPE_YVYU] = AkVideoCaps::Format_yvyu422;

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

QString Capture::ioMethod() const
{
    return ioMethodToStr->value(this->m_ioMethod, "any");
}

int Capture::nBuffers() const
{
    return 0;
}

bool Capture::isCompressed() const
{
    return false;
}

AkCaps Capture::caps() const
{
    if (this->m_caps)
        return this->m_caps;

    GraphBuilderPtr graph;
    SampleGrabberPtr grabber;

    AkCaps caps = this->prepare(&graph, &grabber, this->m_device);

    if (caps)
        return caps;

    return AkCaps();
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
                                        (void **) &pPropBag);

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

QVariantList Capture::availableSizes(const QString &webcam) const
{
    MediaTypesList mediaTypes = this->listMediaTypes(webcam);

    if (mediaTypes.isEmpty())
        return QVariantList();

    QVariantList resolutions;

    foreach (MediaTypePtr mediaType, mediaTypes) {
        VIDEOINFOHEADER *pInfoHeader = (VIDEOINFOHEADER *) mediaType->pbFormat;

        QSize size(pInfoHeader->bmiHeader.biWidth,
                   pInfoHeader->bmiHeader.biHeight);

        if (!resolutions.contains(size))
            resolutions << size;
    }

    return resolutions;
}

QSize Capture::size(const QString &webcam) const
{
    if (this->m_resolution.contains(webcam))
        return this->m_resolution[webcam];

    QVariantList resolutions = this->availableSizes(webcam);

    if (resolutions.isEmpty())
        return QSize();

    return resolutions[0].toSize();
}

bool Capture::setSize(const QString &webcam, const QSize &size)
{
    QVariantList resolutions = this->availableSizes(webcam);

    if (!resolutions.contains(size))
        return false;

    this->m_resolution[webcam] = size;
    emit this->sizeChanged(webcam, size);

    return true;
}

bool Capture::resetSize(const QString &webcam)
{
    QVariantList availableSizes = this->availableSizes(webcam);

    if (availableSizes.isEmpty())
        return true;

    return this->setSize(webcam, availableSizes.first().toSize());
}

QVariantList Capture::imageControls(const QString &webcam) const
{
    BaseFilterPtr filter = this->findFilter(webcam);

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

    if (SUCCEEDED(filter->QueryInterface(IID_IAMVideoProcAmp, (void **) &pProcAmp))) {
        foreach (VideoProcAmpProperty property, vpapToStr->keys()) {
            if (SUCCEEDED(pProcAmp->GetRange(property,
                                             (LONG *) &min,
                                             (LONG *) &max,
                                             (LONG *) &step,
                                             (LONG *) &defaultValue,
                                             (LONG *) &flags)))
                if (SUCCEEDED(pProcAmp->Get(property,
                                            (LONG *) &value,
                                            (LONG *) &flags))) {
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

bool Capture::setImageControls(const QString &webcam, const QVariantMap &imageControls) const
{
    BaseFilterPtr filter = this->findFilter(webcam);

    if (!filter)
        return false;

    IAMVideoProcAmp *pProcAmp = NULL;

    if (SUCCEEDED(filter->QueryInterface(IID_IAMVideoProcAmp, (void **) &pProcAmp))) {
        foreach (VideoProcAmpProperty property, vpapToStr->keys()) {
            QString propertyStr = vpapToStr->value(property);

            if (imageControls.contains(propertyStr))
                pProcAmp->Set(property,
                              imageControls[propertyStr].toInt(),
                              VideoProcAmp_Flags_Manual);
        }

        pProcAmp->Release();
    }

    emit this->imageControlsChanged(webcam, imageControls);

    return true;
}

bool Capture::resetImageControls(const QString &webcam) const
{
    QVariantMap controls;

    foreach (QVariant control, this->imageControls(webcam)) {
        QVariantList params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setImageControls(webcam, controls);
}

QVariantList Capture::cameraControls(const QString &webcam) const
{
    BaseFilterPtr filter = this->findFilter(webcam);

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

    if (SUCCEEDED(filter->QueryInterface(IID_IAMCameraControl, (void **) &pCameraControl))) {
        foreach (CameraControlProperty cameraControl, ccToStr->keys()) {
            if (SUCCEEDED(pCameraControl->GetRange(cameraControl,
                                                   (LONG *) &min,
                                                   (LONG *) &max,
                                                   (LONG *) &step,
                                                   (LONG *) &defaultValue,
                                                   (LONG *) &flags)))
                if (SUCCEEDED(pCameraControl->Get(cameraControl,
                                                  (LONG *) &value,
                                                  (LONG *) &flags))) {
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

bool Capture::setCameraControls(const QString &webcam, const QVariantMap &cameraControls) const
{
    return false;

    BaseFilterPtr filter = this->findFilter(webcam);

    if (!filter)
        return false;

    IAMCameraControl *pCameraControl = NULL;

    if (SUCCEEDED(filter->QueryInterface(IID_IAMCameraControl, (void **) &pCameraControl))) {
        foreach (CameraControlProperty cameraControl, ccToStr->keys()) {
            QString cameraControlStr = ccToStr->value(cameraControl);

            if (cameraControls.contains(cameraControlStr))
                pCameraControl->Set(cameraControl,
                                    cameraControls[cameraControlStr].toInt(),
                                    CameraControl_Flags_Manual);
        }

        pCameraControl->Release();
    }

    emit this->cameraControlsChanged(webcam, cameraControls);

    return true;
}

bool Capture::resetCameraControls(const QString &webcam) const
{
    QVariantMap controls;

    foreach (QVariant control, this->cameraControls(webcam)) {
        QVariantList params = control.toList();

        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setCameraControls(webcam, controls);
}

AkPacket Capture::readFrame()
{
    AkPacket packet;

    if (this->m_ioMethod != IoMethodDirectRead) {
        this->m_mutex.lock();

        if (this->m_curBuffer.isEmpty())
            this->m_waitCondition.wait(&this->m_mutex, 1000);

        if (!this->m_curBuffer.isEmpty()) {
            int bufferSize = this->m_curBuffer.size();
            QByteArray oBuffer(bufferSize, Qt::Uninitialized);
            memcpy(oBuffer.data(), this->m_curBuffer.constData(), bufferSize);

            qint64 pts = this->m_curTime
                          * this->m_timeBase.invert().value();

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
        hr = this->m_grabber->GetCurrentBuffer(&bufferSize, (long *) oBuffer.data());

        if (FAILED(hr))
            return AkPacket();

        timeval timestamp;
        gettimeofday(&timestamp, NULL);

        qint64 pts = (timestamp.tv_sec
                      + 1e-6 * timestamp.tv_usec)
                      * this->m_timeBase.invert().value();

        packet = AkPacket(this->m_caps, oBuffer);
        packet.setPts(pts);
        packet.setTimeBase(this->m_timeBase);
        packet.setIndex(0);
        packet.setId(this->m_id);
    }

    return packet;
}

HRESULT Capture::enumerateCameras(IEnumMoniker **ppEnum) const
{
    // Create the System Device Enumerator.
    ICreateDevEnum *pDevEnum = NULL;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_ICreateDevEnum,
                                  (void **) &pDevEnum);

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
                                                 (void **) &pPropBag);

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
                                       (void**) &filter);

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
    if (!filter)
        return MediaTypesList();

    MediaTypesList mediaTypes;
    IEnumPins *pEnumPins = NULL;
    IPin *pin = NULL;

    filter->EnumPins(&pEnumPins);

    while (pEnumPins->Next(1, &pin, NULL) == S_OK) {
        PIN_INFO pInfo;
        pin->QueryPinInfo(&pInfo);

        if (pInfo.dir == PINDIR_OUTPUT) {
            IEnumMediaTypes *pEnum = NULL;
            pin->EnumMediaTypes(&pEnum);

            AM_MEDIA_TYPE *mediaType = NULL;

            while (pEnum->Next(1, &mediaType, NULL) == S_OK)
                if (mediaType->bFixedSizeSamples == TRUE
                    && mediaType->lSampleSize != 0
                    && mediaType->formattype == FORMAT_VideoInfo
                    && mediaType->cbFormat >= sizeof(VIDEOINFOHEADER)
                    && mediaType->pbFormat != NULL
                    && guidToStr->contains(mediaType->subtype)) {
                    mediaTypes << MediaTypePtr(mediaType, this->deleteMediaType);
                } else {
                    this->deleteMediaType(mediaType);
                }

            pEnum->Release();
        }

        if (pInfo.pFilter)
            pInfo.pFilter->Release();

        pin->Release();
        pin = NULL;
    }

    pEnumPins->Release();

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

AkCaps Capture::prepare(GraphBuilderPtr *graph, SampleGrabberPtr *grabber, const QString &webcam) const
{
    // Create the pipeline.
    IGraphBuilder *graphPtr = NULL;

    HRESULT hr = CoCreateInstance(CLSID_FilterGraph,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IGraphBuilder,
                                  (void **) &graphPtr);

    if (FAILED(hr))
        return AkCaps();

    *graph = GraphBuilderPtr(graphPtr, this->deleteUnknown);

    // Create the Sample Grabber filter.
    IBaseFilter *grabberFilter = NULL;

    hr = CoCreateInstance(CLSID_SampleGrabber,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IBaseFilter,
                          (void **) &grabberFilter);

    if (FAILED(hr))
        return AkCaps();

    hr = graphPtr->AddFilter(grabberFilter, L"Grabber");

    if (FAILED(hr))
        return AkCaps();

    ISampleGrabber *grabberPtr = NULL;

    hr = grabberFilter->QueryInterface(IID_ISampleGrabber,
                                       (void **) &grabberPtr);

    if (FAILED(hr))
        return AkCaps();

    *grabber = SampleGrabberPtr(grabberPtr, this->deleteUnknown);
    MediaTypesList mediaTypes = this->listMediaTypes(webcam);

    if (mediaTypes.isEmpty())
        return AkCaps();

    AM_MEDIA_TYPE mediaType;
    ZeroMemory(&mediaType, sizeof(mediaType));

    mediaType.majortype = MEDIATYPE_Video;
    mediaType.subtype = mediaTypes[0]->subtype; //MEDIASUBTYPE_RGB32;

    hr = (*grabber)->SetMediaType(&mediaType);

    if (FAILED(hr))
        return AkCaps();

    // Create the webcam filter.
    IBaseFilter *webcamFilter = this->findFilterP(webcam);
    hr = graphPtr->AddFilter(webcamFilter, L"Source");

    if (FAILED(hr))
        return AkCaps();

    this->changeResolution(webcamFilter, this->size(webcam));

    if (!this->connectFilters(graphPtr, webcamFilter, grabberFilter))
        return AkCaps();

    AM_MEDIA_TYPE connectedMediaType;
    ZeroMemory(&connectedMediaType, sizeof(connectedMediaType));

    hr = (*grabber)->GetConnectedMediaType(&connectedMediaType);

    if (FAILED(hr)
        || connectedMediaType.majortype != MEDIATYPE_Video
        || connectedMediaType.cbFormat < sizeof(VIDEOINFOHEADER)
        || connectedMediaType.pbFormat == NULL)
        return AkCaps();

    VIDEOINFOHEADER *videoInfoHeader = (VIDEOINFOHEADER *) connectedMediaType.pbFormat;

    int fps = 1.0e7 / videoInfoHeader->AvgTimePerFrame;

    AkVideoCaps caps;
    caps.isValid() = true;
    caps.format() = guidToStr->value(connectedMediaType.subtype);
    caps.bpp() = AkVideoCaps::bitsPerPixel(caps.format());
    caps.width() = (int) videoInfoHeader->bmiHeader.biWidth;
    caps.height() = (int) videoInfoHeader->bmiHeader.biHeight;
    caps.fps() = AkFrac(fps, 1);

    return caps.toCaps();
}

PinList Capture::enumPins(IBaseFilter *filter, PIN_DIRECTION direction) const
{
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

void Capture::changeResolution(IBaseFilter *cameraFilter, const QSize &size) const
{
    PinList pins = this->enumPins(cameraFilter, PINDIR_OUTPUT);

    foreach (PinPtr pin, pins) {
        IAMStreamConfig *pStreamConfig = NULL;
        HRESULT hr = pin->QueryInterface(IID_IAMStreamConfig, (void **) &pStreamConfig);

        if (FAILED(hr)) {
            pStreamConfig->Release();

            continue;
        }

        StreamConfigPtr streamConfig(pStreamConfig, this->deleteUnknown);

        int count = 0;
        int configSize = 0;

        hr = streamConfig->GetNumberOfCapabilities(&count, &configSize);

        if (FAILED(hr)
            || configSize != sizeof(VIDEO_STREAM_CONFIG_CAPS))
            continue;

        for (int format = 0; format < count; format++) {
            VIDEO_STREAM_CONFIG_CAPS streamConfigCaps;
            AM_MEDIA_TYPE *pMediaType;

            hr = streamConfig->GetStreamCaps(format, &pMediaType, (BYTE *) &streamConfigCaps);

            if (FAILED(hr)) {
                this->deleteMediaType(pMediaType);

                continue;
            }

            MediaTypePtr mediaType(pMediaType, this->deleteMediaType);

            if (mediaType->majortype != MEDIATYPE_Video
                || mediaType->formattype != FORMAT_VideoInfo
                || mediaType->cbFormat < sizeof (VIDEOINFOHEADER)
                || mediaType->pbFormat == NULL)
                continue;

            VIDEOINFOHEADER *infoHeader = (VIDEOINFOHEADER *) mediaType->pbFormat;

            infoHeader->bmiHeader.biWidth = size.width();
            infoHeader->bmiHeader.biHeight = size.height();

            streamConfig->SetFormat(pMediaType);
        }
    }
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

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) this);

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
        Capture *thisPtr = (Capture *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
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
        CoTaskMemFree((PVOID) mediaType->pbFormat);
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
    AkCaps caps = this->prepare(&this->m_graph, &this->m_grabber, this->m_device);

    if (!caps)
        return false;

    HRESULT hr = this->m_grabber->SetOneShot(FALSE);

    if (FAILED(hr))
        return false;

    hr = this->m_grabber->SetBufferSamples(TRUE);

    if (FAILED(hr))
        return false;

    if (this->m_ioMethod != IoMethodDirectRead) {
        int type = this->m_ioMethod == IoMethodGrabSample? 0: 1;
        hr = this->m_grabber->SetCallback(&this->m_frameGrabber, type);
    }

    if (FAILED(hr))
        return false;

    IMediaControl *control = NULL;

    hr = this->m_graph->QueryInterface(IID_IMediaControl,
                                       (void **) &control);

    if (FAILED(hr))
        return false;

    hr = control->Run();
    control->Release();

    if (FAILED(hr))
        return false;

    this->m_id = Ak::id();
    this->m_caps = caps;
    this->m_timeBase = AkFrac(caps.property("fps").toString()).invert();

    return true;
}

void Capture::uninit()
{
    IMediaControl *control = NULL;

    if (SUCCEEDED(this->m_graph->QueryInterface(IID_IMediaControl,
                                                (void **) &control))) {
        control->StopWhenReady();
        control->Release();
    }

    this->m_grabber.clear();
    this->m_graph.clear();
}

void Capture::setDevice(const QString &device)
{
    if (this->m_device == device)
        return;

    this->m_device = device;
    emit this->deviceChanged(device);
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

void Capture::resetIoMethod()
{
    this->setIoMethod("any");
}

void Capture::resetNBuffers()
{
}

void Capture::reset(const QString &webcam)
{
    QStringList webcams;

    if (webcam.isEmpty())
        webcams = this->webcams();
    else
        webcams << webcam;

    foreach (QString webcam, webcams) {
        this->resetSize(webcam);
        this->resetImageControls(webcam);
        this->resetCameraControls(webcam);
    }
}

void Capture::frameReceived(qreal time, const QByteArray &buffer)
{
    this->m_mutex.lock();
    this->m_curTime = time;
    this->m_curBuffer = buffer;
    this->m_waitCondition.wakeAll();
    this->m_mutex.unlock();
}
