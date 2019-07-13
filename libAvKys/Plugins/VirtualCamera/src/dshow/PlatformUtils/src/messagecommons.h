/* Webcamoid, webcam capture application.
 * Copyright (C) 2018  Gonzalo Exequiel Pedone
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

#ifndef MESSAGECOMMONS_H
#define MESSAGECOMMONS_H

#include <cstdint>
#include <cstring>
#include <functional>

#include "VCamUtils/src/image/videoframetypes.h"

#define AKVCAM_ASSISTANT_CLIENT_NAME "AkVCam_Client"
#define AKVCAM_ASSISTANT_SERVER_NAME "AkVCam_Server"

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

#define MSG_BUFFER_SIZE 4096
#define MAX_STRING 1024

#define AKVCAM_BIND_FUNC(member) \
    std::bind(&member, this, std::placeholders::_1)

namespace AkVCam
{
    struct Frame
    {
        uint32_t format;
        int32_t width;
        int32_t height;
        uint32_t size;
        uint8_t data[4];
    };

    struct Message
    {
        uint32_t messageId;
        uint32_t dataSize;
        uint8_t data[MSG_BUFFER_SIZE];

        Message():
            messageId(0),
            dataSize(0)
        {
            memset(this->data, 0, MSG_BUFFER_SIZE);
        }

        Message(const Message &other):
            messageId(other.messageId),
            dataSize(other.dataSize)
        {
            memcpy(this->data, other.data, MSG_BUFFER_SIZE);
        }

        Message(const Message *other):
            messageId(other->messageId),
            dataSize(other->dataSize)
        {
            memcpy(this->data, other->data, MSG_BUFFER_SIZE);
        }

        Message &operator =(const Message &other)
        {
            if (this != &other) {
                this->messageId = other.messageId;
                this->dataSize = other.dataSize;
                memcpy(this->data, other.data, MSG_BUFFER_SIZE);
            }

            return *this;
        }

        inline void clear()
        {
            this->messageId = 0;
            this->dataSize = 0;
            memset(this->data, 0, MSG_BUFFER_SIZE);
        }
    };

    template<typename T>
    inline T *messageData(Message *message)
    {
        return reinterpret_cast<T *>(message->data);
    }

    using MessageHandler = std::function<void (Message *message)>;

    struct MsgRequestPort
    {
        bool client;
        char port[MAX_STRING];
    };

    struct MsgAddPort
    {
        char port[MAX_STRING];
        char pipeName[MAX_STRING];
        bool status;
    };

    struct MsgRemovePort
    {
        char port[MAX_STRING];
    };

    struct MsgBroadcasting
    {
        char device[MAX_STRING];
        char broadcaster[MAX_STRING];
        bool status;
    };

    struct MsgMirroring
    {
        char device[MAX_STRING];
        bool hmirror;
        bool vmirror;
        bool status;
    };

    struct MsgScaling
    {
        char device[MAX_STRING];
        Scaling scaling;
        bool status;
    };

    struct MsgAspectRatio
    {
        char device[MAX_STRING];
        AspectRatio aspect;
        bool status;
    };

    struct MsgSwapRgb
    {
        char device[MAX_STRING];
        bool swap;
        bool status;
    };

    struct MsgListeners
    {
        char device[MAX_STRING];
        char listener[MAX_STRING];
        size_t nlistener;
        bool status;
    };

    struct MsgIsAlive
    {
        bool alive;
    };

    struct MsgFrameReady
    {
        char device[MAX_STRING];
        char port[MAX_STRING];
    };
}

#endif // MESSAGECOMMONS_H
