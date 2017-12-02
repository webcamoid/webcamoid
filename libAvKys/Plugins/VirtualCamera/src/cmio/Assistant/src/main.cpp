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

#include <iostream>

#include "assistant.h"

int main()
{
    AkVCam::Assistant assistant;
    CFMessagePortContext context {0, &assistant, nullptr, nullptr, nullptr};

    auto messagePort =
            CFMessagePortCreateLocal(kCFAllocatorDefault,
                                     CFSTR(AKVCAM_ASSISTANT_NAME),
                                     assistant.messageReceived,
                                     &context,
                                     nullptr);

    if (messagePort) {
        auto runLoopSource =
                CFMessagePortCreateRunLoopSource(kCFAllocatorDefault,
                                                 messagePort,
                                                 0);

        if (runLoopSource) {
            CFRunLoopAddSource(CFRunLoopGetMain(),
                               runLoopSource,
                               kCFRunLoopCommonModes);
            CFRunLoopRun();

            CFRunLoopRemoveSource(CFRunLoopGetMain(),
                                  runLoopSource,
                                  kCFRunLoopCommonModes);
            CFRelease(runLoopSource);
        }

        CFMessagePortInvalidate(messagePort);
        CFRelease(messagePort);
    }

    return 0;
}
