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

#include <QTime>
#include <ak.h>
#include <akvideopacket.h>

#import "framegrabber.h"

@implementation FrameGrabber

- (id) initWithScreenDev: (AVFoundationScreenDev *) screenDev
       onScreen: (CGDirectDisplayID) screen
       withFps: (AkFrac) fps
{
    self = [super init];

    if (!self)
        return nil;

    m_id = Ak::id();
    m_screenDev = screenDev;
    m_screen = screen;
    m_fps = fps;

    return self;
}

- (void) captureOutput: (AVCaptureOutput *) captureOutput
         didOutputSampleBuffer: (CMSampleBufferRef) videoFrame
         fromConnection: (AVCaptureConnection *) connection
{
    Q_UNUSED(captureOutput)
    Q_UNUSED(connection)

    if (!videoFrame)
        return;

    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(videoFrame);

    if (!imageBuffer)
        return;

    CMItemCount count;
    CMSampleTimingInfo timingInfo;
    qint64 pts = 0;
    AkFrac fps;

    if (CMSampleBufferGetOutputSampleTimingInfoArray(videoFrame,
                                                     1,
                                                     &timingInfo,
                                                     &count) == noErr) {
        pts = timingInfo.presentationTimeStamp.value;
        fps = AkFrac(timingInfo.presentationTimeStamp.timescale, 1);
    } else {
        pts = qint64(QTime::currentTime().msecsSinceStartOfDay()
                     * m_fps.value() / 1e3);
        fps = m_fps;
    }

    AkVideoPacket videoPacket({AkVideoCaps::Format_argb,
                               int(CVPixelBufferGetWidth(imageBuffer)),
                               int(CVPixelBufferGetHeight(imageBuffer)),
                               fps});

    CVPixelBufferLockBaseAddress(imageBuffer, 0);
    auto iData = reinterpret_cast<const quint8 *>(CVPixelBufferGetBaseAddress(imageBuffer));
    auto iLineSize = CVPixelBufferGetBytesPerRow(imageBuffer);
    auto lineSize = qMin<size_t>(videoPacket.lineSize(0),
                                 iLineSize);

    for (int y = 0; y < videoPacket.caps().height(); ++y) {
        auto srcLine = iData + y * iLineSize;
        auto dstLine = videoPacket.line(0, y);
        memcpy(dstLine, srcLine, lineSize);
    }

    CVPixelBufferUnlockBaseAddress(imageBuffer, 0);

    videoPacket.setPts(pts);
    videoPacket.setTimeBase(fps.invert());
    videoPacket.setIndex(0);
    videoPacket.setId(m_id);

    m_screenDev->frameReceived(videoPacket);
}

@end
