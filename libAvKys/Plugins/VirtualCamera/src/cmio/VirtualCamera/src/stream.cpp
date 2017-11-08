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

#include <CoreMediaIO/CMIOSampleBuffer.h>

#include "stream.h"
#include "utils.h"

Ak::Stream::Stream(bool registerObject,
                   Object *parent):
    Object(parent),
    m_queueAltered(nullptr),
    m_queueAlteredRefCon(nullptr),
    m_timer(nullptr),
    m_running(false)
{
    this->m_className = "Stream";
    this->m_classID = kCMIOStreamClassID;
    this->m_clock =
            ClockPtr(new Clock("CMIO::VirtualCamera::Stream",
                               CMTimeMake(1, 10),
                               100,
                               10));
    this->m_queue = SampleBufferQueuePtr(new SampleBufferQueue(30));

    if (registerObject) {
        this->createObject();
        this->registerObject();
    }

    this->m_properties.setProperty(kCMIOStreamPropertyClock, this->m_clock);
}

Ak::Stream::~Stream()
{
    this->registerObject(false);
}

OSStatus Ak::Stream::createObject()
{
    AkObjectLogMethod();

    if (!this->m_pluginInterface
        || !*this->m_pluginInterface
        || !this->m_parent)
        return kCMIOHardwareUnspecifiedError;

    CMIOObjectID streamID = 0;

    auto status =
            CMIOObjectCreate(this->m_pluginInterface,
                             this->m_parent->objectID(),
                             this->m_classID,
                             &streamID);

    if (status == kCMIOHardwareNoError) {
        this->m_isCreated = true;
        this->m_objectID = streamID;
        AkLoggerLog("Created stream: " << this->m_objectID);
    }

    return status;
}

OSStatus Ak::Stream::registerObject(bool regist)
{
    AkObjectLogMethod();
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!this->m_isCreated
        || !this->m_pluginInterface
        || !*this->m_pluginInterface
        || !this->m_parent)
        return status;

    if (regist) {
        status = CMIOObjectsPublishedAndDied(this->m_pluginInterface,
                                             this->m_parent->objectID(),
                                             1,
                                             &this->m_objectID,
                                             0,
                                             nullptr);
    } else {
        status = CMIOObjectsPublishedAndDied(this->m_pluginInterface,
                                             this->m_parent->objectID(),
                                             0,
                                             nullptr,
                                             1,
                                             &this->m_objectID);
    }

    return status;
}

void Ak::Stream::setFormats(const std::vector<Ak::VideoFormat> &formats)
{
    AkObjectLogMethod();
    if (formats.empty())
        return;

    this->m_properties.setProperty(kCMIOStreamPropertyFormatDescriptions,
                                   formats);
    this->setFormat(formats[0]);
}

void Ak::Stream::setFormat(const Ak::VideoFormat &format)
{
    AkObjectLogMethod();
    this->m_properties.setProperty(kCMIOStreamPropertyFormatDescription,
                                   format);
    this->m_properties.setProperty(kCMIOStreamPropertyFrameRates,
                                   format.frameRates());
    this->m_properties.setProperty(kCMIOStreamPropertyFrameRateRanges,
                                   format.frameRateRanges());

    this->m_properties.setProperty(kCMIOStreamPropertyMinimumFrameRate,
                                   format.minimumFrameRate());

    if (!format.frameRates().empty())
        this->setFrameRate(format.frameRates()[0]);
}

void Ak::Stream::setFrameRate(Float64 frameRate)
{
    this->m_properties.setProperty(kCMIOStreamPropertyFrameRate,
                                   frameRate);
}

bool Ak::Stream::start()
{
    AkObjectLogMethod();

    if (this->m_running)
        return false;

    CFTimeInterval interval = 30.0e-3;
    CFRunLoopTimerContext context {0, this, nullptr, nullptr, nullptr};
    this->m_timer =
            CFRunLoopTimerCreate(kCFAllocatorDefault,
                                 0.0,
                                 interval,
                                 0,
                                 0,
                                 Ak::Stream::streamLoop,
                                 &context);

    if (!this->m_timer)
        return false;

    CFRunLoopAddTimer(CFRunLoopGetMain(),
                      this->m_timer,
                      kCFRunLoopCommonModes);
    this->m_running = true;

    return true;
}

void Ak::Stream::stop()
{
    AkObjectLogMethod();

    if (!this->m_running)
        return;

    this->m_running = false;

    CFRunLoopTimerInvalidate(this->m_timer);
    CFRunLoopRemoveTimer(CFRunLoopGetMain(),
                         this->m_timer,
                         kCFRunLoopCommonModes);
    CFRelease(this->m_timer);
    this->m_timer = nullptr;
}

bool Ak::Stream::running()
{
    return this->m_running;
}

OSStatus Ak::Stream::copyBufferQueue(CMIODeviceStreamQueueAlteredProc queueAlteredProc,
                                     void *queueAlteredRefCon,
                                     CMSimpleQueueRef *queue)
{
    AkObjectLogMethod();

    this->m_queueAltered = queueAlteredProc;
    this->m_queueAlteredRefCon = queueAlteredRefCon;
    *queue = queueAlteredProc? this->m_queue->ref(): nullptr;

    if (*queue)
        CFRetain(*queue);

    return kCMIOHardwareNoError;
}

OSStatus Ak::Stream::deckPlay()
{
    AkObjectLogMethod();

    AkLoggerLog("STUB");

    return kCMIOHardwareUnspecifiedError;
}

OSStatus Ak::Stream::deckStop()
{
    AkObjectLogMethod();

    AkLoggerLog("STUB");

    return kCMIOHardwareUnspecifiedError;
}

OSStatus Ak::Stream::deckJog(SInt32 speed)
{
    AkObjectLogMethod();
    UNUSED(speed)

    AkLoggerLog("STUB");

    return kCMIOHardwareUnspecifiedError;
}

OSStatus Ak::Stream::deckCueTo(Float64 frameNumber, Boolean playOnCue)
{
    AkObjectLogMethod();
    UNUSED(frameNumber)
    UNUSED(playOnCue)

    AkLoggerLog("STUB");

    return kCMIOHardwareUnspecifiedError;
}

void Ak::Stream::streamLoop(CFRunLoopTimerRef timer, void *info)
{
    AkLoggerLog("Reading frame");
    UNUSED(timer)

    auto self = reinterpret_cast<Stream *>(info);

    if (self->m_queue->fullness() >= 1.0f)
        return;

    static int64_t pts = 0;

    CMIOStreamClockPostTimingEvent(CMTimeMake(pts, 30),
                                   UInt64(CFAbsoluteTimeGetCurrent()),
                                   false,
                                   self->m_clock->ref());
    pts++;

    CVImageBufferRef imageBuffer = nullptr;
    CVPixelBufferCreate(kCFAllocatorDefault,
                        640,
                        480,
                        kCMPixelFormat_32ARGB,
                        nullptr,
                        &imageBuffer);

    CVPixelBufferLockBaseAddress(imageBuffer, 0);
    auto data = reinterpret_cast<UInt32 *>(CVPixelBufferGetBaseAddress(imageBuffer));

    for (int y = 0; y < 480; y++) {
        auto line = data + y * 640;

        for (int x = 0; x < 640; x++) {
            UInt32 r = rand() % 256;
            UInt32 g = rand() % 256;
            UInt32 b = rand() % 256;

            line[x] = 0xff000000 | (r << 16) | (g << 8) | b;
        }
    }

    CVPixelBufferUnlockBaseAddress(imageBuffer, 0);

    CMVideoFormatDescriptionRef format = nullptr;
    CMVideoFormatDescriptionCreateForImageBuffer(kCFAllocatorDefault,
                                                 imageBuffer,
                                                 &format);

    CMSampleTimingInfo timingInfo {
        CMTimeMake(1, 30),
        CMTimeMake(pts, 30),
        CMTimeMake(pts, 30)
    };

    CMSampleBufferRef buffer = nullptr;
    CMIOSampleBufferCreateForImageBuffer(kCFAllocatorDefault,
                                         imageBuffer,
                                         format,
                                         &timingInfo,
                                         UInt64(pts),
                                         0,
                                         &buffer);

    self->m_queue->enqueue(buffer);

    if (self->m_queueAltered)
        self->m_queueAltered(self->m_objectID,
                             buffer,
                             self->m_queueAlteredRefCon);
}
