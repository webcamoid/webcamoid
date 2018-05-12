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

#ifndef ASSISTANTGLOBALS_H
#define ASSISTANTGLOBALS_H

#include <functional>
#include <xpc/xpc.h>

#define AKVCAM_ASSISTANT_NAME        "org.webcamoid.cmio.AkVCam.Assistant"
#define AKVCAM_ASSISTANT_CLIENT_NAME "org.webcamoid.cmio.AkVCam.Client"
#define AKVCAM_ASSISTANT_SERVER_NAME "org.webcamoid.cmio.AkVCam.Server"

// General messages
#define AKVCAM_ASSISTANT_MSG_REQUEST_PORT                0x000
#define AKVCAM_ASSISTANT_MSG_ADD_PORT                    0x001
#define AKVCAM_ASSISTANT_MSG_REMOVE_PORT                 0x002
#define AKVCAM_ASSISTANT_MSG_ISALIVE                     0x003

// Server messages
#define AKVCAM_ASSISTANT_MSG_DEVICE_CREATE               0x100
#define AKVCAM_ASSISTANT_MSG_DEVICE_CREATED              0x101
#define AKVCAM_ASSISTANT_MSG_DEVICE_DESTROY              0x102
#define AKVCAM_ASSISTANT_MSG_DEVICE_DESTROYED            0x103
#define AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING      0x104
#define AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING_CHANGED 0x105
#define AKVCAM_ASSISTANT_MSG_DEVICE_SETMIRRORING         0x106
#define AKVCAM_ASSISTANT_MSG_DEVICE_MIRRORING_CHANGED    0x107
#define AKVCAM_ASSISTANT_MSG_DEVICE_SETSCALING           0x108
#define AKVCAM_ASSISTANT_MSG_DEVICE_SCALING_CHANGED      0x109
#define AKVCAM_ASSISTANT_MSG_DEVICE_SETASPECTRATIO       0x10A
#define AKVCAM_ASSISTANT_MSG_DEVICE_ASPECTRATIO_CHANGED  0x10B
#define AKVCAM_ASSISTANT_MSG_FRAME_READY                 0x10C
#define AKVCAM_ASSISTANT_MSG_LISTENERS                   0x10D
#define AKVCAM_ASSISTANT_MSG_LISTENERS_CHANGED           0x10E

// Client messages
#define AKVCAM_ASSISTANT_MSG_DEVICES                     0x200
#define AKVCAM_ASSISTANT_MSG_DESCRIPTION                 0x201
#define AKVCAM_ASSISTANT_MSG_FORMATS                     0x202
#define AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING         0x203
#define AKVCAM_ASSISTANT_MSG_DEVICE_MIRRORING            0x204
#define AKVCAM_ASSISTANT_MSG_DEVICE_SCALING              0x205
#define AKVCAM_ASSISTANT_MSG_DEVICE_ASPECTRATIO          0x206
#define AKVCAM_ASSISTANT_MSG_ADD_LISTENER                0x207
#define AKVCAM_ASSISTANT_MSG_REMOVE_LISTENER             0x208

#define AKVCAM_BIND_FUNC(member) \
    std::bind(&member, this, std::placeholders::_1, std::placeholders::_2)

namespace AkVCam
{
    typedef std::function<void (xpc_connection_t,
                                xpc_object_t)> XpcMessage;
}

#endif // ASSISTANTGLOBALS_H
