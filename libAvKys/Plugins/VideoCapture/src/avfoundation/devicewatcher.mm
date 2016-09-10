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

#include "devicewatcher.h"

@implementation DeviceWatcher

- (id) initWithCaptureObject: (Capture *) object
{
    self = [super init];

    if (!self)
        return nil;

    m_capture = object;

    return self;
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
