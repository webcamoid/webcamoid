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

#include "assistant.h"

#define TIMEOUT 10.0

namespace AkVCam
{
    class AssistantPrivate;
    AssistantPrivate *assistantPrivate();

    class AssistantPrivate
    {
        public:
            inline void add(Assistant *assistant)
            {
                this->m_assistants.push_back(assistant);
            }

            inline void remove(Assistant *assistant)
            {
                for (size_t i = 0; i < this->m_assistants.size(); i++)
                    if (this->m_assistants[i] == assistant) {
                        this->m_assistants.erase(this->m_assistants.begin()
                                                 + long(i));

                        break;
                    }
            }

            inline std::vector<Assistant *> &assistants()
            {
                return this->m_assistants;
            }

            inline static void messagePortInvalidated(CFMessagePortRef messagePort,
                                                      void *info)
            {
                for (auto assistant: assistantPrivate()->assistants())
                    assistant->messagePortInvalidated(messagePort);
            }

        private:
            std::vector<Assistant *> m_assistants;
    };

    inline AssistantPrivate *assistantPrivate()
    {
        static AssistantPrivate assistantPrivate;

        return &assistantPrivate;
    }
}

AkVCam::Assistant::Assistant()
{
    assistantPrivate()->add(this);
}

AkVCam::Assistant::~Assistant()
{
    assistantPrivate()->remove(this);
    std::vector<std::string> ports;

    for (auto &server: this->m_servers)
        ports.push_back(server.first);

    for (auto &client: this->m_clients)
        ports.push_back(client.first);

    for (auto &port: ports)
        this->removePort(port);
}

CFDataRef AkVCam::Assistant::requestPort(bool asClient) const
{
    std::string portName = asClient?
                AKVCAM_ASSISTANT_CLIENT_NAME:
                AKVCAM_ASSISTANT_SERVER_NAME;
    portName += std::to_string(this->id());

    return CFDataCreate(kCFAllocatorDefault,
                        reinterpret_cast<const UInt8 *>(portName.c_str()),
                        CFIndex(portName.data()));
}

CFDataRef AkVCam::Assistant::addPort(const std::string &portName)
{
    auto cfPortName = CFStringCreateWithCString(kCFAllocatorDefault,
                                                portName.c_str(),
                                                kCFStringEncodingUTF8);
    auto messagePort =
            CFMessagePortCreateRemote(kCFAllocatorDefault,
                                      cfPortName);
    CFRelease(cfPortName);
    CFMessagePortSetInvalidationCallBack(messagePort,
                                         AssistantPrivate::messagePortInvalidated);

    if (portName.find(AKVCAM_ASSISTANT_CLIENT_NAME) != std::string::npos) {
        for (auto &client: this->m_clients)
            if (client.first == portName)
                return nullptr;

        this->m_clients[portName] = messagePort;
    } else {
        for (auto &server: this->m_servers)
            if (server.first == portName)
                return nullptr;

        this->m_servers[portName] = {messagePort, {}};
    }

    return nullptr;
}

CFDataRef AkVCam::Assistant::removePort(const std::string &port)
{
    for (auto &server: this->m_servers)
        if (server.first == port) {
            std::vector<std::string> devices;

            for (auto &device: server.second.devices)
                devices.push_back(device.deviceId);

            for (auto &device: devices)
                this->deviceDestroy(device);

            CFRelease(server.second.messagePort);
            this->m_servers.erase(port);

            break;
        }

    for (auto &client: this->m_clients)
        if (client.first == port) {
            CFRelease(client.second);
            this->m_clients.erase(port);

            break;
        }

    return nullptr;
}

CFDataRef AkVCam::Assistant::deviceCreate(const std::string &port,
                                          const std::string &description,
                                          const std::vector<VideoFormat> &formats)
{
    for (auto &server: this->m_servers)
        if (server.first == port) {
            std::stringstream ss;
            ss << port << "/Device" << this->id();
            auto deviceId = ss.str();
            server.second.devices.push_back({deviceId, description, formats});
            auto data =
                    CFDataCreate(kCFAllocatorDefault,
                                 reinterpret_cast<const UInt8 *>(deviceId.c_str()),
                                 CFIndex(deviceId.size()));

            for (auto &client: this->m_clients)
                CFMessagePortSendRequest(client.second,
                                         AKVCAM_ASSISTANT_MSG_DEVICE_CREATED,
                                         data,
                                         TIMEOUT,
                                         TIMEOUT,
                                         nullptr,
                                         nullptr);

            return data;
        }

    return nullptr;
}

CFDataRef AkVCam::Assistant::deviceDestroy(const std::string &deviceId)
{
    for (auto &server: this->m_servers)
        for (size_t i = 0; i < server.second.devices.size(); i++)
            if (server.second.devices[i].deviceId == deviceId) {
                auto data = CFDataCreate(kCFAllocatorDefault,
                                         reinterpret_cast<const UInt8 *>(deviceId.c_str()),
                                         CFIndex(deviceId.size()));

                for (auto &client: this->m_clients)
                    CFMessagePortSendRequest(client.second,
                                             AKVCAM_ASSISTANT_MSG_DEVICE_DESTROYED,
                                             data,
                                             TIMEOUT,
                                             TIMEOUT,
                                             nullptr,
                                             nullptr);

                CFRelease(data);
                server.second.devices.erase(server.second.devices.begin() +
                                            long(i));

                return nullptr;
            }

    return nullptr;
}

CFDataRef AkVCam::Assistant::frameReady(CFDataRef data) const
{
    for (auto &client: this->m_clients)
        CFMessagePortSendRequest(client.second,
                                 AKVCAM_ASSISTANT_MSG_FRAME_READY,
                                 data,
                                 TIMEOUT,
                                 TIMEOUT,
                                 nullptr,
                                 nullptr);

    return nullptr;
}

CFDataRef AkVCam::Assistant::devices() const
{
    if (this->m_servers.size() < 1)
        return nullptr;

    size_t dataSize = 0;

    for (auto &server: this->m_servers)
        for (auto &device: server.second.devices)
            dataSize += device.deviceId.length() + 1;

    dataSize++;
    std::vector<UInt8> vecData(dataSize, 0);
    auto vecDataIt = vecData.begin();

    for (auto &server: this->m_servers)
        for (auto &device: server.second.devices) {
            std::copy(device.deviceId.begin(),
                      device.deviceId.end(),
                      vecDataIt);
            vecDataIt += long(device.deviceId.length());
            *vecDataIt = 0;
            vecDataIt++;
        }

    *vecDataIt = 0;

    return CFDataCreate(kCFAllocatorDefault, vecData.data(), CFIndex(dataSize));
}

CFDataRef AkVCam::Assistant::description(const std::string &deviceId) const
{
    for (auto &server: this->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                return CFDataCreate(kCFAllocatorDefault,
                                    reinterpret_cast<const UInt8 *>(device.description.c_str()),
                                    CFIndex(device.description.length()));
            }

    return nullptr;
}

CFDataRef AkVCam::Assistant::formats(const std::string &deviceId) const
{
    for (auto &server: this->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                std::vector<VideoFormatStruct> formats;

                for (auto &format: device.formats)
                    formats.push_back(format.toStruct());

                return CFDataCreate(kCFAllocatorDefault,
                                    reinterpret_cast<const UInt8 *>(formats.data()),
                                    CFIndex(formats.size() * sizeof(VideoFormatStruct)));
            }

    return nullptr;
}

CFDataRef AkVCam::Assistant::messageReceived(CFMessagePortRef local,
                                             SInt32 msgid,
                                             CFDataRef data,
                                             void *info)
{
    auto self = reinterpret_cast<Assistant *>(info);

    switch (msgid) {
        case AKVCAM_ASSISTANT_MSG_REQUEST_PORT: {
            auto isClient = CFDataGetBytePtr(data);

            return self->requestPort(*isClient);
        }

        case AKVCAM_ASSISTANT_MSG_ADD_PORT: {
            auto cdata = CFDataGetBytePtr(data);
            std::string portName;

            for (; *cdata != 0; cdata++)
                portName += char(*cdata);

            cdata++;

            return self->addPort(portName);
        }

        case AKVCAM_ASSISTANT_MSG_REMOVE_PORT: {
            std::string port(reinterpret_cast<const char *>(CFDataGetBytePtr(data)),
                             size_t(CFDataGetLength(data)));
            return self->removePort(port);
        }

        case AKVCAM_ASSISTANT_MSG_DEVICE_CREATE: {
            auto cdata = CFDataGetBytePtr(data);
            std::string port;

            for (; *cdata != 0; cdata++)
                port += char(*cdata);

            std::string description;

            for (cdata++; *cdata != 0; cdata++)
                description += char(*cdata);

            cdata++;
            size_t bytes = size_t(CFDataGetLength(data))
                         - port.size()
                         - description.size()
                         - 2;
            size_t nformats = bytes / sizeof(VideoFormatStruct);
            std::vector<VideoFormatStruct> formatStructs(nformats);
            memcpy(formatStructs.data(), cdata, bytes);

            return self->deviceCreate(port,
                                      description,
                                      {formatStructs.begin(),
                                       formatStructs.end()});
        }

        case AKVCAM_ASSISTANT_MSG_DEVICE_DESTROY: {
            std::string deviceId(reinterpret_cast<const char *>(CFDataGetBytePtr(data)),
                                 size_t(CFDataGetLength(data)));

            return self->deviceDestroy(deviceId);
        }

        case AKVCAM_ASSISTANT_MSG_FRAME_READY:
            return self->frameReady(data);

        case AKVCAM_ASSISTANT_MSG_DEVICES:
            return self->devices();

        case AKVCAM_ASSISTANT_MSG_DESCRIPTION: {
            std::string deviceId(reinterpret_cast<const char *>(CFDataGetBytePtr(data)),
                                 size_t(CFDataGetLength(data)));

            return self->description(deviceId);
        }

        case AKVCAM_ASSISTANT_MSG_FORMATS: {
            std::string deviceId(reinterpret_cast<const char *>(CFDataGetBytePtr(data)),
                                 size_t(CFDataGetLength(data)));

            return self->formats(deviceId);
        }
    }

    return nullptr;
}

void AkVCam::Assistant::messagePortInvalidated(CFMessagePortRef messagePort)
{
}
