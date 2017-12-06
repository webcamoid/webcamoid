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

#include <sstream>

#include "ipcbridge.h"
#include "../../Assistant/src/assistantglobals.h"
#include "VCamUtils/src/image/videoformat.h"

namespace AkVCam
{
    class IpcBridgePrivate;
    IpcBridgePrivate *ipcBridgePrivate();

    class IpcBridgePrivate
    {
        public:
            IpcBridge *parent;
            CFMessagePortRef messagePort;
            CFMessagePortRef serverMessagePort;
            CFRunLoopSourceRef runLoopSource;
            std::vector<std::string> devices;
            IpcBridge::FrameReadCallback frameReadCallback;
            IpcBridge::DeviceChangedCallback deviceAddedCallback;
            IpcBridge::DeviceChangedCallback deviceRemovedCallback;

            IpcBridgePrivate(IpcBridge *parent=nullptr):
                parent(parent),
                messagePort(nullptr),
                serverMessagePort(nullptr),
                runLoopSource(nullptr)
            {
                this->startAssistant();
            }

            inline bool startAssistant()
            {
                std::stringstream launchctl;

                launchctl << "launchctl list " << AKVCAM_ASSISTANT_NAME;
                int result = system(launchctl.str().c_str());

                if (result != 0) {
                    launchctl.str("");
                    launchctl << "launchctl load -w "
                              << CMIO_DAEMONS_PATH
                              << "/"
                              << AKVCAM_ASSISTANT_NAME
                              << ".plist";
                    result = system(launchctl.str().c_str());

                    return result == 0;
                }

                return true;
            }

            inline void add(IpcBridge *bridge)
            {
                this->m_bridges.push_back(bridge);
            }

            inline void remove(IpcBridge *bridge)
            {
                for (size_t i = 0; i < this->m_bridges.size(); i++)
                    if (this->m_bridges[i] == bridge) {
                        this->m_bridges.erase(this->m_bridges.begin()
                                              + long(i));

                        break;
                    }
            }

            inline std::vector<IpcBridge *> &bridges()
            {
                return this->m_bridges;
            }

            inline static CFDataRef messageReceived(CFMessagePortRef local,
                                                    SInt32 msgid,
                                                    CFDataRef data,
                                                    void *info)
            {
                auto self = reinterpret_cast<IpcBridge *>(info);

                switch (msgid) {
                    case AKVCAM_ASSISTANT_MSG_DEVICE_CREATED: {
                        if (self->d->deviceAddedCallback)
                            self->d->deviceAddedCallback(self->d->dataToCppString(data));
                    }

                    case AKVCAM_ASSISTANT_MSG_DEVICE_DESTROYED: {
                        if (self->d->deviceRemovedCallback)
                            self->d->deviceRemovedCallback(self->d->dataToCppString(data));
                    }

                    case AKVCAM_ASSISTANT_MSG_FRAME_READY: {
                    }

                    default:
                        break;
                }

                return nullptr;
            }

            inline static void serverMessagePortInvalidated(CFMessagePortRef messagePort,
                                                            void *info)
            {
                for (auto bridge: ipcBridgePrivate()->bridges())
                    bridge->d->serverMessagePortInvalidated(messagePort);
            }

            inline void serverMessagePortInvalidated(CFMessagePortRef messagePort)
            {

            }

            inline CFDataRef stringToData(const CFStringRef cfstr)
            {
                auto data = CFStringGetCStringPtr(cfstr,
                                                  kCFStringEncodingUTF8);

                return CFDataCreate(kCFAllocatorDefault,
                                    reinterpret_cast<const UInt8 *>(data),
                                    CFStringGetLength(cfstr));
            }

            inline std::string stringToCppString(const CFStringRef cfstr)
            {
                return std::string(CFStringGetCStringPtr(cfstr,
                                                         kCFStringEncodingUTF8),
                                   CFStringGetLength(cfstr));
            }

            inline CFDataRef stringToData(const std::string &cppstr)
            {
                return CFDataCreate(kCFAllocatorDefault,
                                    reinterpret_cast<const UInt8 *>(cppstr.c_str()),
                                    cppstr.size());
            }

            inline std::string dataToCppString(const CFDataRef data)
            {
                return std::string(reinterpret_cast<const char *>(CFDataGetBytePtr(data)),
                                   size_t(CFDataGetLength(data)));
            }

            inline std::vector<std::string> dataToStringList(const CFDataRef data)
            {
                std::vector<std::string> strList;
                auto dateStrList = CFDataGetBytePtr(data);
                auto len = CFDataGetLength(data);
                CFIndex i = 0;

                do {
                    std::string str;

                    for (; *dateStrList && i < len; dateStrList++, i++)
                        str += *dateStrList;

                    if (!str.empty()) {
                        strList.push_back(str);
                        dateStrList++;
                        i++;
                    }
                } while (*dateStrList && i < len);

                return strList;
            }

            inline std::vector<AkVCam::VideoFormat> dataToVideoFormats(const CFDataRef data)
            {
                size_t bytes = size_t(CFDataGetLength(data));
                size_t nformats = bytes / sizeof(VideoFormatStruct);
                std::vector<VideoFormatStruct> formatStructs(nformats);
                memcpy(formatStructs.data(), CFDataGetBytePtr(data), bytes);

                return {formatStructs.begin(), formatStructs.end()};
            }

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
    this->d = new IpcBridgePrivate(this);
    ipcBridgePrivate()->add(this);
}

AkVCam::IpcBridge::~IpcBridge()
{
    this->unregisterEndPoint();
    ipcBridgePrivate()->remove(this);
    delete this->d;
}

bool AkVCam::IpcBridge::registerEndPoint(bool asClient)
{
    if (this->d->serverMessagePort)
        return true;

    CFDataRef dataServerName = nullptr;
    CFMessagePortRef messagePort = nullptr;
    CFMessagePortRef serverMessagePort = nullptr;
    CFRunLoopSourceRef runLoopSource = nullptr;
    CFStringRef serverName = nullptr;
    CFMessagePortContext context {0, this, nullptr, nullptr, nullptr};
    CFDataRef data = nullptr;
    SInt32 status = 0;

    serverMessagePort =
            CFMessagePortCreateRemote(kCFAllocatorDefault,
                                      CFSTR(AKVCAM_ASSISTANT_NAME));

    if (!serverMessagePort)
        goto registerEndPoint_failed;

    CFMessagePortSetInvalidationCallBack(serverMessagePort,
                                         IpcBridgePrivate::serverMessagePortInvalidated);
    data = CFDataCreate(kCFAllocatorDefault,
                        reinterpret_cast<UInt8 *>(&asClient),
                        1);
    status =
        CFMessagePortSendRequest(serverMessagePort,
                                 AKVCAM_ASSISTANT_MSG_REQUEST_PORT,
                                 data,
                                 AKVCAM_ASSISTANT_REQUEST_TIMEOUT,
                                 AKVCAM_ASSISTANT_REQUEST_TIMEOUT,
                                 kCFRunLoopDefaultMode,
                                 &dataServerName);
    CFRelease(data);

    if (status != kCFMessagePortSuccess)
        goto registerEndPoint_failed;

    serverName =
            CFStringCreateWithBytes(kCFAllocatorDefault,
                                    CFDataGetBytePtr(dataServerName),
                                    CFDataGetLength(dataServerName),
                                    kCFStringEncodingUTF8,
                                    false);

    messagePort =
            CFMessagePortCreateLocal(kCFAllocatorDefault,
                                     serverName,
                                     IpcBridgePrivate::messageReceived,
                                     &context,
                                     nullptr);
    CFRelease(serverName);

    if (!messagePort)
        goto registerEndPoint_failed;

    runLoopSource =
            CFMessagePortCreateRunLoopSource(kCFAllocatorDefault,
                                             messagePort,
                                             0);

    if (!runLoopSource)
        goto registerEndPoint_failed;

    CFRunLoopAddSource(CFRunLoopGetMain(),
                       runLoopSource,
                       kCFRunLoopCommonModes);

    status =
        CFMessagePortSendRequest(serverMessagePort,
                                 AKVCAM_ASSISTANT_MSG_ADD_PORT,
                                 dataServerName,
                                 AKVCAM_ASSISTANT_REQUEST_TIMEOUT,
                                 AKVCAM_ASSISTANT_REQUEST_TIMEOUT,
                                 nullptr,
                                 nullptr);

    if (status != kCFMessagePortSuccess)
        goto registerEndPoint_failed;

    if (dataServerName)
        CFRelease(dataServerName);

    this->d->messagePort = messagePort;
    this->d->serverMessagePort = serverMessagePort;
    this->d->runLoopSource = runLoopSource;

    return true;

registerEndPoint_failed:
    if (dataServerName)
        CFRelease(dataServerName);

    if (runLoopSource) {
        CFRunLoopRemoveSource(CFRunLoopGetMain(),
                              runLoopSource,
                              kCFRunLoopCommonModes);
        CFRelease(runLoopSource);
    }

    if (messagePort) {
        CFMessagePortInvalidate(messagePort);
        CFRelease(messagePort);
    }

    if (serverMessagePort)
        CFRelease(serverMessagePort);

    return false;
}

void AkVCam::IpcBridge::unregisterEndPoint()
{
    if (this->d->runLoopSource) {
        CFRunLoopRemoveSource(CFRunLoopGetMain(),
                              this->d->runLoopSource,
                              kCFRunLoopCommonModes);
        CFRelease(this->d->runLoopSource);
        this->d->runLoopSource = nullptr;
    }

    CFStringRef messagePortName = nullptr;

    if (this->d->messagePort) {
        messagePortName =
                CFStringCreateCopy(kCFAllocatorDefault,
                                   CFMessagePortGetName(this->d->messagePort));
        CFRelease(this->d->messagePort);
        this->d->messagePort = nullptr;
    }

    if (this->d->serverMessagePort) {
        if (messagePortName) {
            auto dataMessagePortName =
                    this->d->stringToData(messagePortName);

            CFMessagePortSendRequest(this->d->serverMessagePort,
                                     AKVCAM_ASSISTANT_MSG_REMOVE_PORT,
                                     dataMessagePortName,
                                     AKVCAM_ASSISTANT_REQUEST_TIMEOUT,
                                     AKVCAM_ASSISTANT_REQUEST_TIMEOUT,
                                     nullptr,
                                     nullptr);
            CFRelease(dataMessagePortName);

        }

        CFRelease(this->d->serverMessagePort);
        this->d->serverMessagePort = nullptr;
    }

    if (messagePortName)
        CFRelease(messagePortName);
}

std::vector<std::string> AkVCam::IpcBridge::listDevices(bool all) const
{
    if (!this->d->serverMessagePort)
        return {};

    if (!all)
        return this->d->devices;

    CFDataRef dataDevices = nullptr;

    auto status =
        CFMessagePortSendRequest(this->d->serverMessagePort,
                                 AKVCAM_ASSISTANT_MSG_DEVICES,
                                 nullptr,
                                 AKVCAM_ASSISTANT_REQUEST_TIMEOUT,
                                 AKVCAM_ASSISTANT_REQUEST_TIMEOUT,
                                 kCFRunLoopDefaultMode,
                                 &dataDevices);

    if (status != kCFMessagePortSuccess)
        return {};

    auto devices = this->d->dataToStringList(dataDevices);
    CFRelease(dataDevices);

    return devices;
}

std::string AkVCam::IpcBridge::description(const std::string &deviceId) const
{
    if (!this->d->serverMessagePort)
        return {};

    auto dataDeviceId = this->d->stringToData(deviceId);
    CFDataRef dataDescription = nullptr;

    auto status =
        CFMessagePortSendRequest(this->d->serverMessagePort,
                                 AKVCAM_ASSISTANT_MSG_DESCRIPTION,
                                 dataDeviceId,
                                 AKVCAM_ASSISTANT_REQUEST_TIMEOUT,
                                 AKVCAM_ASSISTANT_REQUEST_TIMEOUT,
                                 kCFRunLoopDefaultMode,
                                 &dataDescription);
    CFRelease(dataDeviceId);

    if (status != kCFMessagePortSuccess)
        return {};

    auto description = this->d->dataToCppString(dataDescription);
    CFRelease(dataDescription);

    return description;
}

std::vector<AkVCam::VideoFormat> AkVCam::IpcBridge::formats(const std::string &deviceId) const
{
    if (!this->d->serverMessagePort)
        return {};

    auto dataDeviceId = this->d->stringToData(deviceId);
    CFDataRef dataFormats = nullptr;

    auto status =
        CFMessagePortSendRequest(this->d->serverMessagePort,
                                 AKVCAM_ASSISTANT_MSG_FORMATS,
                                 dataDeviceId,
                                 AKVCAM_ASSISTANT_REQUEST_TIMEOUT,
                                 AKVCAM_ASSISTANT_REQUEST_TIMEOUT,
                                 kCFRunLoopDefaultMode,
                                 &dataFormats);
    CFRelease(dataDeviceId);

    if (status != kCFMessagePortSuccess)
        return {};

    auto formats = this->d->dataToVideoFormats(dataFormats);
    CFRelease(dataFormats);

    return formats;
}

std::string AkVCam::IpcBridge::deviceCreate(const std::string &description,
                                            const std::vector<VideoFormat> &formats)
{
    if (!this->d->startAssistant())
        return {};

    this->registerEndPoint(false);

    if (!this->d->serverMessagePort || !this->d->messagePort)
        return {};

    auto messagePortName = CFMessagePortGetName(this->d->messagePort);
    auto portName = this->d->stringToCppString(messagePortName);
    std::vector<VideoFormatStruct> formatsStruct;

    for (auto &format: formats)
        formatsStruct.push_back(format.toStruct());

    CFIndex dataBytes = portName.size() + 1
                      + description.size() + 1
                      + formats.size() * sizeof(VideoFormatStruct);
    std::vector<UInt8> data(dataBytes, 0);

    memcpy(data.data(), portName.data(), portName.size());
    auto it = data.data() + portName.size() + 1;
    memcpy(it, description.data(), description.size());
    it += description.size() + 1;
    memcpy(it, formatsStruct.data(), formatsStruct.size() * sizeof(VideoFormatStruct));

    auto dataParams = CFDataCreate(kCFAllocatorDefault, data.data(), data.size());
    CFDataRef dataDeviceId = nullptr;

    auto status =
        CFMessagePortSendRequest(this->d->serverMessagePort,
                                 AKVCAM_ASSISTANT_MSG_DEVICE_CREATE,
                                 dataParams,
                                 AKVCAM_ASSISTANT_REQUEST_TIMEOUT,
                                 AKVCAM_ASSISTANT_REQUEST_TIMEOUT,
                                 kCFRunLoopDefaultMode,
                                 &dataDeviceId);
    CFRelease(dataParams);

    if (status != kCFMessagePortSuccess)
        return {};

    auto deviceId = this->d->dataToCppString(dataDeviceId);
    CFRelease(dataDeviceId);
    this->d->devices.push_back(deviceId);

    return deviceId;
}

void AkVCam::IpcBridge::deviceDestroy(const std::string &deviceId)
{
    if (!this->d->serverMessagePort)
        return;

    auto it = std::find(this->d->devices.begin(),
                        this->d->devices.end(),
                        deviceId);

    if (it == this->d->devices.end())
        return;

    auto dataDeviceId = this->d->stringToData(deviceId);

    CFMessagePortSendRequest(this->d->serverMessagePort,
                             AKVCAM_ASSISTANT_MSG_DEVICE_DESTROY,
                             dataDeviceId,
                             AKVCAM_ASSISTANT_REQUEST_TIMEOUT,
                             AKVCAM_ASSISTANT_REQUEST_TIMEOUT,
                             nullptr,
                             nullptr);
    CFRelease(dataDeviceId);

    this->d->devices.erase(it);
}

bool AkVCam::IpcBridge::deviceStart(const std::string &deviceId)
{
    return false;
}

void AkVCam::IpcBridge::deviceStop(const std::string &deviceId)
{

}

void AkVCam::IpcBridge::write(const std::string &deviceId,
                              const VideoFormat &format,
                              const void *data)
{

}

bool AkVCam::IpcBridge::deviceOpen(const std::string &deviceId)
{
    return false;
}

bool AkVCam::IpcBridge::deviceClose(const std::string &deviceId)
{
    return false;
}

void AkVCam::IpcBridge::setFrameReadCallback(FrameReadCallback callback)
{
    this->d->frameReadCallback = callback;
}

void AkVCam::IpcBridge::setDeviceAddedCallback(DeviceChangedCallback callback)
{
    this->d->deviceAddedCallback = callback;
}

void AkVCam::IpcBridge::setDeviceRemovedCallback(DeviceChangedCallback callback)
{
    this->d->deviceRemovedCallback = callback;
}
