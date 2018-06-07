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

#include <fstream>
#include <map>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <dirent.h>
#include <Foundation/Foundation.h>
#include <IOSurface/IOSurface.h>
#include <CoreMedia/CMFormatDescription.h>
#include <xpc/connection.h>

#include "ipcbridge.h"
#include "../../Assistant/src/assistantglobals.h"
#include "VCamUtils/src/utils.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/image/videoframe.h"

#define AkIpcBridgeLogMethod() \
    AkLoggerLog("IpcBridge::", __FUNCTION__, "()")

#define AKVCAM_BIND_FUNC(member) \
    std::bind(&member, this, std::placeholders::_1, std::placeholders::_2)

namespace AkVCam
{
    class IpcBridgePrivate
    {
        public:
            IpcBridge *parent;
            std::string portName;
            xpc_connection_t messagePort;
            xpc_connection_t serverMessagePort;
            IpcBridge::FrameReadyCallback frameReadyCallback;
            IpcBridge::DeviceChangedCallback deviceAddedCallback;
            IpcBridge::DeviceChangedCallback deviceRemovedCallback;
            IpcBridge::BroadcastingChangedCallback broadcastingChangedCallback;
            IpcBridge::MirrorChangedCallback mirrorChangedCallback;
            IpcBridge::ScalingChangedCallback scalingChangedCallback;
            IpcBridge::AspectRatioChangedCallback aspectRatioChangedCallback;
            IpcBridge::SwapRgbChangedCallback swapRgbChangedCallback;
            IpcBridge::ListenerCallback listenerAddedCallback;
            IpcBridge::ListenerCallback listenerRemovedCallback;
            std::map<int64_t, XpcMessage> m_messageHandlers;
            std::vector<std::string> broadcasting;
            std::map<std::string, std::string> options;
            bool uninstall;

            IpcBridgePrivate(IpcBridge *parent=nullptr);
            inline void add(IpcBridge *bridge);
            void remove(IpcBridge *bridge);
            inline std::vector<IpcBridge *> &bridges();

            // Message handling methods
            void isAlive(xpc_connection_t client, xpc_object_t event);
            void deviceCreate(xpc_connection_t client, xpc_object_t event);
            void deviceDestroy(xpc_connection_t client, xpc_object_t event);
            void frameReady(xpc_connection_t client, xpc_object_t event);
            void setBroadcasting(xpc_connection_t client,
                                     xpc_object_t event);
            void setMirror(xpc_connection_t client, xpc_object_t event);
            void setScaling(xpc_connection_t client, xpc_object_t event);
            void setAspectRatio(xpc_connection_t client, xpc_object_t event);
            void setSwapRgb(xpc_connection_t client, xpc_object_t event);
            void listenerAdd(xpc_connection_t client, xpc_object_t event);
            void listenerRemove(xpc_connection_t client, xpc_object_t event);
            void messageReceived(xpc_connection_t client, xpc_object_t event);

            // Utility methods
            std::string homePath() const;
            bool fileExists(const std::string &path) const;
            std::string fileName(const std::string &path) const;
            bool mkpath(const std::string &path) const;
            bool rm(const std::string &path) const;
            bool createDaemonPlist(const std::string &fileName) const;
            bool loadDaemon() const;
            void unloadDaemon() const;
            bool checkDaemon();
            void uninstallPlugin();

        private:
            std::vector<IpcBridge *> m_bridges;
    };

    inline IpcBridgePrivate *ipcBridgePrivate()
    {
        static IpcBridgePrivate ipcBridgePrivate;

        return &ipcBridgePrivate;
    }
}

AkVCam::IpcBridge::IpcBridge()
{
    AkIpcBridgeLogMethod();

    this->d = new IpcBridgePrivate(this);
    ipcBridgePrivate()->add(this);
}

AkVCam::IpcBridge::~IpcBridge()
{
    this->unregisterPeer();
    ipcBridgePrivate()->remove(this);
    delete this->d;
}

void AkVCam::IpcBridge::setOption(const std::string &key,
                                  const std::string &value)
{
    AkIpcBridgeLogMethod();

    if (value.empty())
        this->d->options.erase(key);
    else
        this->d->options[key] = value;
}

int AkVCam::IpcBridge::sudo(const std::vector<std::string> &parameters,
                            const std::map<std::string, std::string> &options)
{
    UNUSED(options)
    AkIpcBridgeLogMethod();

    std::stringstream ss;
    ss << "osascript -e \"do shell script \\\"";

    for (auto param: parameters) {
        if (param.find(' ') == std::string::npos)
            ss << param;
        else
            ss << "'" << param << "'";

        ss << ' ';
    }

    ss << "\\\" with administrator privileges\" 2>&1";
    auto sudo = popen(ss.str().c_str(), "r");

    if (!sudo)
        return -1;

    std::string output;
    char buffer[1024];

    while (fgets(buffer, 1024, sudo))
        output += std::string(buffer);

    auto result = pclose(sudo);

    if (result)
        AkLoggerLog(output);

    return result;
}

bool AkVCam::IpcBridge::registerPeer(bool asClient)
{
    AkIpcBridgeLogMethod();

    if (!asClient) {
        std::string plistFile =
                CMIO_DAEMONS_PATH "/" AKVCAM_ASSISTANT_NAME ".plist";

        auto daemon = replace(plistFile, "~", this->d->homePath());

        if (!this->d->fileExists(daemon))
            return false;
    }

    if (this->d->serverMessagePort)
        return true;

    xpc_object_t dictionary = nullptr;
    xpc_object_t reply = nullptr;
    std::string portName;
    xpc_connection_t messagePort = nullptr;
    xpc_type_t replyType;
    bool status = false;

    auto serverMessagePort =
            xpc_connection_create_mach_service(AKVCAM_ASSISTANT_NAME,
                                               NULL,
                                               0);

    if (!serverMessagePort)
        goto registerEndPoint_failed;

    xpc_connection_set_event_handler(serverMessagePort, ^(xpc_object_t) {});
    xpc_connection_resume(serverMessagePort);

    dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_REQUEST_PORT);
    xpc_dictionary_set_bool(dictionary, "client", asClient);
    reply = xpc_connection_send_message_with_reply_sync(serverMessagePort,
                                                        dictionary);
    xpc_release(dictionary);
    replyType = xpc_get_type(reply);

    if (replyType == XPC_TYPE_DICTIONARY)
        portName = xpc_dictionary_get_string(reply, "port");

    xpc_release(reply);

    if (replyType != XPC_TYPE_DICTIONARY)
        goto registerEndPoint_failed;

    messagePort = xpc_connection_create(NULL, NULL);

    if (!messagePort)
        goto registerEndPoint_failed;

    xpc_connection_set_event_handler(messagePort, ^(xpc_object_t event) {
        auto type = xpc_get_type(event);

        if (type == XPC_TYPE_ERROR)
            return;

        auto client = reinterpret_cast<xpc_connection_t>(event);

        xpc_connection_set_event_handler(client, ^(xpc_object_t event) {
            ipcBridgePrivate()->messageReceived(client, event);
        });

        xpc_connection_resume(client);
    });

    xpc_connection_resume(messagePort);

    dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_ADD_PORT);
    xpc_dictionary_set_string(dictionary, "port", portName.c_str());
    xpc_dictionary_set_connection(dictionary, "connection", messagePort);
    reply = xpc_connection_send_message_with_reply_sync(serverMessagePort,
                                                        dictionary);
    xpc_release(dictionary);
    replyType = xpc_get_type(reply);

    if (replyType == XPC_TYPE_DICTIONARY)
        status = xpc_dictionary_get_bool(reply, "status");

    xpc_release(reply);

    if (replyType != XPC_TYPE_DICTIONARY || !status)
        goto registerEndPoint_failed;

    this->d->portName = portName;
    this->d->messagePort = messagePort;
    this->d->serverMessagePort = serverMessagePort;

    AkLoggerLog("SUCCESSFUL");

    return true;

registerEndPoint_failed:
    if (messagePort)
        xpc_release(messagePort);

    if (serverMessagePort)
        xpc_release(serverMessagePort);

    AkLoggerLog("FAILED");

    return false;
}

void AkVCam::IpcBridge::unregisterPeer()
{
    AkIpcBridgeLogMethod();

    if (this->d->messagePort) {
        xpc_release(this->d->messagePort);
        this->d->messagePort = nullptr;
    }

    if (this->d->serverMessagePort) {
        if (!this->d->portName.empty()) {
            auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
            xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_REMOVE_PORT);
            xpc_dictionary_set_string(dictionary, "port", this->d->portName.c_str());
            xpc_connection_send_message(this->d->serverMessagePort,
                                        dictionary);
            xpc_release(dictionary);
        }

        xpc_release(this->d->serverMessagePort);
        this->d->serverMessagePort = nullptr;
    }

    this->d->portName.clear();
}

std::vector<std::string> AkVCam::IpcBridge::listDevices() const
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return {};

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICES);
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return {};
    }

    auto devicesList = xpc_dictionary_get_array(reply, "devices");
    std::vector<std::string> devices;

    for (size_t i = 0; i < xpc_array_get_count(devicesList); i++)
        devices.push_back(xpc_array_get_string(devicesList, i));

    xpc_release(reply);

    return devices;
}

std::string AkVCam::IpcBridge::description(const std::string &deviceId) const
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return {};

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_DESCRIPTION);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return {};
    }

    std::string description(xpc_dictionary_get_string(reply, "description"));
    xpc_release(reply);

    return description;
}

std::vector<AkVCam::PixelFormat> AkVCam::IpcBridge::supportedOutputPixelFormats() const
{
    return {
        PixelFormatRGB32,
        PixelFormatRGB24,
        PixelFormatUYVY,
        PixelFormatYUY2
    };
}

std::vector<AkVCam::VideoFormat> AkVCam::IpcBridge::formats(const std::string &deviceId) const
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return {};

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_FORMATS);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return {};
    }

    auto formatsList = xpc_dictionary_get_array(reply, "formats");
    std::vector<VideoFormat> formats;

    for (size_t i = 0; i < xpc_array_get_count(formatsList); i++) {
        auto format = xpc_array_get_dictionary(formatsList, i);
        auto fourcc = FourCC(xpc_dictionary_get_uint64(format, "fourcc"));
        auto width = int(xpc_dictionary_get_int64(format, "width"));
        auto height = int(xpc_dictionary_get_int64(format, "height"));
        double fps = xpc_dictionary_get_double(format, "fps");

        formats.push_back(VideoFormat(fourcc, width, height, {fps}));
    }

    xpc_release(reply);

    return formats;
}

std::string AkVCam::IpcBridge::broadcaster(const std::string &deviceId) const
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return {};

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return {};
    }

    std::string broadcaster = xpc_dictionary_get_string(reply, "broadcaster");
    xpc_release(reply);

    AkLoggerLog("Device: ", deviceId);
    AkLoggerLog("Broadcaster: ", broadcaster);

    return broadcaster;
}

bool AkVCam::IpcBridge::isHorizontalMirrored(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return false;

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_MIRRORING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return false;
    }

    bool horizontalMirror = xpc_dictionary_get_bool(reply, "hmirror");
    xpc_release(reply);

    return horizontalMirror;
}

bool AkVCam::IpcBridge::isVerticalMirrored(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return false;

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_MIRRORING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return false;
    }

    bool verticalMirror = xpc_dictionary_get_bool(reply, "vmirror");
    xpc_release(reply);

    return verticalMirror;
}

AkVCam::Scaling AkVCam::IpcBridge::scalingMode(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return ScalingFast;

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SCALING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return ScalingFast;
    }

    auto scaling = Scaling(xpc_dictionary_get_int64(reply, "scaling"));
    xpc_release(reply);

    return scaling;
}

AkVCam::AspectRatio AkVCam::IpcBridge::aspectRatioMode(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return AspectRatioIgnore;

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_ASPECTRATIO);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return AspectRatioIgnore;
    }

    auto aspectRatio = AspectRatio(xpc_dictionary_get_int64(reply, "aspect"));
    xpc_release(reply);

    return aspectRatio;
}

bool AkVCam::IpcBridge::swapRgb(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return false;

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SWAPRGB);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return false;
    }

    auto swap = xpc_dictionary_get_bool(reply, "swap");
    xpc_release(reply);

    return swap;
}

std::vector<std::string> AkVCam::IpcBridge::listeners(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return {};

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_LISTENERS);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return {};
    }

    auto listenersList = xpc_dictionary_get_array(reply, "listeners");
    std::vector<std::string> listeners;

    for (size_t i = 0; i < xpc_array_get_count(listenersList); i++)
        listeners.push_back(xpc_array_get_string(listenersList, i));

    xpc_release(reply);

    AkLoggerLog("Device: ", deviceId);
    AkLoggerLog("Listeners: ", listeners.size());

    return listeners;
}

std::string AkVCam::IpcBridge::deviceCreate(const std::string &description,
                                            const std::vector<VideoFormat> &formats)
{
    AkIpcBridgeLogMethod();

    if (!this->d->checkDaemon())
        return {};

    this->registerPeer(false);

    if (!this->d->serverMessagePort || !this->d->messagePort)
        return {};

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_CREATE);
    xpc_dictionary_set_string(dictionary, "port", this->d->portName.c_str());
    xpc_dictionary_set_string(dictionary, "description", description.c_str());
    auto formatsList = xpc_array_create(NULL, 0);

    for (auto &format: formats) {
        auto dictFormat = xpc_dictionary_create(nullptr, nullptr, 0);
        xpc_dictionary_set_uint64(dictFormat, "fourcc", format.fourcc());
        xpc_dictionary_set_int64(dictFormat, "width", format.width());
        xpc_dictionary_set_int64(dictFormat, "height", format.height());
        xpc_dictionary_set_double(dictFormat, "fps", format.minimumFrameRate());
        xpc_array_append_value(formatsList, dictFormat);
    }

    xpc_dictionary_set_value(dictionary, "formats", formatsList);

    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return {};
    }

    std::string deviceId(xpc_dictionary_get_string(reply, "device"));
    xpc_release(reply);

    return deviceId;
}

void AkVCam::IpcBridge::deviceDestroy(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return;

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_DESTROY);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_connection_send_message(this->d->serverMessagePort,
                                dictionary);
    xpc_release(dictionary);

    // If no devices are registered
    if (this->d->uninstall && listDevices().empty())
        this->d->uninstallPlugin();
}

bool AkVCam::IpcBridge::changeDescription(const std::string &deviceId,
                                          const std::string &description)
{
    AkIpcBridgeLogMethod();

    auto formats = this->formats(deviceId);

    if (formats.empty())
        return false;

    this->d->uninstall = false;
    this->deviceDestroy(deviceId);
    this->d->uninstall = true;

    if (this->deviceCreate(description.empty()?
                               "AvKys Virtual Camera":
                               description,
                           formats).empty())
        return false;

    return true;
}

bool AkVCam::IpcBridge::destroyAllDevices()
{
    AkIpcBridgeLogMethod();

    for (auto &device: this->listDevices())
        this->deviceDestroy(device);

    return true;
}

bool AkVCam::IpcBridge::deviceStart(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return false;

    auto it = std::find(this->d->broadcasting.begin(),
                        this->d->broadcasting.end(),
                        deviceId);

    if (it != this->d->broadcasting.end())
        return false;

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_string(dictionary, "broadcaster", this->d->portName.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return false;
    }

    bool status = xpc_dictionary_get_bool(reply, "status");
    xpc_release(reply);
    this->d->broadcasting.push_back(deviceId);

    return status;
}

void AkVCam::IpcBridge::deviceStop(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return;

    auto it = std::find(this->d->broadcasting.begin(),
                        this->d->broadcasting.end(),
                        deviceId);

    if (it == this->d->broadcasting.end())
        return;

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_string(dictionary, "broadcaster", "");
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    xpc_release(reply);
    this->d->broadcasting.erase(it);
}

bool AkVCam::IpcBridge::write(const std::string &deviceId,
                              const VideoFrame &frame)
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return false;

    auto it = std::find(this->d->broadcasting.begin(),
                        this->d->broadcasting.end(),
                        deviceId);

    if (it == this->d->broadcasting.end())
        return false;

    std::vector<CFStringRef> keys {
        kIOSurfacePixelFormat,
        kIOSurfaceWidth,
        kIOSurfaceHeight,
        kIOSurfaceAllocSize
    };

    auto fourcc = frame.format().fourcc();
    auto width = frame.format().width();
    auto height = frame.format().height();
    auto dataSize = int64_t(frame.dataSize());

    std::vector<CFNumberRef> values {
        CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &fourcc),
        CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &width),
        CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &height),
        CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &dataSize)
    };

    auto surfaceProperties =
            CFDictionaryCreate(kCFAllocatorDefault,
                               reinterpret_cast<const void **>(keys.data()),
                               reinterpret_cast<const void **>(values.data()),
                               CFIndex(values.size()),
                               NULL,
                               NULL);
    auto surface = IOSurfaceCreate(surfaceProperties);

    for (auto value: values)
        CFRelease(value);

    CFRelease(surfaceProperties);

    if (!surface)
        return false;

    uint32_t surfaceSeed = 0;
    IOSurfaceLock(surface, 0, &surfaceSeed);
    auto data = IOSurfaceGetBaseAddress(surface);
    memcpy(data, frame.data().get(), frame.dataSize());
    IOSurfaceUnlock(surface, 0, &surfaceSeed);

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_FRAME_READY);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_value(dictionary, "frame", IOSurfaceCreateXPCObject(surface));
    xpc_connection_send_message(this->d->serverMessagePort,
                                dictionary);
    xpc_release(dictionary);
    CFRelease(surface);

    return true;
}

void AkVCam::IpcBridge::setMirroring(const std::string &deviceId,
                                     bool horizontalMirrored,
                                     bool verticalMirrored)
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return;

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETMIRRORING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_bool(dictionary, "hmirror", horizontalMirrored);
    xpc_dictionary_set_bool(dictionary, "vmirror", verticalMirrored);
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    xpc_release(reply);
}

void AkVCam::IpcBridge::setScaling(const std::string &deviceId,
                                   Scaling scaling)
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return;

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETSCALING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_int64(dictionary, "scaling", scaling);
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    xpc_release(reply);
}

void AkVCam::IpcBridge::setAspectRatio(const std::string &deviceId,
                                       AspectRatio aspectRatio)
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return;

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETASPECTRATIO);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_int64(dictionary, "aspect", aspectRatio);
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    xpc_release(reply);
}

void AkVCam::IpcBridge::setSwapRgb(const std::string &deviceId, bool swap)
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return;

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETSWAPRGB);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_bool(dictionary, "swap", swap);
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    xpc_release(reply);
}

void AkVCam::IpcBridge::setListenerAddedCallback(ListenerCallback callback)
{
    this->d->listenerAddedCallback = callback;
}

void AkVCam::IpcBridge::setListenerRemovedCallback(ListenerCallback callback)
{
    this->d->listenerRemovedCallback = callback;
}

bool AkVCam::IpcBridge::addListener(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return false;

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_ADD);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_string(dictionary, "listener", this->d->portName.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return false;
    }

    bool status = xpc_dictionary_get_bool(reply, "status");
    xpc_release(reply);

    return status;
}

bool AkVCam::IpcBridge::removeListener(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    if (!this->d->serverMessagePort)
        return true;

    auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_REMOVE);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_string(dictionary, "listener", this->d->portName.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return true;
    }

    bool status = xpc_dictionary_get_bool(reply, "status");
    xpc_release(reply);

    return status;
}

void AkVCam::IpcBridge::setFrameReadyCallback(FrameReadyCallback callback)
{
    this->d->frameReadyCallback = callback;
}

void AkVCam::IpcBridge::setDeviceAddedCallback(DeviceChangedCallback callback)
{
    this->d->deviceAddedCallback = callback;
}

void AkVCam::IpcBridge::setDeviceRemovedCallback(DeviceChangedCallback callback)
{
    this->d->deviceRemovedCallback = callback;
}

void AkVCam::IpcBridge::setBroadcastingChangedCallback(BroadcastingChangedCallback callback)
{
    this->d->broadcastingChangedCallback = callback;
}

void AkVCam::IpcBridge::setMirrorChangedCallback(MirrorChangedCallback callback)
{
    this->d->mirrorChangedCallback = callback;
}

void AkVCam::IpcBridge::setScalingChangedCallback(ScalingChangedCallback callback)
{
    this->d->scalingChangedCallback = callback;
}

void AkVCam::IpcBridge::setAspectRatioChangedCallback(AspectRatioChangedCallback callback)
{
    this->d->aspectRatioChangedCallback = callback;
}

void AkVCam::IpcBridge::setSwapRgbChangedCallback(SwapRgbChangedCallback callback)
{
    this->d->swapRgbChangedCallback = callback;
}

AkVCam::IpcBridgePrivate::IpcBridgePrivate(IpcBridge *parent):
    parent(parent),
    messagePort(nullptr),
    serverMessagePort(nullptr),
    uninstall(true)
{
    this->m_messageHandlers = {
        {AKVCAM_ASSISTANT_MSG_ISALIVE               , AKVCAM_BIND_FUNC(IpcBridgePrivate::isAlive)        },
        {AKVCAM_ASSISTANT_MSG_FRAME_READY           , AKVCAM_BIND_FUNC(IpcBridgePrivate::frameReady)     },
        {AKVCAM_ASSISTANT_MSG_DEVICE_CREATE         , AKVCAM_BIND_FUNC(IpcBridgePrivate::deviceCreate)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_DESTROY        , AKVCAM_BIND_FUNC(IpcBridgePrivate::deviceDestroy)  },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_ADD   , AKVCAM_BIND_FUNC(IpcBridgePrivate::listenerAdd)    },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_REMOVE, AKVCAM_BIND_FUNC(IpcBridgePrivate::listenerRemove) },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING, AKVCAM_BIND_FUNC(IpcBridgePrivate::setBroadcasting)},
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETMIRRORING   , AKVCAM_BIND_FUNC(IpcBridgePrivate::setMirror)      },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETSCALING     , AKVCAM_BIND_FUNC(IpcBridgePrivate::setScaling)     },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETASPECTRATIO , AKVCAM_BIND_FUNC(IpcBridgePrivate::setAspectRatio) },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETSWAPRGB     , AKVCAM_BIND_FUNC(IpcBridgePrivate::setSwapRgb)     },
    };
}

void AkVCam::IpcBridgePrivate::add(IpcBridge *bridge)
{
    this->m_bridges.push_back(bridge);
}

void AkVCam::IpcBridgePrivate::remove(IpcBridge *bridge)
{
    for (size_t i = 0; i < this->m_bridges.size(); i++)
        if (this->m_bridges[i] == bridge) {
            this->m_bridges.erase(this->m_bridges.begin() + long(i));

            break;
        }
}

std::vector<AkVCam::IpcBridge *> &AkVCam::IpcBridgePrivate::bridges()
{
    return this->m_bridges;
}

void AkVCam::IpcBridgePrivate::isAlive(xpc_connection_t client,
                                       xpc_object_t event)
{
    AkLoggerLog("IpcBridgePrivate::isAlive");

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "alive", true);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::IpcBridgePrivate::deviceCreate(xpc_connection_t client,
                                            xpc_object_t event)
{
    UNUSED(client)
    AkLoggerLog("IpcBridgePrivate::deviceCreated");

    std::string device = xpc_dictionary_get_string(event, "device");

    for (auto bridge: this->m_bridges)
        if (bridge->d->deviceAddedCallback)
            bridge->d->deviceAddedCallback(device);
}

void AkVCam::IpcBridgePrivate::deviceDestroy(xpc_connection_t client,
                                             xpc_object_t event)
{
    UNUSED(client)
    AkLoggerLog("IpcBridgePrivate::deviceDestroyed");

    std::string device = xpc_dictionary_get_string(event, "device");

    for (auto bridge: this->m_bridges)
        if (bridge->d->deviceRemovedCallback)
            bridge->d->deviceRemovedCallback(device);
}

void AkVCam::IpcBridgePrivate::frameReady(xpc_connection_t client,
                                          xpc_object_t event)
{
    UNUSED(client)
    AkLoggerLog("IpcBridgePrivate::frameReady");

    std::string deviceId =
            xpc_dictionary_get_string(event, "device");
    auto frame = xpc_dictionary_get_value(event, "frame");
    auto surface = IOSurfaceLookupFromXPCObject(frame);

    if (!surface)
        return;

    uint32_t surfaceSeed = 0;
    IOSurfaceLock(surface, kIOSurfaceLockReadOnly, &surfaceSeed);
    FourCC fourcc = IOSurfaceGetPixelFormat(surface);
    int width = int(IOSurfaceGetWidth(surface));
    int height = int(IOSurfaceGetHeight(surface));
    size_t size = IOSurfaceGetAllocSize(surface);
    auto data = reinterpret_cast<uint8_t *>(IOSurfaceGetBaseAddress(surface));
    AkVCam::VideoFrame videoFrame({fourcc, width, height},
                                  data, size);
    IOSurfaceUnlock(surface, kIOSurfaceLockReadOnly, &surfaceSeed);
    CFRelease(surface);

    for (auto bridge: this->m_bridges)
        if (bridge->d->frameReadyCallback)
            bridge->d->frameReadyCallback(deviceId, videoFrame);
}

void AkVCam::IpcBridgePrivate::setBroadcasting(xpc_connection_t client,
                                               xpc_object_t event)
{
    UNUSED(client)
    AkLoggerLog("IpcBridgePrivate::broadcastingChanged");

    std::string deviceId =
            xpc_dictionary_get_string(event, "device");
    std::string broadcaster =
            xpc_dictionary_get_string(event, "broadcaster");

    for (auto bridge: this->m_bridges)
        if (bridge->d->broadcastingChangedCallback)
            bridge->d->broadcastingChangedCallback(deviceId, broadcaster);
}

void AkVCam::IpcBridgePrivate::setMirror(xpc_connection_t client,
                                         xpc_object_t event)
{
    UNUSED(client)
    AkLoggerLog("IpcBridgePrivate::mirrorChanged");

    std::string deviceId =
            xpc_dictionary_get_string(event, "device");
    bool horizontalMirror =
            xpc_dictionary_get_bool(event, "hmirror");
    bool verticalMirror =
            xpc_dictionary_get_bool(event, "vmirror");

    for (auto bridge: this->m_bridges)
        if (bridge->d->mirrorChangedCallback)
            bridge->d->mirrorChangedCallback(deviceId,
                                             horizontalMirror,
                                             verticalMirror);
}

void AkVCam::IpcBridgePrivate::setScaling(xpc_connection_t client,
                                          xpc_object_t event)
{
    UNUSED(client)
    AkLoggerLog("IpcBridgePrivate::scalingChanged");

    std::string deviceId =
            xpc_dictionary_get_string(event, "device");
    auto scaling =
            Scaling(xpc_dictionary_get_int64(event, "scaling"));

    for (auto bridge: this->m_bridges)
        if (bridge->d->scalingChangedCallback)
            bridge->d->scalingChangedCallback(deviceId, scaling);
}

void AkVCam::IpcBridgePrivate::setAspectRatio(xpc_connection_t client,
                                              xpc_object_t event)
{
    UNUSED(client)
    AkLoggerLog("IpcBridgePrivate::aspectRatioChanged");

    std::string deviceId =
            xpc_dictionary_get_string(event, "device");
    auto aspectRatio =
            AspectRatio(xpc_dictionary_get_int64(event, "aspect"));

    for (auto bridge: this->m_bridges)
        if (bridge->d->aspectRatioChangedCallback)
            bridge->d->aspectRatioChangedCallback(deviceId, aspectRatio);
}

void AkVCam::IpcBridgePrivate::setSwapRgb(xpc_connection_t client,
                                          xpc_object_t event)
{
    UNUSED(client)
    AkLoggerLog("IpcBridgePrivate::swapRgbChanged");

    std::string deviceId =
            xpc_dictionary_get_string(event, "device");
    auto swap = xpc_dictionary_get_bool(event, "swap");

    for (auto bridge: this->m_bridges)
        if (bridge->d->swapRgbChangedCallback)
            bridge->d->swapRgbChangedCallback(deviceId, swap);
}

void AkVCam::IpcBridgePrivate::listenerAdd(xpc_connection_t client,
                                           xpc_object_t event)
{
    UNUSED(client)
    AkLoggerLog("IpcBridgePrivate::listenerAdded");

    std::string deviceId = xpc_dictionary_get_string(event, "device");
    std::string listener = xpc_dictionary_get_string(event, "listener");

    for (auto bridge: this->m_bridges)
        if (bridge->d->listenerAddedCallback)
            bridge->d->listenerAddedCallback(deviceId, listener);
}

void AkVCam::IpcBridgePrivate::listenerRemove(xpc_connection_t client,
                                              xpc_object_t event)
{
    UNUSED(client)
    AkLoggerLog("IpcBridgePrivate::listenerRemoved");

    std::string deviceId = xpc_dictionary_get_string(event, "device");
    std::string listener = xpc_dictionary_get_string(event, "listener");

    for (auto bridge: this->m_bridges)
        if (bridge->d->listenerRemovedCallback)
            bridge->d->listenerRemovedCallback(deviceId, listener);
}

void AkVCam::IpcBridgePrivate::messageReceived(xpc_connection_t client,
                                               xpc_object_t event)
{
    auto type = xpc_get_type(event);

    if (type == XPC_TYPE_ERROR) {
        auto description = xpc_copy_description(event);
        AkLoggerLog("ERROR: ", description);
        free(description);
    } else if (type == XPC_TYPE_DICTIONARY) {
        auto message = xpc_dictionary_get_int64(event, "message");

        if (this->m_messageHandlers.count(message))
            this->m_messageHandlers[message](client, event);
    }
}

std::string AkVCam::IpcBridgePrivate::homePath() const
{
    auto homePath = NSHomeDirectory();

    if (!homePath)
        return {};

    return std::string(homePath.UTF8String);
}

bool AkVCam::IpcBridgePrivate::fileExists(const std::string &path) const
{
    struct stat stats;
    memset(&stats, 0, sizeof(struct stat));

    return stat(path.c_str(), &stats) == 0;
}

std::string AkVCam::IpcBridgePrivate::fileName(const std::string &path) const
{
    return path.substr(path.rfind("/") + 1);
}

bool AkVCam::IpcBridgePrivate::mkpath(const std::string &path) const
{
    if (path.empty())
        return false;

    if (this->fileExists(path))
        return true;

    // Create parent folders
    for (auto pos = path.find('/');
         pos != std::string::npos;
         pos = path.find('/', pos + 1)) {
        auto path_ = path.substr(0, pos);

        if (path_.empty() || this->fileExists(path_))
            continue;

        if (mkdir(path_.c_str(),
                  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
            return false;
    }

    return !mkdir(path.c_str(),
                  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
}

bool AkVCam::IpcBridgePrivate::rm(const std::string &path) const
{
    if (path.empty())
        return false;

    struct stat stats;
    memset(&stats, 0, sizeof(struct stat));

    if (stat(path.c_str(), &stats))
        return false;

    bool ok = true;

    if (S_ISDIR(stats.st_mode)) {
        auto dir = opendir(path.c_str());

        while (auto entry = readdir(dir))
            if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
                this->rm(entry->d_name);

        closedir(dir);

        ok &= !rmdir(path.c_str());
    } else {
        ok &= !::remove(path.c_str());
    }

    return ok;
}

bool AkVCam::IpcBridgePrivate::createDaemonPlist(const std::string &fileName) const
{
    std::fstream plistFile;
    plistFile.open(fileName, std::ios_base::out);

    if (!plistFile.is_open())
        return false;

    plistFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl
              << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
              << "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"
              << std::endl
              << "<plist version=\"1.0\">" << std::endl
              << "    <dict>" << std::endl
              << "        <key>Label</key>" << std::endl
              << "        <string>" << AKVCAM_ASSISTANT_NAME
                                    << "</string>" << std::endl
              << "        <key>ProgramArguments</key>" << std::endl
              << "        <array>" << std::endl
              << "            <string>" << CMIO_PLUGINS_DAL_PATH
                                        << "/"
                                        << CMIO_PLUGIN_NAME
                                        << ".plugin/Contents/Resources/"
                                        << CMIO_PLUGIN_ASSISTANT_NAME
                                        << "</string>" << std::endl
              << "            <string>--timeout</string>" << std::endl
              << "            <string>300.0</string>" << std::endl
              << "        </array>" << std::endl
              << "        <key>MachServices</key>" << std::endl
              << "        <dict>" << std::endl
              << "            <key>" << AKVCAM_ASSISTANT_NAME
                                     << "</key>" << std::endl
              << "            <true/>" << std::endl
              << "        </dict>" << std::endl;

#ifdef QT_DEBUG
    std::string daemonLog = "/tmp/" AKVCAM_ASSISTANT_NAME ".log";

    plistFile << "        <key>StandardOutPath</key>" << std::endl
              << "        <string>" << daemonLog << "</string>" << std::endl
              << "        <key>StandardErrorPath</key>" << std::endl
              << "        <string>" << daemonLog << "</string>" << std::endl;
#endif

    plistFile << "    </dict>" << std::endl
              << "</plist>" << std::endl;

    return true;
}

bool AkVCam::IpcBridgePrivate::loadDaemon() const
{
    auto launchctl = popen("launchctl list " AKVCAM_ASSISTANT_NAME, "r");

    if (launchctl && !pclose(launchctl))
        return true;

    auto daemonsPath = replace(CMIO_DAEMONS_PATH, "~", this->homePath());
    auto dstDaemonsPath = daemonsPath + "/" AKVCAM_ASSISTANT_NAME ".plist";

    if (!this->fileExists(dstDaemonsPath))
        return false;

    launchctl = popen(("launchctl load -w '" + dstDaemonsPath + "'").c_str(),
                      "r");

    return launchctl && !pclose(launchctl);
}

void AkVCam::IpcBridgePrivate::unloadDaemon() const
{
    std::string daemonPlist = AKVCAM_ASSISTANT_NAME ".plist";
    auto daemonsPath = replace(CMIO_DAEMONS_PATH, "~", this->homePath());
    auto dstDaemonsPath = daemonsPath + "/" + daemonPlist;

    if (!this->fileExists(dstDaemonsPath))
        return ;

    auto launchctl =
            popen(("launchctl unload -w '" + dstDaemonsPath + "'").c_str(),
                  "r");
    pclose(launchctl);
}

bool AkVCam::IpcBridgePrivate::checkDaemon()
{
    auto driverPath = replace(this->options["driverPath"], "\\", "/");
    auto plugin = this->fileName(driverPath);
    std::string dstPath = CMIO_PLUGINS_DAL_PATH;
    std::string pluginInstallPath = dstPath + "/" + plugin;

    if (!this->fileExists(pluginInstallPath))
        if (this->parent->sudo({"cp", "-rvf", driverPath, dstPath}))
            return false;

    auto daemonsPath = replace(CMIO_DAEMONS_PATH, "~", this->homePath());
    auto dstDaemonsPath = daemonsPath + "/" + AKVCAM_ASSISTANT_NAME + ".plist";

    if (!this->fileExists(dstDaemonsPath)) {
        if (!this->mkpath(daemonsPath))
            return false;

        if (!this->createDaemonPlist(dstDaemonsPath))
            return false;
    }

    return this->loadDaemon();
}

void AkVCam::IpcBridgePrivate::uninstallPlugin()
{
    // Stop the daemon
    this->unloadDaemon();

    // Remove the agent plist
    auto daemonsPath =
            replace(CMIO_DAEMONS_PATH, "~", this->homePath());
    this->rm(daemonsPath + "/" AKVCAM_ASSISTANT_NAME ".plist");

    // Remove the plugin
    auto driverPath = replace(this->options["driverPath"], "\\", "/");
    auto plugin = this->fileName(driverPath);
    std::string dstPath = CMIO_PLUGINS_DAL_PATH;
    this->parent->sudo({"rm", "-rvf", dstPath + "/" + plugin});
}
