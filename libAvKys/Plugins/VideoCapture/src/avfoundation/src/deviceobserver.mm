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

#include "deviceobserver.h"

@implementation DeviceObserver

- (id) initWithCaptureObject: (CaptureAvFoundation *) object
{
    self = [super init];

    if (!self)
        return nil;

    m_capture = object;

    return self;
}

- (void) captureOutput: (AVCaptureOutput *) captureOutput
         didOutputSampleBuffer: (CMSampleBufferRef) videoFrame
         fromConnection: (AVCaptureConnection *) connection
{
    Q_UNUSED(captureOutput)
    Q_UNUSED(connection)

    m_capture->mutex().lock();

    CMSampleBufferRef *frame =
            reinterpret_cast<CMSampleBufferRef *>(m_capture->curFrame());

    if (!frame) {
        m_capture->mutex().unlock();

        return;
    }

    if (*frame)
        CFRelease(*frame);

    *frame = (CMSampleBufferRef) CFRetain(videoFrame);
    m_capture->frameReady().wakeAll();
    m_capture->mutex().unlock();
}

- (void) cameraConnected: (NSNotification *) notification
{
    Q_UNUSED(notification)

    if (!m_capture)
        return;

    dispatch_async(dispatch_get_main_queue(),
                   ^{
                       if (m_capture)
                           m_capture->cameraConnected();
                   });
}

- (void) cameraDisconnected: (NSNotification *) notification
{
    Q_UNUSED(notification)

    if (!m_capture)
        return;

    dispatch_async(dispatch_get_main_queue(),
                   ^{
                       if (m_capture)
                           m_capture->cameraDisconnected();
                   });
}

- (void) disconnect
{
    [NSObject cancelPreviousPerformRequestsWithTarget: self];
    m_capture = NULL;
}

@end
