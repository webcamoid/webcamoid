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

AkVCam::Stream::Stream(bool registerObject,
                       Object *parent):
    Object(parent),
    m_queueAltered(nullptr),
    m_queueAlteredRefCon(nullptr),
    m_timer(nullptr),
    m_running(false),
    m_broadcasting(false),
    m_horizontalMirror(false),
    m_verticalMirror(false),
    m_scaling(VideoFrame::ScalingFast)
{
    this->m_className = "Stream";
    this->m_classID = kCMIOStreamClassID;
    this->m_testFrame = {":/VirtualCamera/share/TestFrame/TestFrame.bmp"};
    this->m_clock =
            std::make_shared<Clock>("CMIO::VirtualCamera::Stream",
                                    CMTimeMake(1, 10),
                                    100,
                                    10);
    this->m_queue = std::make_shared<SampleBufferQueue>(30);

    if (registerObject) {
        this->createObject();
        this->registerObject();
    }

    this->m_properties.setProperty(kCMIOStreamPropertyClock, this->m_clock);
}

AkVCam::Stream::~Stream()
{
    this->registerObject(false);
}

OSStatus AkVCam::Stream::createObject()
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

OSStatus AkVCam::Stream::registerObject(bool regist)
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

void AkVCam::Stream::setFormats(const std::vector<VideoFormat> &formats)
{
    AkObjectLogMethod();

    if (formats.empty())
        return;

#ifdef QT_DEBUG
    for (auto &format: formats)
        AkLoggerLog("Format: "
                    << enumToString(format.fourcc())
                    << " "
                    << format.width()
                    << "x"
                    << format.height());
#endif

    this->m_properties.setProperty(kCMIOStreamPropertyFormatDescriptions,
                                   formats);
    this->setFormat(formats[0]);
}

void AkVCam::Stream::setFormat(const VideoFormat &format)
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

    this->m_format = format;
}

void AkVCam::Stream::setFrameRate(Float64 frameRate)
{
    this->m_properties.setProperty(kCMIOStreamPropertyFrameRate,
                                   frameRate);
    this->m_fps = frameRate;
}

bool AkVCam::Stream::start()
{
    AkObjectLogMethod();

    if (this->m_running)
        return false;

    this->m_sequence = 0;
    memset(&this->m_pts, 0, sizeof(CMTime));    

    if (this->m_broadcasting) {
        this->m_running = true;

        return true;
    }

    this->m_running = this->startTimer();

    return this->m_running;
}

void AkVCam::Stream::stop()
{
    AkObjectLogMethod();

    if (!this->m_running)
        return;

    this->m_running = false;
    this->stopTimer();
}

bool AkVCam::Stream::running()
{
    return this->m_running;
}

void AkVCam::Stream::frameReady(const AkVCam::VideoFrame &frame)
{
    AkObjectLogMethod();
    AkLoggerLog("Running: " << this->m_running);
    AkLoggerLog("Broadcasting: " << this->m_broadcasting);

    if (!this->m_running || !this->m_broadcasting)
        return;

    FourCC fourcc = this->m_format.fourcc();
    int width = this->m_format.width();
    int height = this->m_format.height();

    auto outputFrame =
            frame
            .mirror(this->m_horizontalMirror,
                    this->m_verticalMirror)
            .scaled(width, height,
                    this->m_scaling)
            .convert(fourcc);

    this->sendFrame(outputFrame);
}

void AkVCam::Stream::setBroadcasting(bool broadcasting)
{
    this->m_broadcasting = broadcasting;

    if (broadcasting)
        this->stopTimer();
    else if (this->m_running)
        this->startTimer();
}

void AkVCam::Stream::setMirror(bool horizontalMirror, bool verticalMirror)
{
    this->m_horizontalMirror = horizontalMirror;
    this->m_verticalMirror = verticalMirror;
}

void AkVCam::Stream::setScaling(AkVCam::VideoFrame::Scaling scaling)
{
    this->m_scaling = scaling;
}

OSStatus AkVCam::Stream::copyBufferQueue(CMIODeviceStreamQueueAlteredProc queueAlteredProc,
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

OSStatus AkVCam::Stream::deckPlay()
{
    AkObjectLogMethod();

    AkLoggerLog("STUB");

    return kCMIOHardwareUnspecifiedError;
}

OSStatus AkVCam::Stream::deckStop()
{
    AkObjectLogMethod();

    AkLoggerLog("STUB");

    return kCMIOHardwareUnspecifiedError;
}

OSStatus AkVCam::Stream::deckJog(SInt32 speed)
{
    AkObjectLogMethod();
    UNUSED(speed)

    AkLoggerLog("STUB");

    return kCMIOHardwareUnspecifiedError;
}

OSStatus AkVCam::Stream::deckCueTo(Float64 frameNumber, Boolean playOnCue)
{
    AkObjectLogMethod();
    UNUSED(frameNumber)
    UNUSED(playOnCue)

    AkLoggerLog("STUB");

    return kCMIOHardwareUnspecifiedError;
}

bool AkVCam::Stream::startTimer()
{
    if (this->m_timer)
        return false;

    FourCC fourcc = this->m_format.fourcc();
    int width = this->m_format.width();
    int height = this->m_format.height();

    this->m_testFrameAdapted =
            this->m_testFrame
            .mirror(this->m_horizontalMirror,
                    this->m_verticalMirror)
            .scaled(width, height,
                    this->m_scaling)
            .convert(fourcc);

    CFTimeInterval interval = 1.0 / this->m_fps;
    CFRunLoopTimerContext context {0, this, nullptr, nullptr, nullptr};
    this->m_timer =
            CFRunLoopTimerCreate(kCFAllocatorDefault,
                                 0.0,
                                 interval,
                                 0,
                                 0,
                                 Stream::streamLoop,
                                 &context);

    if (!this->m_timer)
        return false;

    CFRunLoopAddTimer(CFRunLoopGetMain(),
                      this->m_timer,
                      kCFRunLoopCommonModes);

    return true;
}

void AkVCam::Stream::stopTimer()
{
    if (!this->m_timer)
        return;

    CFRunLoopTimerInvalidate(this->m_timer);
    CFRunLoopRemoveTimer(CFRunLoopGetMain(),
                         this->m_timer,
                         kCFRunLoopCommonModes);
    CFRelease(this->m_timer);
    this->m_timer = nullptr;
    this->m_testFrameAdapted.clear();
}

void AkVCam::Stream::streamLoop(CFRunLoopTimerRef timer, void *info)
{
    AkLoggerLog("AkVCam::Stream::streamLoop");
    UNUSED(timer)

    auto self = reinterpret_cast<Stream *>(info);

    if (!self->m_running || self->m_broadcasting)
        return;

    self->sendFrame(self->m_testFrameAdapted);
}

void AkVCam::Stream::sendFrame(const AkVCam::VideoFrame &frame)
{
    AkObjectLogMethod();

    if (this->m_queue->fullness() >= 1.0f)
        return;

    FourCC fourcc = frame.format().fourcc();
    int width = frame.format().width();
    int height = frame.format().height();

    AkLoggerLog("Sending Frame: "
                << enumToString(fourcc)
                << " "
                << width
                << "x"
                << height);

    bool resync = false;
    auto hostTime = UInt64(CFAbsoluteTimeGetCurrent());
    auto pts = CMTimeMake(hostTime, 1e9);
    auto ptsDiff = CMTimeGetSeconds(CMTimeSubtract(this->m_pts, pts));

    if (CMTimeCompare(pts, this->m_pts) == 0)
        return;
    if (CMTIME_IS_INVALID(this->m_pts)
        || ptsDiff < 0
        || ptsDiff > 2. / this->m_fps) {
        this->m_pts = pts;
        resync = true;
    }

    CMIOStreamClockPostTimingEvent(this->m_pts,
                                   hostTime,
                                   resync,
                                   this->m_clock->ref());

    CVImageBufferRef imageBuffer = nullptr;
    CVPixelBufferCreate(kCFAllocatorDefault,
                        width,
                        height,
                        formatToCM(PixelFormat(fourcc)),
                        nullptr,
                        &imageBuffer);

    if (!imageBuffer)
        return;

    CVPixelBufferLockBaseAddress(imageBuffer, 0);
    auto data = CVPixelBufferGetBaseAddress(imageBuffer);
    memcpy(data, frame.data().get(), frame.dataSize());
    CVPixelBufferUnlockBaseAddress(imageBuffer, 0);

    CMVideoFormatDescriptionRef format = nullptr;
    CMVideoFormatDescriptionCreateForImageBuffer(kCFAllocatorDefault,
                                                 imageBuffer,
                                                 &format);

    auto duration = CMTimeMake(1, this->m_fps);
    CMSampleTimingInfo timingInfo {
        duration,
        this->m_pts,
        this->m_pts
    };

    CMSampleBufferRef buffer = nullptr;
    CMIOSampleBufferCreateForImageBuffer(kCFAllocatorDefault,
                                         imageBuffer,
                                         format,
                                         &timingInfo,
                                         this->m_sequence,
                                         resync?
                                             kCMIOSampleBufferDiscontinuityFlag_UnknownDiscontinuity:
                                             kCMIOSampleBufferNoDiscontinuities,
                                         &buffer);
    CFRelease(format);
    CFRelease(imageBuffer);

    this->m_queue->enqueue(buffer);
    this->m_pts = CMTimeAdd(this->m_pts, duration);
    this->m_sequence++;

    if (this->m_queueAltered)
        this->m_queueAltered(this->m_objectID,
                             buffer,
                             this->m_queueAlteredRefCon);
}
