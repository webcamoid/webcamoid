/* Webcamoid, webcam capture application.
 * Copyright (C) 2018  Gonzalo Exequiel Pedone
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

#include <algorithm>
#include <atomic>
#include <functional>
#include <limits>
#include <mutex>
#include <sstream>
#include <thread>
#include <dshow.h>

#include "pin.h"
#include "basefilter.h"
#include "enummediatypes.h"
#include "memallocator.h"
#include "propertyset.h"
#include "pushsource.h"
#include "qualitycontrol.h"
#include "referenceclock.h"
#include "videoprocamp.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/image/videoframe.h"
#include "VCamUtils/src/utils.h"

#define AK_CUR_INTERFACE "Pin"

namespace AkVCam
{
    class PinPrivate
    {
        public:
            Pin *self;
            BaseFilter *m_baseFilter;
            VideoProcAmp *m_videoProcAmp;
            std::wstring m_pinName;
            std::wstring m_pinId;
            EnumMediaTypes *m_mediaTypes;
            IPin *m_connectedTo;
            IMemInputPin *m_memInputPin;
            IMemAllocator *m_memAllocator;
            REFERENCE_TIME m_pts;
            REFERENCE_TIME m_ptsDrift;
            REFERENCE_TIME m_start;
            REFERENCE_TIME m_stop;
            double m_rate;
            FILTER_STATE m_prevState;
            DWORD_PTR m_adviseCookie;
            HANDLE m_sendFrameEvent;
            std::thread m_sendFrameThread;
            std::atomic<bool> m_running;
            std::mutex m_mutex;
            VideoFrame m_currentFrame;
            VideoFrame m_testFrame;
            VideoFrame m_testFrameAdapted;
            std::string m_broadcaster;
            bool m_horizontalFlip;   // Controlled by client
            bool m_verticalFlip;
            bool m_horizontalMirror; // Controlled by server
            bool m_verticalMirror;
            Scaling m_scaling;
            AspectRatio m_aspectRatio;
            bool m_swapRgb;
            LONG m_brightness;
            LONG m_contrast;
            LONG m_saturation;
            LONG m_gamma;
            LONG m_hue;
            LONG m_colorenable;

            void sendFrameOneShot();
            void sendFrameLoop();
            HRESULT sendFrame();
            void updateTestFrame();
            VideoFrame applyAdjusts(const VideoFrame &frame);
            static void propertyChanged(void *userData,
                                        LONG Property,
                                        LONG lValue,
                                        LONG Flags);
    };
}

AkVCam::Pin::Pin(BaseFilter *baseFilter,
                 const std::vector<VideoFormat> &formats,
                 const std::wstring &pinName):
    StreamConfig(this)
{
    this->setParent(this, &IID_IPin);

    this->d = new PinPrivate;
    this->d->self = this;
    this->d->m_baseFilter = baseFilter;
    this->d->m_pinName = pinName;
    std::wstringstream wss;
    wss << L"pin(" << this << L")";
    this->d->m_pinId = wss.str();
    this->d->m_mediaTypes = new AkVCam::EnumMediaTypes(formats);
    this->d->m_mediaTypes->AddRef();
    this->d->m_connectedTo = nullptr;
    this->d->m_memInputPin = nullptr;
    this->d->m_memAllocator = nullptr;
    this->d->m_ptsDrift = -1;
    this->d->m_ptsDrift = 0;
    this->d->m_start = 0;
    this->d->m_stop = MAXLONGLONG;
    this->d->m_rate = 1.0;
    this->d->m_horizontalFlip = false;
    this->d->m_verticalFlip = false;
    this->d->m_prevState = State_Stopped;
    this->d->m_adviseCookie = 0;
    this->d->m_sendFrameEvent = nullptr;
    this->d->m_running = false;
    this->d->m_horizontalMirror = false;
    this->d->m_verticalMirror = false;
    this->d->m_scaling = ScalingFast;
    this->d->m_aspectRatio = AspectRatioIgnore;
    this->d->m_swapRgb = false;
    auto bmp = programFilesPath()
             + L"\\" DSHOW_PLUGIN_NAME_L L".plugin\\share\\TestFrame.bmp";

    if (!this->d->m_testFrame.load(std::string(bmp.begin(), bmp.end()))) {
        this->d->m_testFrame.load(":/VirtualCamera/share/TestFrame/TestFrame.bmp");
    }

    baseFilter->QueryInterface(IID_IAMVideoProcAmp,
                               reinterpret_cast<void **>(&this->d->m_videoProcAmp));

    LONG flags = 0;
    this->d->m_videoProcAmp->Get(VideoProcAmp_Brightness,
                                 &this->d->m_brightness,
                                 &flags);
    this->d->m_videoProcAmp->Get(VideoProcAmp_Contrast,
                                 &this->d->m_contrast, &flags);
    this->d->m_videoProcAmp->Get(VideoProcAmp_Saturation,
                                 &this->d->m_saturation,
                                 &flags);
    this->d->m_videoProcAmp->Get(VideoProcAmp_Gamma,
                                 &this->d->m_gamma,
                                 &flags);
    this->d->m_videoProcAmp->Get(VideoProcAmp_Hue,
                                 &this->d->m_hue,
                                 &flags);
    this->d->m_videoProcAmp->Get(VideoProcAmp_ColorEnable,
                                 &this->d->m_colorenable,
                                 &flags);

    this->d->m_videoProcAmp->connectPropertyChanged(this->d,
                                                    &PinPrivate::propertyChanged);
}

AkVCam::Pin::~Pin()
{
    this->d->m_mediaTypes->Release();

    if (this->d->m_connectedTo)
        this->d->m_connectedTo->Release();

    if (this->d->m_memInputPin)
        this->d->m_memInputPin->Release();

    if (this->d->m_memAllocator)
        this->d->m_memAllocator->Release();

    if (this->d->m_videoProcAmp)
        this->d->m_videoProcAmp->Release();

    delete this->d;
}

AkVCam::BaseFilter *AkVCam::Pin::baseFilter() const
{
    AkLogMethod();

    return this->d->m_baseFilter;
}

void AkVCam::Pin::setBaseFilter(BaseFilter *baseFilter)
{
    AkLogMethod();
    this->d->m_baseFilter = baseFilter;
}

HRESULT AkVCam::Pin::stateChanged(void *userData, FILTER_STATE state)
{
    auto self = reinterpret_cast<Pin *>(userData);
    AkLoggerLog(AK_CUR_INTERFACE, "(", self, ")::", __FUNCTION__, "()");
    AkLoggerLog("Old state: ", self->d->m_prevState);
    AkLoggerLog("New state: ", state);

    if (state == self->d->m_prevState)
        return S_OK;

    if (self->d->m_prevState == State_Stopped) {
        if (FAILED(self->d->m_memAllocator->Commit()))
            return VFW_E_NOT_COMMITTED;

        self->d->updateTestFrame();
        self->d->m_currentFrame = self->d->m_testFrameAdapted;
        self->d->m_pts = -1;
        self->d->m_ptsDrift = 0;

        self->d->m_sendFrameEvent =
                CreateSemaphore(nullptr, 1, 1, L"SendFrame");

        self->d->m_running = true;
        self->d->m_sendFrameThread =
                std::thread(&PinPrivate::sendFrameLoop, self->d);
        AkLoggerLog("Launching thread ", self->d->m_sendFrameThread.get_id());

        auto clock = self->d->m_baseFilter->referenceClock();
        REFERENCE_TIME now = 0;
        clock->GetTime(&now);

        AM_MEDIA_TYPE *mediaType = nullptr;
        self->GetFormat(&mediaType);
        auto videoFormat = formatFromMediaType(mediaType);
        deleteMediaType(&mediaType);
        auto fps = videoFormat.minimumFrameRate();
        auto period = REFERENCE_TIME(TIME_BASE
                                     * fps.den()
                                     / fps.num());

        clock->AdvisePeriodic(now,
                              period,
                              HSEMAPHORE(self->d->m_sendFrameEvent),
                              &self->d->m_adviseCookie);
    } else if (state == State_Stopped) {
        self->d->m_running = false;
        self->d->m_sendFrameThread.join();
        auto clock = self->d->m_baseFilter->referenceClock();
        clock->Unadvise(self->d->m_adviseCookie);
        self->d->m_adviseCookie = 0;
        CloseHandle(self->d->m_sendFrameEvent);
        self->d->m_sendFrameEvent = nullptr;
        self->d->m_memAllocator->Decommit();
        self->d->m_currentFrame.clear();
        self->d->m_testFrameAdapted.clear();
    }

    self->d->m_prevState = state;

    return S_OK;
}

void AkVCam::Pin::serverStateChanged(IpcBridge::ServerState state)
{
    AkLogMethod();

    if (state == IpcBridge::ServerStateGone) {
        this->d->m_broadcaster.clear();
        this->d->m_horizontalMirror = false;
        this->d->m_verticalMirror = false;
        this->d->m_scaling = ScalingFast;
        this->d->m_aspectRatio = AspectRatioIgnore;
        this->d->m_swapRgb = false;
        this->d->updateTestFrame();

        this->d->m_mutex.lock();
        this->d->m_currentFrame = this->d->m_testFrameAdapted;
        this->d->m_mutex.unlock();
    }
}

void AkVCam::Pin::frameReady(const VideoFrame &frame)
{
    AkLogMethod();
    AkLoggerLog("Running: ", this->d->m_running);
    AkLoggerLog("Broadcaster: ", this->d->m_broadcaster);

    if (!this->d->m_running)
        return;

    this->d->m_mutex.lock();

    if (!this->d->m_broadcaster.empty()) {
        auto frameAdjusted = this->d->applyAdjusts(frame);

        if (frameAdjusted.format().size() > 0)
            this->d->m_currentFrame = frameAdjusted;
    }

    this->d->m_mutex.unlock();
}

void AkVCam::Pin::setBroadcasting(const std::string &broadcaster)
{
    AkLogMethod();
    AkLoggerLog("Broadcaster: ", broadcaster);

    if (this->d->m_broadcaster == broadcaster)
        return;

    this->d->m_mutex.lock();
    this->d->m_broadcaster = broadcaster;

    if (broadcaster.empty())
        this->d->m_currentFrame = this->d->m_testFrameAdapted;

    this->d->m_mutex.unlock();
}

void AkVCam::Pin::setMirror(bool horizontalMirror, bool verticalMirror)
{
    AkLogMethod();

    if (this->d->m_horizontalMirror == horizontalMirror
        && this->d->m_verticalMirror == verticalMirror)
        return;

    this->d->m_horizontalMirror = horizontalMirror;
    this->d->m_verticalMirror = verticalMirror;
    this->d->updateTestFrame();
}

void AkVCam::Pin::setScaling(Scaling scaling)
{
    AkLogMethod();

    if (this->d->m_scaling == scaling)
        return;

    this->d->m_scaling = scaling;
    this->d->updateTestFrame();
}

void AkVCam::Pin::setAspectRatio(AspectRatio aspectRatio)
{
    AkLogMethod();

    if (this->d->m_aspectRatio == aspectRatio)
        return;

    this->d->m_aspectRatio = aspectRatio;
    this->d->updateTestFrame();
}

void AkVCam::Pin::setSwapRgb(bool swap)
{
    AkLogMethod();

    if (this->d->m_swapRgb == swap)
        return;

    this->d->m_swapRgb = swap;
    this->d->updateTestFrame();
}

bool AkVCam::Pin::horizontalFlip() const
{
    return this->d->m_horizontalFlip;
}

void AkVCam::Pin::setHorizontalFlip(bool flip)
{
    this->d->m_horizontalFlip = flip;
}

bool AkVCam::Pin::verticalFlip() const
{
    return this->d->m_verticalFlip;
}

void AkVCam::Pin::setVerticalFlip(bool flip)
{
    this->d->m_verticalFlip = flip;
}

HRESULT AkVCam::Pin::QueryInterface(const IID &riid, void **ppvObject)
{
    AkLogMethod();
    AkLoggerLog("IID: ", AkVCam::stringFromClsid(riid));

    if (!ppvObject)
        return E_POINTER;

    *ppvObject = nullptr;

    if (IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IPin)) {
        AkLogInterface(IPin, this);
        this->AddRef();
        *ppvObject = this;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IAMStreamConfig)) {
        auto streamConfig = static_cast<IAMStreamConfig *>(this);
        AkLogInterface(IAMStreamConfig, streamConfig);
        streamConfig->AddRef();
        *ppvObject = streamConfig;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IAMPushSource)) {
        auto pushSource = new PushSource(this);
        AkLogInterface(IAMPushSource, pushSource);
        pushSource->AddRef();
        *ppvObject = pushSource;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IKsPropertySet)) {
        auto propertySet = new PropertySet();
        AkLogInterface(IKsPropertySet, propertySet);
        propertySet->AddRef();
        *ppvObject = propertySet;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IQualityControl)) {
        auto qualityControl = new QualityControl();
        AkLogInterface(IQualityControl, qualityControl);
        qualityControl->AddRef();
        *ppvObject = qualityControl;

        return S_OK;
    }

    return CUnknown::QueryInterface(riid, ppvObject);
}

HRESULT AkVCam::Pin::Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    AkLogMethod();
    AkLoggerLog("Receive pin: ", pReceivePin);
    AkLoggerLog("Media type: ", stringFromMediaType(pmt));

    if (!pReceivePin)
        return E_POINTER;

    if (this->d->m_connectedTo)
        return VFW_E_ALREADY_CONNECTED;

    if (this->d->m_baseFilter) {
        FILTER_STATE state;

        if (SUCCEEDED(this->d->m_baseFilter->GetState(0, &state))
            && state != State_Stopped)
            return VFW_E_NOT_STOPPED;
    }

    PIN_DIRECTION direction = PINDIR_OUTPUT;

    // Only connect to an input pin.
    if (FAILED(pReceivePin->QueryDirection(&direction))
        || direction != PINDIR_INPUT)
        return VFW_E_NO_TRANSPORT;

    /* When the Filter Graph Manager calls Connect, the output pin must request
     * a IMemInputPin and get a IMemAllocator interface to the input pin.
     */
    IMemInputPin *memInputPin = nullptr;

    if (FAILED(pReceivePin->QueryInterface(IID_IMemInputPin,
                                           reinterpret_cast<void **>(&memInputPin)))) {
        return VFW_E_NO_TRANSPORT;
    }

    AM_MEDIA_TYPE *mediaType = nullptr;

    if (pmt) {
        // Try setting requested media type.
        if (!containsMediaType(pmt, this->d->m_mediaTypes))
            return VFW_E_TYPE_NOT_ACCEPTED;

        mediaType = createMediaType(pmt);
    } else {
        // Test currently set media type.
        AM_MEDIA_TYPE *mt = nullptr;

        if (SUCCEEDED(this->GetFormat(&mt)) && mt) {
            if (pReceivePin->QueryAccept(mt) == S_OK)
                mediaType = mt;
            else
                deleteMediaType(&mt);
        }

        if (!mediaType) {
            // Test media types supported by the input pin.
            AM_MEDIA_TYPE *mt = nullptr;
            IEnumMediaTypes *mediaTypes = nullptr;

            if (SUCCEEDED(pReceivePin->EnumMediaTypes(&mediaTypes))) {
                mediaTypes->Reset();

                while (mediaTypes->Next(1, &mt, nullptr) == S_OK) {
                    AkLoggerLog("Testing media type: ", stringFromMediaType(mt));

                    // If the mediatype match our suported mediatypes...
                    if (this->QueryAccept(mt) == S_OK) {
                        // set it.
                        mediaType = mt;

                        break;
                    }

                    deleteMediaType(&mt);
                }

                mediaTypes->Release();
            }
        }

        if (!mediaType) {
            /* If none of the input media types was suitable for us, ask to
             * input pin if it at least supports one of us.
             */
            AM_MEDIA_TYPE *mt = nullptr;
            this->d->m_mediaTypes->Reset();

            while (this->d->m_mediaTypes->Next(1, &mt, nullptr) == S_OK) {
                if (pReceivePin->QueryAccept(mt) == S_OK) {
                    mediaType = mt;

                    break;
                }

                deleteMediaType(&mt);
            }
        }
    }

    if (!mediaType)
        return VFW_E_NO_ACCEPTABLE_TYPES;

    AkLoggerLog("Setting Media Type: ", stringFromMediaType(mediaType));
    auto result = pReceivePin->ReceiveConnection(this, mediaType);

    if (FAILED(result)) {
        deleteMediaType(&mediaType);

        return result;
    }

    AkLoggerLog("Connection accepted by input pin");

    // Define memory allocator requirements.
    ALLOCATOR_PROPERTIES allocatorRequirements;
    memset(&allocatorRequirements, 0, sizeof(ALLOCATOR_PROPERTIES));
    memInputPin->GetAllocatorRequirements(&allocatorRequirements);
    auto videoFormat = formatFromMediaType(mediaType);

    if (allocatorRequirements.cBuffers < 1)
        allocatorRequirements.cBuffers = 1;

    allocatorRequirements.cbBuffer = LONG(videoFormat.size());

    if (allocatorRequirements.cbAlign < 1)
        allocatorRequirements.cbAlign = 1;

    // Get a memory allocator.
    IMemAllocator *memAllocator = nullptr;

    // if it fail use our own.
    if (FAILED(memInputPin->GetAllocator(&memAllocator))) {
        memAllocator = new MemAllocator;
        memAllocator->AddRef();
    }

    ALLOCATOR_PROPERTIES actualRequirements;
    memset(&actualRequirements, 0, sizeof(ALLOCATOR_PROPERTIES));

    if (FAILED(memAllocator->SetProperties(&allocatorRequirements,
                                           &actualRequirements))) {
        memAllocator->Release();
        memInputPin->Release();
        deleteMediaType(&mediaType);

        return VFW_E_NO_TRANSPORT;
    }

    if (FAILED(memInputPin->NotifyAllocator(memAllocator, S_OK))) {
        memAllocator->Release();
        memInputPin->Release();
        deleteMediaType(&mediaType);

        return VFW_E_NO_TRANSPORT;
    }

    if (this->d->m_memInputPin)
        this->d->m_memInputPin->Release();

    this->d->m_memInputPin = memInputPin;

    if (this->d->m_memAllocator)
        this->d->m_memAllocator->Release();

    this->d->m_memAllocator = memAllocator;
    this->SetFormat(mediaType);

    if (this->d->m_connectedTo)
        this->d->m_connectedTo->Release();

    this->d->m_connectedTo = pReceivePin;
    this->d->m_connectedTo->AddRef();
    this->d->m_baseFilter->connectStateChanged(this, &Pin::stateChanged);
    AkLoggerLog("Connected to ", pReceivePin);

    return S_OK;
}

HRESULT AkVCam::Pin::ReceiveConnection(IPin *pConnector,
                                       const AM_MEDIA_TYPE *pmt)
{
    UNUSED(pConnector)
    UNUSED(pmt)
    AkLogMethod();

    return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT AkVCam::Pin::Disconnect()
{
    AkLogMethod();
    this->d->m_baseFilter->disconnectStateChanged(this, &Pin::stateChanged);

    if (this->d->m_baseFilter) {
        FILTER_STATE state;

        if (SUCCEEDED(this->d->m_baseFilter->GetState(0, &state))
            && state != State_Stopped)
            return VFW_E_NOT_STOPPED;
    }

    if (this->d->m_connectedTo) {
        this->d->m_connectedTo->Release();
        this->d->m_connectedTo = nullptr;
    }

    if (this->d->m_memInputPin) {
        this->d->m_memInputPin->Release();
        this->d->m_memInputPin = nullptr;
    }

    if (this->d->m_memAllocator) {
        this->d->m_memAllocator->Release();
        this->d->m_memAllocator = nullptr;
    }

    return S_OK;
}

HRESULT AkVCam::Pin::ConnectedTo(IPin **pPin)
{
    AkLogMethod();

    if (!pPin)
        return E_POINTER;

    *pPin = nullptr;

    if (!this->d->m_connectedTo)
        return VFW_E_NOT_CONNECTED;

    *pPin = this->d->m_connectedTo;
    (*pPin)->AddRef();

    return S_OK;
}

HRESULT AkVCam::Pin::ConnectionMediaType(AM_MEDIA_TYPE *pmt)
{
    AkLogMethod();

    if (!pmt)
        return E_POINTER;

    memset(pmt, 0, sizeof(AM_MEDIA_TYPE));

    if (!this->d->m_connectedTo)
        return VFW_E_NOT_CONNECTED;

    AM_MEDIA_TYPE *mediaType = nullptr;
    this->GetFormat(&mediaType);
    copyMediaType(pmt, mediaType);
    AkLoggerLog("Media Type: ", stringFromMediaType(mediaType));

    return S_OK;
}

HRESULT AkVCam::Pin::QueryPinInfo(PIN_INFO *pInfo)
{
    AkLogMethod();

    if (!pInfo)
        return E_POINTER;

    pInfo->pFilter = this->d->m_baseFilter;

    if (pInfo->pFilter)
        pInfo->pFilter->AddRef();

    pInfo->dir = PINDIR_OUTPUT;
    memset(pInfo->achName, 0, MAX_PIN_NAME * sizeof(WCHAR));

    if (!this->d->m_pinName.empty())
        memcpy(pInfo->achName,
               this->d->m_pinName.c_str(),
               std::min<size_t>(this->d->m_pinName.size() * sizeof(WCHAR),
                                MAX_PIN_NAME));

    return S_OK;
}

HRESULT AkVCam::Pin::QueryDirection(PIN_DIRECTION *pPinDir)
{
    AkLogMethod();

    if (!pPinDir)
        return E_POINTER;

    *pPinDir = PINDIR_OUTPUT;

    return S_OK;
}

HRESULT AkVCam::Pin::QueryId(LPWSTR *Id)
{
    AkLogMethod();

    if (!Id)
        return E_POINTER;

    auto wstrSize = (this->d->m_pinId.size() + 1) * sizeof(WCHAR);
    *Id = reinterpret_cast<LPWSTR>(CoTaskMemAlloc(wstrSize));

    if (!*Id)
        return E_OUTOFMEMORY;

    memset(*Id, 0, wstrSize);
    memcpy(*Id,
           this->d->m_pinId.c_str(),
           this->d->m_pinId.size() * sizeof(WCHAR));

    return S_OK;
}

HRESULT AkVCam::Pin::QueryAccept(const AM_MEDIA_TYPE *pmt)
{
    AkLogMethod();

    if (!pmt)
        return E_POINTER;

    AkLoggerLog("Accept? ", stringFromMediaType(pmt));

    if (!containsMediaType(pmt, this->d->m_mediaTypes)) {
        AkLoggerLog("NO");

        return S_FALSE;
    }

    AkLoggerLog("YES");

    return S_OK;
}

HRESULT AkVCam::Pin::EnumMediaTypes(IEnumMediaTypes **ppEnum)
{
    AkLogMethod();

    if (!ppEnum)
        return E_POINTER;

    *ppEnum = new AkVCam::EnumMediaTypes(this->d->m_mediaTypes->formats());
    (*ppEnum)->AddRef();

    return S_OK;
}

HRESULT AkVCam::Pin::QueryInternalConnections(IPin **apPin, ULONG *nPin)
{
    AkLogMethod();
    UNUSED(apPin)
    UNUSED(nPin)

    return E_NOTIMPL;
}

HRESULT AkVCam::Pin::EndOfStream()
{
    AkLogMethod();

    return E_UNEXPECTED;
}

HRESULT AkVCam::Pin::BeginFlush()
{
    AkLogMethod();

    return E_UNEXPECTED;
}

HRESULT AkVCam::Pin::EndFlush()
{
    AkLogMethod();

    return E_UNEXPECTED;
}

HRESULT AkVCam::Pin::NewSegment(REFERENCE_TIME tStart,
                                REFERENCE_TIME tStop,
                                double dRate)
{
    AkLogMethod();
    this->d->m_start = tStart;
    this->d->m_stop = tStop;
    this->d->m_rate = dRate;

    return S_OK;
}

void AkVCam::PinPrivate::sendFrameOneShot()
{
    AkLogMethod();

    WaitForSingleObject(this->m_sendFrameEvent, INFINITE);
    this->sendFrame();
    AkLoggerLog("Thread ", std::this_thread::get_id(), " finnished");
    this->m_running = false;
}

void AkVCam::PinPrivate::sendFrameLoop()
{
    AkLogMethod();

    while (this->m_running) {
        WaitForSingleObject(this->m_sendFrameEvent, INFINITE);
        auto result = this->sendFrame();

        if (FAILED(result)) {
            AkLoggerLog("Error sending frame: ",
                        result,
                        ": ",
                        stringFromResult(result));
            this->m_running = false;

            break;
        }
    }

    AkLoggerLog("Thread ", std::this_thread::get_id(), " finnished");
}

HRESULT AkVCam::PinPrivate::sendFrame()
{
    AkLogMethod();
    IMediaSample *sample = nullptr;

    if (FAILED(this->m_memAllocator->GetBuffer(&sample,
                                               nullptr,
                                               nullptr,
                                               0))
        || !sample)
        return E_FAIL;

    BYTE *buffer = nullptr;
    LONG size = sample->GetSize();

    if (size < 1 || FAILED(sample->GetPointer(&buffer)) || !buffer) {
        sample->Release();

        return E_FAIL;
    }

    this->m_mutex.lock();
    auto copyBytes = (std::min)(size_t(size), this->m_currentFrame.dataSize());

    if (copyBytes > 0)
        memcpy(buffer, this->m_currentFrame.data().get(), copyBytes);

    this->m_mutex.unlock();

    REFERENCE_TIME clock = 0;
    this->m_baseFilter->referenceClock()->GetTime(&clock);

    AM_MEDIA_TYPE *mediaType = nullptr;
    self->GetFormat(&mediaType);
    auto format = formatFromMediaType(mediaType);
    deleteMediaType(&mediaType);
    auto fps = format.minimumFrameRate();
    auto duration = REFERENCE_TIME(TIME_BASE
                                   * fps.den()
                                   / fps.num());

    if (this->m_pts < 0) {
        this->m_pts = 0;
        this->m_ptsDrift = this->m_pts - clock;
    } else {
        auto diff = clock - this->m_pts + this->m_ptsDrift;

        if (diff <= 2 * duration) {
            this->m_pts = clock + this->m_ptsDrift;
        } else {
            this->m_pts += duration;
            this->m_ptsDrift = this->m_pts - clock;
        }
    }

    auto startTime = this->m_pts;
    auto endTime = startTime + duration;

    sample->SetTime(&startTime, &endTime);
    sample->SetMediaTime(&startTime, &endTime);
    sample->SetActualDataLength(size);
    sample->SetDiscontinuity(false);
    sample->SetSyncPoint(true);
    sample->SetPreroll(false);
    AkLoggerLog("Sending ", stringFromMediaSample(sample));
    auto result = this->m_memInputPin->Receive(sample);
    AkLoggerLog("Frame sent");
    sample->Release();

    return result;
}

void AkVCam::PinPrivate::updateTestFrame()
{
    auto frame = this->applyAdjusts(this->m_testFrame);

    if (frame.format().size() < 1)
        return;

    this->m_testFrameAdapted = frame;
}

AkVCam::VideoFrame AkVCam::PinPrivate::applyAdjusts(const VideoFrame &frame)
{
    AM_MEDIA_TYPE *mediaType = nullptr;

    if (FAILED(this->self->GetFormat(&mediaType)))
        return {};

    auto format = formatFromMediaType(mediaType);
    deleteMediaType(&mediaType);
    FourCC fourcc = format.fourcc();
    int width = format.width();
    int height = format.height();

    /* In Windows red and blue channels are swapped, so hack it with the
     * opposite format. Endianness problem maybe?
     */
    std::map<FourCC, FourCC> fixFormat {
        {PixelFormatRGB32, PixelFormatBGR32},
        {PixelFormatRGB24, PixelFormatBGR24},
        {PixelFormatRGB16, PixelFormatBGR16},
        {PixelFormatRGB15, PixelFormatBGR15},
    };

    bool vmirror;

    if (fixFormat.count(fourcc) > 0) {
        fourcc = fixFormat[fourcc];
        vmirror = this->m_verticalMirror == this->m_verticalFlip;
    } else {
        vmirror = this->m_verticalMirror != this->m_verticalFlip;
    }

    VideoFrame newFrame;

    if (width * height > frame.format().width() * frame.format().height()) {
        newFrame =
                frame
                .mirror(this->m_horizontalMirror != this->m_horizontalFlip,
                        vmirror)
                .swapRgb(this->m_swapRgb)
                .adjust(this->m_hue,
                        this->m_saturation,
                        this->m_brightness,
                        this->m_gamma,
                        this->m_contrast,
                        !this->m_colorenable)
                .scaled(width, height,
                        this->m_scaling,
                        this->m_aspectRatio)
                .convert(fourcc);
    } else {
        newFrame =
                frame
                .scaled(width, height,
                        this->m_scaling,
                        this->m_aspectRatio)
                .mirror(this->m_horizontalMirror != this->m_horizontalFlip,
                        vmirror)
                .swapRgb(this->m_swapRgb)
                .adjust(this->m_hue,
                        this->m_saturation,
                        this->m_brightness,
                        this->m_gamma,
                        this->m_contrast,
                        !this->m_colorenable)
                .convert(fourcc);

    }

    newFrame.format().fourcc() = format.fourcc();

    return newFrame;
}

void AkVCam::PinPrivate::propertyChanged(void *userData,
                                         LONG Property,
                                         LONG lValue,
                                         LONG Flags)
{
    AkLoggerLog("PinPrivate::propertyChanged()");
    UNUSED(Flags)
    auto self = reinterpret_cast<PinPrivate *>(userData);

    switch (Property) {
    case VideoProcAmp_Brightness:
        self->m_brightness = lValue;

        break;

    case VideoProcAmp_Contrast:
        self->m_contrast = lValue;

        break;

    case VideoProcAmp_Saturation:
        self->m_saturation = lValue;

        break;

    case VideoProcAmp_Gamma:
        self->m_gamma = lValue;

        break;

    case VideoProcAmp_Hue:
        self->m_hue = lValue;

        break;

    case VideoProcAmp_ColorEnable:
        self->m_colorenable = lValue;
        break;

    default:
        break;
    }

    self->updateTestFrame();
    self->m_mutex.lock();
    self->m_currentFrame = self->m_testFrameAdapted;
    self->m_mutex.unlock();
}
