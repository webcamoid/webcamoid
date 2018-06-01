/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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

#include <thread>
#include <CoreMediaIO/CMIOSampleBuffer.h>

#include "stream.h"
#include "clock.h"
#include "utils.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/image/videoframe.h"

namespace AkVCam
{
    class StreamPrivate
    {
        public:
            Stream *self;
            ClockPtr m_clock;
            UInt64 m_sequence;
            CMTime m_pts;
            SampleBufferQueuePtr m_queue;
            CMIODeviceStreamQueueAlteredProc m_queueAltered;
            VideoFormat m_format;
            double m_fps;
            VideoFrame m_currentFrame;
            VideoFrame m_testFrame;
            VideoFrame m_testFrameAdapted;
            void *m_queueAlteredRefCon;
            CFRunLoopTimerRef m_timer;
            bool m_running;
            std::string m_broadcaster;
            bool m_horizontalMirror;
            bool m_verticalMirror;
            Scaling m_scaling;
            AspectRatio m_aspectRatio;
            bool m_swapRgb;
            std::mutex m_mutex;

            StreamPrivate(Stream *self):
                self(self)
            {
            }

            bool startTimer();
            void stopTimer();
            static void streamLoop(CFRunLoopTimerRef timer, void *info);
            void sendFrame(const VideoFrame &frame);
            void updateTestFrame();
            VideoFrame applyAdjusts(const VideoFrame &frame);
    };
}

AkVCam::Stream::Stream(bool registerObject,
                       Object *parent):
    Object(parent)
{
    this->d = new StreamPrivate(this);
    this->d->m_queueAltered = nullptr;
    this->d->m_queueAlteredRefCon = nullptr;
    this->d->m_timer = nullptr;
    this->d->m_running = false;
    this->d->m_horizontalMirror = false;
    this->d->m_verticalMirror = false;
    this->d->m_scaling = ScalingFast;
    this->d->m_aspectRatio = AspectRatioIgnore;
    this->d->m_swapRgb = false;
    this->m_className = "Stream";
    this->m_classID = kCMIOStreamClassID;
    this->d->m_testFrame = {":/VirtualCamera/share/TestFrame/TestFrame.bmp"};
    this->d->m_clock =
            std::make_shared<Clock>("CMIO::VirtualCamera::Stream",
                                    CMTimeMake(1, 10),
                                    100,
                                    10);
    this->d->m_queue = std::make_shared<SampleBufferQueue>(30);

    if (registerObject) {
        this->createObject();
        this->registerObject();
    }

    this->m_properties.setProperty(kCMIOStreamPropertyClock, this->d->m_clock);
}

AkVCam::Stream::~Stream()
{
    this->registerObject(false);
    delete this->d;
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
        AkLoggerLog("Created stream: ", this->m_objectID);
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

    std::vector<VideoFormat> formatsAdjusted;

    for (auto format: formats) {
        int width;
        int height;
        AkVCam::VideoFormat::roundNearest(format.width(),
                                          format.height(),
                                          &width,
                                          &height);
        format.width() = width;
        format.height() = height;
        formatsAdjusted.push_back(format);
    }

#ifdef QT_DEBUG
    for (auto &format: formatsAdjusted)
        AkLoggerLog("Format: ",
                    enumToString(format.fourcc()),
                    " ",
                    format.width(),
                    "x",
                    format.height());
#endif

    this->m_properties.setProperty(kCMIOStreamPropertyFormatDescriptions,
                                   formatsAdjusted);
    this->setFormat(formatsAdjusted[0]);
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

    this->d->m_format = format;
}

void AkVCam::Stream::setFrameRate(Float64 frameRate)
{
    this->m_properties.setProperty(kCMIOStreamPropertyFrameRate,
                                   frameRate);
    this->d->m_fps = frameRate;
}

bool AkVCam::Stream::start()
{
    AkObjectLogMethod();

    if (this->d->m_running)
        return false;

    this->d->updateTestFrame();
    this->d->m_currentFrame = this->d->m_testFrameAdapted;
    this->d->m_sequence = 0;
    memset(&this->d->m_pts, 0, sizeof(CMTime));
    this->d->m_running = this->d->startTimer();
    AkLoggerLog("Running: ", this->d->m_running);

    return this->d->m_running;
}

void AkVCam::Stream::stop()
{
    AkObjectLogMethod();

    if (!this->d->m_running)
        return;

    this->d->m_running = false;
    this->d->stopTimer();
    this->d->m_currentFrame.clear();
    this->d->m_testFrameAdapted.clear();
}

bool AkVCam::Stream::running()
{
    return this->d->m_running;
}

void AkVCam::Stream::frameReady(const AkVCam::VideoFrame &frame)
{
    AkObjectLogMethod();
    AkLoggerLog("Running: ", this->d->m_running);
    AkLoggerLog("Broadcaster: ", this->d->m_broadcaster);

    if (!this->d->m_running)
        return;

    this->d->m_mutex.lock();

    if (!this->d->m_broadcaster.empty())
        this->d->m_currentFrame = this->d->applyAdjusts(frame);

    this->d->m_mutex.unlock();
}

void AkVCam::Stream::setBroadcasting(const std::string &broadcaster)
{
    AkObjectLogMethod();

    if (this->d->m_broadcaster == broadcaster)
        return;

    this->d->m_mutex.lock();
    this->d->m_broadcaster = broadcaster;

    if (broadcaster.empty())
        this->d->m_currentFrame = this->d->m_testFrameAdapted;

    this->d->m_mutex.unlock();
}

void AkVCam::Stream::setMirror(bool horizontalMirror, bool verticalMirror)
{
    AkObjectLogMethod();

    if (this->d->m_horizontalMirror == horizontalMirror
        && this->d->m_verticalMirror == verticalMirror)
        return;

    this->d->m_horizontalMirror = horizontalMirror;
    this->d->m_verticalMirror = verticalMirror;
    this->d->updateTestFrame();
}

void AkVCam::Stream::setScaling(Scaling scaling)
{
    AkObjectLogMethod();

    if (this->d->m_scaling == scaling)
        return;

    this->d->m_scaling = scaling;
    this->d->updateTestFrame();
}

void AkVCam::Stream::setAspectRatio(AspectRatio aspectRatio)
{
    AkObjectLogMethod();

    if (this->d->m_aspectRatio == aspectRatio)
        return;

    this->d->m_aspectRatio = aspectRatio;
    this->d->updateTestFrame();
}

void AkVCam::Stream::setSwapRgb(bool swap)
{
    AkObjectLogMethod();

    if (this->d->m_swapRgb == swap)
        return;

    this->d->m_swapRgb = swap;
    this->d->updateTestFrame();
}

OSStatus AkVCam::Stream::copyBufferQueue(CMIODeviceStreamQueueAlteredProc queueAlteredProc,
                                         void *queueAlteredRefCon,
                                         CMSimpleQueueRef *queue)
{
    AkObjectLogMethod();

    this->d->m_queueAltered = queueAlteredProc;
    this->d->m_queueAlteredRefCon = queueAlteredRefCon;
    *queue = queueAlteredProc? this->d->m_queue->ref(): nullptr;

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

bool AkVCam::StreamPrivate::startTimer()
{
    AkLoggerLog("AkVCam::StreamPrivate::startTimer()");

    if (this->m_timer)
        return false;

    CFTimeInterval interval = 1.0 / this->m_fps;
    CFRunLoopTimerContext context {0, this, nullptr, nullptr, nullptr};
    this->m_timer =
            CFRunLoopTimerCreate(kCFAllocatorDefault,
                                 0.0,
                                 interval,
                                 0,
                                 0,
                                 StreamPrivate::streamLoop,
                                 &context);

    if (!this->m_timer)
        return false;

    CFRunLoopAddTimer(CFRunLoopGetMain(),
                      this->m_timer,
                      kCFRunLoopCommonModes);

    return true;
}

void AkVCam::StreamPrivate::stopTimer()
{
    AkLoggerLog("AkVCam::StreamPrivate::stopTimer()");

    if (!this->m_timer)
        return;

    CFRunLoopTimerInvalidate(this->m_timer);
    CFRunLoopRemoveTimer(CFRunLoopGetMain(),
                         this->m_timer,
                         kCFRunLoopCommonModes);
    CFRelease(this->m_timer);
    this->m_timer = nullptr;
}

void AkVCam::StreamPrivate::streamLoop(CFRunLoopTimerRef timer, void *info)
{
    AkLoggerLog("AkVCam::StreamPrivate::streamLoop()");
    UNUSED(timer)

    auto self = reinterpret_cast<StreamPrivate *>(info);
    AkLoggerLog("Running: ", self->m_running);

    if (!self->m_running)
        return;

    self->m_mutex.lock();
    self->sendFrame(self->m_currentFrame);
    self->m_mutex.unlock();
}

void AkVCam::StreamPrivate::sendFrame(const VideoFrame &frame)
{
    AkLoggerLog("AkVCam::StreamPrivate::sendFrame()");

    if (this->m_queue->fullness() >= 1.0f)
        return;

    FourCC fourcc = frame.format().fourcc();
    int width = frame.format().width();
    int height = frame.format().height();

    AkLoggerLog("Sending Frame: ",
                enumToString(fourcc),
                " ",
                width,
                "x",
                height);

    bool resync = false;
    auto hostTime = CFAbsoluteTimeGetCurrent();
    auto pts = CMTimeMake(int64_t(hostTime), 1e9);
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
                                   UInt64(hostTime),
                                   resync,
                                   this->m_clock->ref());

    CVImageBufferRef imageBuffer = nullptr;
    CVPixelBufferCreate(kCFAllocatorDefault,
                        size_t(width),
                        size_t(height),
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

    auto duration = CMTimeMake(1, int32_t(this->m_fps));
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
        this->m_queueAltered(this->self->m_objectID,
                             buffer,
                             this->m_queueAlteredRefCon);
}

void AkVCam::StreamPrivate::updateTestFrame()
{
    this->m_testFrameAdapted = this->applyAdjusts(this->m_testFrame);
}

AkVCam::VideoFrame AkVCam::StreamPrivate::applyAdjusts(const VideoFrame &frame)
{
    FourCC fourcc = this->m_format.fourcc();
    int width = this->m_format.width();
    int height = this->m_format.height();

    if (width * height > frame.format().width() * frame.format().height()) {
        return frame.mirror(this->m_horizontalMirror,
                            this->m_verticalMirror)
                    .swapRgb(this->m_swapRgb)
                    .scaled(width, height,
                            this->m_scaling,
                            this->m_aspectRatio)
                    .convert(fourcc);
    }

    return frame.scaled(width, height,
                        this->m_scaling,
                        this->m_aspectRatio)
                .mirror(this->m_horizontalMirror,
                        this->m_verticalMirror)
                .swapRgb(this->m_swapRgb)
                .convert(fourcc);
}
