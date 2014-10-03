/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "platform/capturewin.h"

Capture::Capture(): QObject()
{
    this->m_id = -1;
    this->resetIoMethod();

    this->m_propertyToStr[VideoProcAmp_Brightness] = "Brightness";
    this->m_propertyToStr[VideoProcAmp_Contrast] = "Contrast";
    this->m_propertyToStr[VideoProcAmp_Hue] = "Hue";
    this->m_propertyToStr[VideoProcAmp_Saturation] = "Saturation";
    this->m_propertyToStr[VideoProcAmp_Sharpness] = "Sharpness";
    this->m_propertyToStr[VideoProcAmp_Gamma] = "Gamma";
    this->m_propertyToStr[VideoProcAmp_ColorEnable] = "Color Enable";
    this->m_propertyToStr[VideoProcAmp_WhiteBalance] = "White Balance";
    this->m_propertyToStr[VideoProcAmp_BacklightCompensation] = "Backlight Compensation";
    this->m_propertyToStr[VideoProcAmp_Gain] = "Gain";

    this->m_cameraControlToStr[CameraControl_Pan] = "Pan";
    this->m_cameraControlToStr[CameraControl_Tilt] = "Tilt";
    this->m_cameraControlToStr[CameraControl_Roll] = "Roll";
    this->m_cameraControlToStr[CameraControl_Zoom] = "Zoom";
    this->m_cameraControlToStr[CameraControl_Exposure] = "Exposure";
    this->m_cameraControlToStr[CameraControl_Iris] = "Iris";
    this->m_cameraControlToStr[CameraControl_Focus] = "Focus";

    this->m_propertyLst = this->m_propertyToStr.keys();
    this->m_cameraControlLst = this->m_cameraControlToStr.keys();

    this->m_guidToStr[MEDIASUBTYPE_RGB1] = "monob";
    this->m_guidToStr[MEDIASUBTYPE_RGB4] = "bgr4_byte";
    this->m_guidToStr[MEDIASUBTYPE_RGB8] = "pal8";
    this->m_guidToStr[MEDIASUBTYPE_RGB555] = "bgr555le";
    this->m_guidToStr[MEDIASUBTYPE_RGB565] = "bgr565le";
    this->m_guidToStr[MEDIASUBTYPE_RGB24] = "bgr24";
    this->m_guidToStr[MEDIASUBTYPE_RGB32] = "bgr0";
//    this->m_guidToStr[MEDIASUBTYPE_ARGB1555] = "";
    this->m_guidToStr[MEDIASUBTYPE_ARGB32] = "bgra";
//    this->m_guidToStr[MEDIASUBTYPE_ARGB4444] = "";
//    this->m_guidToStr[MEDIASUBTYPE_A2R10G10B10] = "";
//    this->m_guidToStr[MEDIASUBTYPE_A2B10G10R10] = "";
    this->m_guidToStr[MEDIASUBTYPE_AYUV] = "yuva420p";
    this->m_guidToStr[MEDIASUBTYPE_YUY2] = "yuyv422";
    this->m_guidToStr[MEDIASUBTYPE_UYVY] = "uyvy422";
//    this->m_guidToStr[MEDIASUBTYPE_IMC1] = "";
//    this->m_guidToStr[MEDIASUBTYPE_IMC2] = "";
//    this->m_guidToStr[MEDIASUBTYPE_IMC3] = "";
//    this->m_guidToStr[MEDIASUBTYPE_IMC4] = "";
    this->m_guidToStr[MEDIASUBTYPE_YV12] = "yuv420p";
    this->m_guidToStr[MEDIASUBTYPE_NV12] = "nv12";
    this->m_guidToStr[MEDIASUBTYPE_I420] = "yuv420p";
//    this->m_guidToStr[MEDIASUBTYPE_IF09] = "";
    this->m_guidToStr[MEDIASUBTYPE_IYUV] = "yuv420p";
//    this->m_guidToStr[MEDIASUBTYPE_Y211] = "";
    this->m_guidToStr[MEDIASUBTYPE_Y411] = "uyyvyy411";
    this->m_guidToStr[MEDIASUBTYPE_Y41P] = "uyyvyy411";
    this->m_guidToStr[MEDIASUBTYPE_YVU9] = "yuv410p";
    this->m_guidToStr[MEDIASUBTYPE_YVYU] = "yvyu422";

    QObject::connect(&this->m_frameGrabber,
                     SIGNAL(frameReady(float, const QByteArray &)),
                     this,
                     SLOT(frameReceived(float, const QByteArray &)),
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
    if (this->m_ioMethod == IoMethodDirectRead)
        return "directRead";
    else if (this->m_ioMethod == IoMethodGrabSample)
        return "grabSample";
    else if (this->m_ioMethod == IoMethodGrabBuffer)
        return "grabBuffer";

    return "any";
}

int Capture::nBuffers() const
{
    return 0;
}

bool Capture::isCompressed() const
{
    return false;
}

QString Capture::caps() const
{
    if (this->m_caps)
        return this->m_caps.toString();

    GraphBuilderPtr graph;
    SampleGrabberPtr grabber;

    QbCaps caps = this->prepare(&graph, &grabber, this->m_device);

    if (caps)
        return caps.toString();

    return "";
}

QString Capture::description(const QString &webcam) const
{
    MonikerPtr moniker = this->findMoniker(webcam);

    if (!moniker)
        return QString();

    IPropertyBag *pPropBag = NULL;
    HRESULT hr = moniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));

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
    return this->setSize(webcam, this->availableSizes(webcam)[0].toSize());
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

    if (SUCCEEDED(filter->QueryInterface(IID_IAMVideoProcAmp, (void **) &pProcAmp)))
        foreach (VideoProcAmpProperty property, this->m_propertyLst)
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

                    control << this->m_propertyToStr[property]
                            << type
                            << min
                            << max
                            << step
                            << defaultValue
                            << value
                            << QStringList();

                    controls << QVariant(control);
                }

    return controls;
}

bool Capture::setImageControls(const QString &webcam, const QVariantMap &imageControls) const
{
    BaseFilterPtr filter = this->findFilter(webcam);

    if (!filter)
        return false;

    IAMVideoProcAmp *pProcAmp = NULL;

    if (SUCCEEDED(filter->QueryInterface(IID_IAMVideoProcAmp, (void **) &pProcAmp)))
        foreach (VideoProcAmpProperty property, this->m_propertyLst) {
            QString propertyStr = this->m_propertyToStr[property];

            if (imageControls.contains(propertyStr))
                pProcAmp->Set(property,
                              imageControls[propertyStr].toInt(),
                              VideoProcAmp_Flags_Manual);
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

    if (SUCCEEDED(filter->QueryInterface(IID_IAMCameraControl, (void **) &pCameraControl)))
        foreach (CameraControlProperty cameraControl, this->m_cameraControlLst)
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

                    control << this->m_cameraControlToStr[cameraControl]
                            << QString("integer")
                            << min
                            << max
                            << step
                            << defaultValue
                            << value
                            << QStringList();

                    controls << QVariant(control);
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

    if (SUCCEEDED(filter->QueryInterface(IID_IAMCameraControl, (void **) &pCameraControl)))
        foreach (CameraControlProperty cameraControl, this->m_cameraControlLst) {
            QString cameraControlStr = this->m_cameraControlToStr[cameraControl];

            if (cameraControls.contains(cameraControlStr))
                pCameraControl->Set(cameraControl,
                                    cameraControls[cameraControlStr].toInt(),
                                    CameraControl_Flags_Manual);
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

QbPacket Capture::readFrame()
{
    QbPacket packet;

    if (this->m_ioMethod != IoMethodDirectRead) {
        this->m_mutex.lock();

        if (this->m_curBuffer.isEmpty())
            this->m_waitCondition.wait(&this->m_mutex, 1000);

        if (!this->m_curBuffer.isEmpty()) {
            int bufferSize = this->m_curBuffer.size();
            QSharedPointer<char> oBuffer(new char[bufferSize]);
            memcpy(oBuffer.data(), this->m_curBuffer.data(), bufferSize);

            qint64 pts = this->m_curTime
                          * this->m_timeBase.invert().value();

            QbPacket oPacket(this->m_caps,
                             oBuffer,
                             bufferSize);

            oPacket.setPts(pts);
            oPacket.setTimeBase(this->m_timeBase);
            oPacket.setIndex(0);
            oPacket.setId(this->m_id);

            packet = oPacket;
            this->m_curBuffer.clear();
        }

        this->m_mutex.unlock();
    }
    else {
        long bufferSize;

        HRESULT hr = this->m_grabber->GetCurrentBuffer(&bufferSize, NULL);

        if (FAILED(hr))
            return QbPacket();

        QSharedPointer<char> oBuffer(new char[bufferSize]);

        if (!oBuffer)
            return QbPacket();

        hr = this->m_grabber->GetCurrentBuffer(&bufferSize, (long *) oBuffer.data());

        if (FAILED(hr))
            return QbPacket();

        timeval timestamp;
        gettimeofday(&timestamp, NULL);

        qint64 pts = (timestamp.tv_sec
                      + 1e-6 * timestamp.tv_usec)
                      * this->m_timeBase.invert().value();

        QbPacket oPacket(this->m_caps,
                         oBuffer,
                         bufferSize);

        oPacket.setPts(pts);
        oPacket.setTimeBase(this->m_timeBase);
        oPacket.setIndex(0);
        oPacket.setId(this->m_id);

        return oPacket;
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
                                  IID_PPV_ARGS(&pDevEnum));

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
            HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));

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
                    && this->m_guidToStr.contains(mediaType->subtype))
                    mediaTypes << MediaTypePtr(mediaType, this->deleteMediaType);

            pEnum->Release();
        }

        pin->Release();
    }

    pEnumPins->Release();

    return mediaTypes;
}

HRESULT Capture::isPinConnected(IPin *pPin, WINBOOL *pResult) const
{
    IPin *pTmp = NULL;
    HRESULT hr = pPin->ConnectedTo(&pTmp);

    if (SUCCEEDED(hr))
        *pResult = TRUE;
    else if (hr == VFW_E_NOT_CONNECTED) {
        // The pin is not connected. This is not an error for our purposes.
        *pResult = FALSE;
        hr = S_OK;
    }

    this->safeRelease(&pTmp);

    return hr;
}

HRESULT Capture::isPinDirection(IPin *pPin, PIN_DIRECTION dir, WINBOOL *pResult) const
{
    PIN_DIRECTION pinDir;
    HRESULT hr = pPin->QueryDirection(&pinDir);

    if (SUCCEEDED(hr))
        *pResult = (pinDir == dir);

    return hr;
}

HRESULT Capture::matchPin(IPin *pPin, PIN_DIRECTION direction, WINBOOL bShouldBeConnected, WINBOOL *pResult) const
{
    BOOL bMatch = FALSE;
    BOOL bIsConnected = FALSE;

    HRESULT hr = this->isPinConnected(pPin, &bIsConnected);

    if (SUCCEEDED(hr)
        && bIsConnected == bShouldBeConnected)
        hr = this->isPinDirection(pPin, direction, &bMatch);

    if (SUCCEEDED(hr) && pResult)
        *pResult = bMatch;

    return hr;
}

HRESULT Capture::findUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin) const
{
    IEnumPins *pEnum = NULL;
    IPin *pPin = NULL;
    BOOL bFound = FALSE;

    HRESULT hr = pFilter->EnumPins(&pEnum);

    if (FAILED(hr)) {
        this->safeRelease(&pPin);
        this->safeRelease(&pEnum);

        return hr;
    }

    while (S_OK == pEnum->Next(1, &pPin, NULL)) {
        hr = this->matchPin(pPin, PinDir, FALSE, &bFound);

        if (FAILED(hr)) {
            this->safeRelease(&pPin);
            this->safeRelease(&pEnum);

            return hr;
        }

        if (bFound) {
            *ppPin = pPin;
            (*ppPin)->AddRef();

            break;
        }

        this->safeRelease(&pPin);
    }

    if (!bFound)
        hr = VFW_E_NOT_FOUND;

    return hr;
}

HRESULT Capture::connectFilters(IGraphBuilder *pGraph, IPin *pOut, IBaseFilter *pDest) const
{
    IPin *pIn = NULL;

    // Find an input pin on the downstream filter.
    HRESULT hr = this->findUnconnectedPin(pDest, PINDIR_INPUT, &pIn);

    if (SUCCEEDED(hr)) {
        // Try to connect them.
        hr = pGraph->Connect(pOut, pIn);
        pIn->Release();
    }

    return hr;
}

HRESULT Capture::connectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IPin *pIn) const
{
    IPin *pOut = NULL;

    // Find an output pin on the upstream filter.
    HRESULT hr = this->findUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);

    if (SUCCEEDED(hr)) {
        // Try to connect them.
        hr = pGraph->Connect(pOut, pIn);
        pOut->Release();
    }

    return hr;
}

HRESULT Capture::connectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IBaseFilter *pDest) const
{
    IPin *pOut = NULL;

    // Find an output pin on the first filter.
    HRESULT hr = this->findUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);

    if (SUCCEEDED(hr)) {
        hr = this->connectFilters(pGraph, pOut, pDest);
        pOut->Release();
    }

    return hr;
}

QbCaps Capture::prepare(GraphBuilderPtr *graph, SampleGrabberPtr *grabber, const QString &webcam) const
{
    IGraphBuilder *graphPtr = NULL;

    HRESULT hr = CoCreateInstance(CLSID_FilterGraph,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&graphPtr));

    if (FAILED(hr))
        return QbCaps();

    *graph = GraphBuilderPtr(graphPtr, this->deleteUnknown);

    // Create the Sample Grabber filter.
    IBaseFilter *grabberFilter = NULL;

    hr = CoCreateInstance(CLSID_SampleGrabber,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&grabberFilter));

    if (FAILED(hr))
        return QbCaps();

    hr = (*graph)->AddFilter(grabberFilter, L"Grabber");

    if (FAILED(hr))
        return QbCaps();

    ISampleGrabber *grabberPtr = NULL;

    hr = grabberFilter->QueryInterface(IID_PPV_ARGS(&grabberPtr));

    if (FAILED(hr))
        return QbCaps();

    *grabber = SampleGrabberPtr(grabberPtr, this->deleteUnknown);

    MediaTypesList mediaTypes = this->listMediaTypes(webcam);

    if (mediaTypes.isEmpty())
        return QbCaps();

    AM_MEDIA_TYPE mediaType;
    ZeroMemory(&mediaType, sizeof(mediaType));

    mediaType.majortype = MEDIATYPE_Video;
    mediaType.subtype = mediaTypes[0]->subtype;

    hr = (*grabber)->SetMediaType(&mediaType);

    if (FAILED(hr))
        return QbCaps();

    IBaseFilter *webcamFilter = this->findFilterP(webcam);

    hr = (*graph)->AddFilter(webcamFilter, L"Source");

    if (FAILED(hr))
        return QbCaps();

    this->changeResolution(webcamFilter, this->size(webcam));

    IEnumPins *pEnum = NULL;

    hr = webcamFilter->EnumPins(&pEnum);

    if (FAILED(hr))
        return QbCaps();

    IPin *pPin = NULL;

    while (pEnum->Next(1, &pPin, NULL) == S_OK) {
        hr = this->connectFilters((*graph).data(), pPin, grabberFilter);
        this->safeRelease(&pPin);

        if (SUCCEEDED(hr))
            break;
    }

    if (FAILED(hr))
        return QbCaps();

    IBaseFilter *nullFilter = NULL;

    hr = CoCreateInstance(CLSID_NullRenderer,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&nullFilter));

    if (FAILED(hr))
        return QbCaps();

    hr = (*graph)->AddFilter(nullFilter, L"NullFilter");

    if (FAILED(hr))
        return QbCaps();

    hr = this->connectFilters((*graph).data(),
                              grabberFilter,
                              nullFilter);

    if (FAILED(hr))
        return QbCaps();

    AM_MEDIA_TYPE connectedMediaType;
    ZeroMemory(&connectedMediaType, sizeof(connectedMediaType));

    hr = (*grabber)->GetConnectedMediaType(&connectedMediaType);

    if (FAILED(hr)
        || connectedMediaType.majortype != MEDIATYPE_Video
        || connectedMediaType.cbFormat < sizeof(VIDEOINFOHEADER)
        || connectedMediaType.pbFormat == NULL)
        return QbCaps();

    VIDEOINFOHEADER *videoInfoHeader = (VIDEOINFOHEADER *) connectedMediaType.pbFormat;

    int fps = 1.0e7 / videoInfoHeader->AvgTimePerFrame;

    QbCaps caps;
    caps.setMimeType("video/x-raw");
    caps.setProperty("format", this->m_guidToStr[connectedMediaType.subtype]);
    caps.setProperty("width", (int) videoInfoHeader->bmiHeader.biWidth);
    caps.setProperty("height", (int) videoInfoHeader->bmiHeader.biHeight);
    caps.setProperty("fps", QString("%1/1").arg(fps));

    return caps;
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

            this->safeRelease(&pin);
        }
    }

    this->safeRelease(&enumPins);

    return pinList;
}

void Capture::changeResolution(IBaseFilter *cameraFilter, const QSize &size) const
{
    PinList pins = this->enumPins(cameraFilter, PINDIR_OUTPUT);

    foreach (PinPtr pin, pins) {
        IAMStreamConfig *pStreamConfig = NULL;
        HRESULT hr = pin->QueryInterface(IID_IAMStreamConfig, (void **) &pStreamConfig);

        if (FAILED(hr)) {
            this->safeRelease(&pStreamConfig);

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

    if (FAILED(result))
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

bool Capture::init()
{
    QbCaps caps = this->prepare(&this->m_graph, &this->m_grabber, this->m_device);

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

    hr = this->m_graph->QueryInterface(IID_PPV_ARGS(&control));

    if (FAILED(hr))
        return false;

    this->m_control = MediaControlPtr(control, this->deleteUnknown);

    hr = this->m_control->Run();

    if (FAILED(hr))
        return false;

    this->m_id = Qb::id();
    this->m_caps = caps;
    this->m_timeBase = QbFrac(caps.property("fps").toString()).invert();

    return true;
}

void Capture::uninit()
{
    if (this->m_control)
        this->m_control->Stop();

    this->m_grabber.clear();
    this->m_control.clear();
    this->m_graph.clear();
}

void Capture::setDevice(const QString &device)
{
    this->m_device = device;
}

void Capture::setIoMethod(const QString &ioMethod)
{
    if (ioMethod == "directRead")
        this->m_ioMethod = IoMethodDirectRead;
    else if (ioMethod == "grabBuffer")
        this->m_ioMethod = IoMethodGrabBuffer;
    else
        this->m_ioMethod = IoMethodGrabSample;
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
    }
}

void Capture::frameReceived(float time, const QByteArray &buffer)
{
    this->m_mutex.lock();
    this->m_curTime = time;
    this->m_curBuffer = buffer;
    this->m_waitCondition.wakeAll();
    this->m_mutex.unlock();
}
