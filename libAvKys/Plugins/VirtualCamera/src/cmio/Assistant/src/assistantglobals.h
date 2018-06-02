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
#define AKVCAM_ASSISTANT_MSG_ISALIVE                0x000
#define AKVCAM_ASSISTANT_MSG_FRAME_READY            0x001

// Assistant messages
#define AKVCAM_ASSISTANT_MSG_REQUEST_PORT           0x100
#define AKVCAM_ASSISTANT_MSG_ADD_PORT               0x101
#define AKVCAM_ASSISTANT_MSG_REMOVE_PORT            0x102

// Device control and information
#define AKVCAM_ASSISTANT_MSG_DEVICES                0x200
#define AKVCAM_ASSISTANT_MSG_DEVICE_CREATE          0x201
#define AKVCAM_ASSISTANT_MSG_DEVICE_DESTROY         0x202
#define AKVCAM_ASSISTANT_MSG_DEVICE_DESCRIPTION     0x203
#define AKVCAM_ASSISTANT_MSG_DEVICE_FORMATS         0x204

// Device listeners controls
#define AKVCAM_ASSISTANT_MSG_DEVICE_LISTENERS       0x300
#define AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER        0x301
#define AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_ADD    0x302
#define AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_REMOVE 0x303

// Device dynamic properties
#define AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING    0x400
#define AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING 0x401
#define AKVCAM_ASSISTANT_MSG_DEVICE_MIRRORING       0x402
#define AKVCAM_ASSISTANT_MSG_DEVICE_SETMIRRORING    0x403
#define AKVCAM_ASSISTANT_MSG_DEVICE_SCALING         0x404
#define AKVCAM_ASSISTANT_MSG_DEVICE_SETSCALING      0x405
#define AKVCAM_ASSISTANT_MSG_DEVICE_ASPECTRATIO     0x406
#define AKVCAM_ASSISTANT_MSG_DEVICE_SETASPECTRATIO  0x407
#define AKVCAM_ASSISTANT_MSG_DEVICE_SWAPRGB         0x408
#define AKVCAM_ASSISTANT_MSG_DEVICE_SETSWAPRGB      0x409

#define AKVCAM_BIND_FUNC(member) \
    std::bind(&member, this, std::placeholders::_1, std::placeholders::_2)

namespace AkVCam
{
    typedef std::function<void (xpc_connection_t,
                                xpc_object_t)> XpcMessage;
}

#endif // ASSISTANTGLOBALS_H
