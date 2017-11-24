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

#ifndef ASSISTANTGLOBALS_H
#define ASSISTANTGLOBALS_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>

#include <CoreFoundation/CFMessagePort.h>

#include "VCamUtils/src/image/videoformat.h"

#define AKVCAM_ASSISTANT_NAME        "org.webcamoid.cmio.AkVCam.Assistant"
#define AKVCAM_ASSISTANT_CLIENT_NAME "org.webcamoid.cmio.AkVCam.Client"
#define AKVCAM_ASSISTANT_SERVER_NAME "org.webcamoid.cmio.AkVCam.Server"

// General messages
#define AKVCAM_ASSISTANT_MSG_REQUEST_PORT     0x0
#define AKVCAM_ASSISTANT_MSG_ADD_PORT         0x1
#define AKVCAM_ASSISTANT_MSG_REMOVE_PORT      0x2

// Server messages
#define AKVCAM_ASSISTANT_MSG_DEVICE_CREATE    0x100
#define AKVCAM_ASSISTANT_MSG_DEVICE_CREATED   0x101
#define AKVCAM_ASSISTANT_MSG_DEVICE_DESTROY   0x102
#define AKVCAM_ASSISTANT_MSG_DEVICE_DESTROYED 0x103
#define AKVCAM_ASSISTANT_MSG_FRAME_READY      0x104

// Client messages
#define AKVCAM_ASSISTANT_MSG_DEVICES          0x200
#define AKVCAM_ASSISTANT_MSG_DESCRIPTION      0x201
#define AKVCAM_ASSISTANT_MSG_FORMATS          0x202

namespace AkVCam
{
    struct AssistantDevice
    {
        std::string deviceId;
        std::string description;
        std::vector<VideoFormat> formats;
    };

    struct AssistantServer
    {
        CFMessagePortRef messagePort;
        std::vector<AssistantDevice> devices;
    };

    typedef std::map<std::string, AssistantServer> AssistantServers;
    typedef std::map<std::string, CFMessagePortRef> AssistantClients;
}

#endif // ASSISTANTGLOBALS_H
