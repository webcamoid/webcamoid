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
#include "referenceclock.h"
#include "utils.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/utils.h"

#define AK_CUR_INTERFACE "Pin"

namespace AkVCam
{
    class PinPrivate
    {
        public:
            Pin *self;
            BaseFilter *m_baseFilter;
            std::wstring m_pinName;
            std::wstring m_pinId;
            EnumMediaTypes *m_mediaTypes;
            IPin *m_connectedTo;
            IMemInputPin *m_memInputPin;
            IMemAllocator *m_memAllocator;
            REFERENCE_TIME m_refTime;
            REFERENCE_TIME m_start;
            REFERENCE_TIME m_stop;
            double m_rate;
            FILTER_STATE m_prevState;
            DWORD_PTR m_adviseCookie;
            HANDLE m_sendFrameEvent;
            std::thread m_sendFrameThread;
            std::atomic<bool> m_running;
            bool m_horizontalFlip;
            bool m_verticalFlip;

            void sendFrameOneShot();
            void sendFrameLoop();
            HRESULT sendFrame();
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
    this->d->m_refTime = -1;
    this->d->m_start = 0;
    this->d->m_stop = MAXLONGLONG;
    this->d->m_rate = 1.0;
    this->d->m_horizontalFlip = false;
    this->d->m_verticalFlip = false;
    this->d->m_prevState = State_Stopped;
    this->d->m_adviseCookie = 0;
    this->d->m_sendFrameEvent = nullptr;
    this->d->m_running = false;
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
        if (state == State_Paused) {            
            if (FAILED(self->d->m_memAllocator->Commit()))
                return VFW_E_NOT_COMMITTED;

            self->d->m_refTime = -1;
            self->d->m_sendFrameEvent =
                    CreateEvent(nullptr, FALSE, FALSE, L"SendFrame");

            self->d->m_running = true;
            self->d->m_sendFrameThread =
                    std::thread(&PinPrivate::sendFrameOneShot, self->d);
            AkLoggerLog("Launching thread ", self->d->m_sendFrameThread.get_id());

            /* The filter must send an initial thumbnail frame before going to
             * running state.
             */
            auto clock = self->d->m_baseFilter->referenceClock();
            REFERENCE_TIME now = 0;
            clock->GetTime(&now);
            clock->AdviseTime(now,
                              0,
                              HEVENT(self->d->m_sendFrameEvent),
                              &self->d->m_adviseCookie);
        }
    } else if (self->d->m_prevState == State_Paused) {
        if (state == State_Stopped) {
            self->d->m_sendFrameThread.join();
            self->d->m_memAllocator->Decommit();
        } else if (state == State_Running) {
            auto clock = self->d->m_baseFilter->referenceClock();
            self->d->m_running = false;
            self->d->m_sendFrameThread.join();

            if (self->d->m_adviseCookie)
                clock->Unadvise(self->d->m_adviseCookie);

            if (self->d->m_sendFrameEvent)
                CloseHandle(self->d->m_sendFrameEvent);

            self->d->m_sendFrameEvent =
                    CreateSemaphore(nullptr, 1, 1, L"SendFrame");

            self->d->m_running = true;
            self->d->m_sendFrameThread =
                    std::thread(&PinPrivate::sendFrameLoop, self->d);
            AkLoggerLog("Launching thread ", self->d->m_sendFrameThread.get_id());

            REFERENCE_TIME now = 0;
            clock->GetTime(&now);

            AM_MEDIA_TYPE *mediaType = nullptr;
            self->GetFormat(&mediaType);
            auto videoFormat = formatFromMediaType(mediaType);
            deleteMediaType(&mediaType);
            auto period = REFERENCE_TIME(TIME_BASE
                                         / videoFormat.minimumFrameRate());

            clock->AdvisePeriodic(now,
                                  period,
                                  HSEMAPHORE(self->d->m_sendFrameEvent),
                                  &self->d->m_adviseCookie);
        }
    } else if (self->d->m_prevState == State_Running) {
        self->d->m_running = false;

        if (state == State_Stopped)
            self->d->m_sendFrameThread.join();

        auto clock = self->d->m_baseFilter->referenceClock();
        clock->Unadvise(self->d->m_adviseCookie);

        self->d->m_adviseCookie = 0;
        CloseHandle(self->d->m_sendFrameEvent);
        self->d->m_sendFrameEvent = nullptr;

        if (state == State_Stopped)
            self->d->m_memAllocator->Decommit();
    }

    self->d->m_prevState = state;

    return S_OK;
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

    this->d->m_baseFilter->subscribeStateChanged(this, Pin::stateChanged);

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

    this->d->m_baseFilter->unsubscribeStateChanged(this, Pin::stateChanged);

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

    for (LONG i = 0; i < size; i++)
        buffer[i] = rand() % 255;

    AM_MEDIA_TYPE *mediaType = nullptr;
    self->GetFormat(&mediaType);
    REFERENCE_TIME startTime = 0;
    auto clock = this->m_baseFilter->referenceClock();
    clock->GetTime(&startTime);

    if (this->m_refTime < 0)
        this->m_refTime = startTime;

    startTime -= this->m_refTime;
    auto format = formatFromMediaType(mediaType);
    auto endTime = startTime
                 + REFERENCE_TIME(TIME_BASE / format.minimumFrameRate());
    deleteMediaType(&mediaType);
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
