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

#include <map>
#include <sstream>
#include <CoreFoundation/CFRunLoop.h>
#include <xpc/xpc.h>
#include <xpc/connection.h>

#include "assistant.h"
#include "assistantglobals.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/image/videoframe.h"
#include "VCamUtils/src/utils.h"

#define AkAssistantLogMethod() \
    AkLoggerLog("Assistant::", __FUNCTION__, "()")

namespace AkVCam
{
    struct AssistantDevice
    {
        std::string deviceId;
        std::string description;
        std::vector<VideoFormat> formats;
        int listeners;
        bool broadcasting;
        bool horizontalMirror;
        bool verticalMirror;
        Scaling scaling;
        AspectRatio aspectRatio;
    };

    struct AssistantServer
    {
        xpc_connection_t connection;
        std::vector<AssistantDevice> devices;
    };

    typedef std::map<std::string, AssistantServer> AssistantServers;
    typedef std::map<std::string, xpc_connection_t> AssistantClients;
    typedef std::function<void (xpc_connection_t,
                                xpc_object_t)> XpcMessage;

    class AssistantPrivate
    {
        public:
            AssistantServers m_servers;
            AssistantClients m_clients;
            std::map<int64_t, XpcMessage> m_messageHandlers;
            CFRunLoopTimerRef m_timer;
            double m_timeout;

            inline static uint64_t id();
            inline bool startTimer();
            inline void stopTimer();
            inline static void timerTimeout(CFRunLoopTimerRef timer, void *info);
    };
}

AkVCam::Assistant::Assistant()
{
    this->d = new AssistantPrivate;
    this->d->m_timer = nullptr;
    this->d->m_timeout = 0;

    this->d->m_messageHandlers = {
        {AKVCAM_ASSISTANT_MSG_REQUEST_PORT          , AKVCAM_BIND_FUNC(Assistant::requestPort)    },
        {AKVCAM_ASSISTANT_MSG_ADD_PORT              , AKVCAM_BIND_FUNC(Assistant::addPort)        },
        {AKVCAM_ASSISTANT_MSG_REMOVE_PORT           , AKVCAM_BIND_FUNC(Assistant::removePort)     },
        {AKVCAM_ASSISTANT_MSG_DEVICE_CREATE         , AKVCAM_BIND_FUNC(Assistant::deviceCreate)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_DESTROY        , AKVCAM_BIND_FUNC(Assistant::deviceDestroy)  },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING, AKVCAM_BIND_FUNC(Assistant::setBroadcasting)},
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETMIRRORING   , AKVCAM_BIND_FUNC(Assistant::setMirroring)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETSCALING     , AKVCAM_BIND_FUNC(Assistant::setScaling)     },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETASPECTRATIO , AKVCAM_BIND_FUNC(Assistant::setAspectRatio) },
        {AKVCAM_ASSISTANT_MSG_FRAME_READY           , AKVCAM_BIND_FUNC(Assistant::frameReady)     },
        {AKVCAM_ASSISTANT_MSG_LISTENERS             , AKVCAM_BIND_FUNC(Assistant::listeners)      },
        {AKVCAM_ASSISTANT_MSG_DEVICES               , AKVCAM_BIND_FUNC(Assistant::devices)        },
        {AKVCAM_ASSISTANT_MSG_DESCRIPTION           , AKVCAM_BIND_FUNC(Assistant::description)    },
        {AKVCAM_ASSISTANT_MSG_FORMATS               , AKVCAM_BIND_FUNC(Assistant::formats)        },
        {AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING   , AKVCAM_BIND_FUNC(Assistant::broadcasting)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_MIRRORING      , AKVCAM_BIND_FUNC(Assistant::mirroring)      },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SCALING        , AKVCAM_BIND_FUNC(Assistant::scaling)        },
        {AKVCAM_ASSISTANT_MSG_DEVICE_ASPECTRATIO    , AKVCAM_BIND_FUNC(Assistant::aspectRatio)    },
        {AKVCAM_ASSISTANT_MSG_ADD_LISTENER          , AKVCAM_BIND_FUNC(Assistant::addListener)    },
        {AKVCAM_ASSISTANT_MSG_REMOVE_LISTENER       , AKVCAM_BIND_FUNC(Assistant::removeListener) },
    };

    this->d->startTimer();
}

AkVCam::Assistant::~Assistant()
{
    std::vector<std::string> ports;

    for (auto &server: this->d->m_servers)
        ports.push_back(server.first);

    for (auto &client: this->d->m_clients)
        ports.push_back(client.first);

    for (auto &port: ports)
        this->removePortByName(port);

    delete this->d;
}

void AkVCam::Assistant::setTimeout(double timeout)
{
    this->d->m_timeout = timeout;
}

void AkVCam::Assistant::requestPort(xpc_connection_t client,
                                    xpc_object_t event)
{
    AkAssistantLogMethod();

    bool asClient = xpc_dictionary_get_bool(event, "client");
    std::string portName = asClient?
                AKVCAM_ASSISTANT_CLIENT_NAME:
                AKVCAM_ASSISTANT_SERVER_NAME;
    portName += std::to_string(this->d->id());

    AkLoggerLog("Returning Port: ", portName);

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_string(reply, "port", portName.c_str());
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::Assistant::addPort(xpc_connection_t client,
                                xpc_object_t event)
{
    AkAssistantLogMethod();

    std::string portName = xpc_dictionary_get_string(event, "port");
    auto endpoint = xpc_dictionary_get_value(event, "connection");
    auto connection = xpc_connection_create_from_endpoint(reinterpret_cast<xpc_endpoint_t>(endpoint));
    xpc_connection_set_event_handler(connection, ^(xpc_object_t) {});
    xpc_connection_resume(connection);
    bool ok = true;

    if (portName.find(AKVCAM_ASSISTANT_CLIENT_NAME) != std::string::npos) {
        for (auto &client: this->d->m_clients)
            if (client.first == portName) {
                ok = false;

                break ;
            }

        if (ok) {
            AkLoggerLog("Adding Client: ", portName);
            this->d->m_clients[portName] = connection;
            this->d->stopTimer();
        }
    } else {
        for (auto &server: this->d->m_servers)
            if (server.first == portName) {
                ok = false;

                break ;
            }

        if (ok) {
            AkLoggerLog("Adding Server: ", portName);
            this->d->m_servers[portName] = {connection, {}};
            this->d->stopTimer();
        }
    }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::Assistant::removePortByName(const std::string &portName)
{
    AkAssistantLogMethod();
    AkLoggerLog("Port: ", portName);

    for (auto &server: this->d->m_servers)
        if (server.first == portName) {
            std::vector<std::string> devices;

            for (auto &device: server.second.devices)
                devices.push_back(device.deviceId);

            for (auto &device: devices)
                this->deviceDestroyById(device);

            xpc_release(server.second.connection);
            this->d->m_servers.erase(portName);

            break;
        }

    for (auto &client: this->d->m_clients)
        if (client.first == portName) {
            xpc_release(client.second);
            this->d->m_clients.erase(portName);

            break;
        }

    if (this->d->m_clients.empty() && this->d->m_servers.empty())
        this->d->startTimer();
}

void AkVCam::Assistant::removePort(xpc_connection_t client,
                                   xpc_object_t event)
{
    UNUSED(client)
    AkAssistantLogMethod();

    this->removePortByName(xpc_dictionary_get_string(event, "port"));
}

void AkVCam::Assistant::deviceCreate(xpc_connection_t client,
                                     xpc_object_t event)
{
    AkAssistantLogMethod();

    std::string portName = xpc_dictionary_get_string(event, "port");

    for (auto &server: this->d->m_servers)
        if (server.first == portName) {
            std::string description = xpc_dictionary_get_string(event, "description");
            auto formatsArray = xpc_dictionary_get_array(event, "formats");

            std::vector<VideoFormat> formats;

            for (size_t i = 0; i < xpc_array_get_count(formatsArray); i++) {
                auto format = xpc_array_get_dictionary(formatsArray, i);
                auto fourcc = FourCC(xpc_dictionary_get_uint64(format, "fourcc"));
                auto width = int(xpc_dictionary_get_int64(format, "width"));
                auto height = int(xpc_dictionary_get_int64(format, "height"));
                double frameRate = xpc_dictionary_get_double(format, "fps");
                formats.push_back(VideoFormat {
                                      fourcc,
                                      width,
                                      height,
                                      {frameRate}
                                  });
            }

            std::stringstream ss;
            ss << portName << "/Device" << this->d->id();
            auto deviceId = ss.str();
            server.second.devices.push_back({deviceId,
                                             description,
                                             formats,
                                             0,
                                             false,
                                             false,
                                             false,
                                             ScalingFast,
                                             AspectRatioIgnore});

            auto notification = xpc_dictionary_create(NULL, NULL, 0);
            xpc_dictionary_set_int64(notification, "message", AKVCAM_ASSISTANT_MSG_DEVICE_CREATED);
            xpc_dictionary_set_string(notification, "device", deviceId.c_str());

            for (auto &client: this->d->m_clients)
                xpc_connection_send_message(client.second, notification);

            xpc_release(notification);

            auto reply = xpc_dictionary_create_reply(event);
            xpc_dictionary_set_string(reply, "device", deviceId.c_str());
            xpc_connection_send_message(client, reply);
            xpc_release(reply);

            break;
        }
}

void AkVCam::Assistant::deviceDestroyById(const std::string &deviceId)
{
    for (auto &server: this->d->m_servers)
        for (size_t i = 0; i < server.second.devices.size(); i++)
            if (server.second.devices[i].deviceId == deviceId) {
                auto notification = xpc_dictionary_create(NULL, NULL, 0);
                xpc_dictionary_set_int64(notification, "message", AKVCAM_ASSISTANT_MSG_DEVICE_DESTROYED);
                xpc_dictionary_set_string(notification, "device", deviceId.c_str());

                for (auto &client: this->d->m_clients)
                    xpc_connection_send_message(client.second, notification);

                xpc_release(notification);
                server.second.devices.erase(server.second.devices.begin() +
                                            long(i));

                return;
            }
}

void AkVCam::Assistant::deviceDestroy(xpc_connection_t client,
                                      xpc_object_t event)
{
    UNUSED(client)
    AkAssistantLogMethod();

    this->deviceDestroyById(xpc_dictionary_get_string(event, "device"));
}

void AkVCam::Assistant::setBroadcasting(xpc_connection_t client,
                                        xpc_object_t event)
{
    AkAssistantLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    bool ok = false;

    for (auto &server: this->d->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                bool broadcasting = xpc_dictionary_get_bool(event, "broadcasting");

                if (device.broadcasting == broadcasting)
                    goto setBroadcasting_end;

                AkLoggerLog("Device: ", deviceId);
                AkLoggerLog("Broadcasting: ", broadcasting);
                device.broadcasting = broadcasting;
                auto notification = xpc_dictionary_create(NULL, NULL, 0);
                xpc_dictionary_set_int64(notification, "message", AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING_CHANGED);
                xpc_dictionary_set_string(notification, "device", deviceId.c_str());
                xpc_dictionary_set_bool(notification, "broadcasting", broadcasting);

                for (auto &client: this->d->m_clients)
                    xpc_connection_send_message(client.second, notification);

                ok = true;

                goto setBroadcasting_end;
            }

setBroadcasting_end:

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::Assistant::setMirroring(xpc_connection_t client, xpc_object_t event)
{
    AkAssistantLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    bool ok = false;

    for (auto &server: this->d->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                bool horizontalMirror = xpc_dictionary_get_bool(event, "hmirror");
                bool verticalMirror = xpc_dictionary_get_bool(event, "vmirror");

                if (device.horizontalMirror == horizontalMirror
                    && device.verticalMirror == verticalMirror)
                    goto setMirroring_end;

                device.horizontalMirror = horizontalMirror;
                device.verticalMirror = verticalMirror;
                auto notification = xpc_dictionary_create(NULL, NULL, 0);
                xpc_dictionary_set_int64(notification, "message", AKVCAM_ASSISTANT_MSG_DEVICE_MIRRORING_CHANGED);
                xpc_dictionary_set_string(notification, "device", deviceId.c_str());
                xpc_dictionary_set_bool(notification, "hmirror", horizontalMirror);
                xpc_dictionary_set_bool(notification, "vmirror", verticalMirror);

                for (auto &client: this->d->m_clients)
                    xpc_connection_send_message(client.second, notification);

                ok = true;

                goto setMirroring_end;
            }

setMirroring_end:

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::Assistant::setScaling(xpc_connection_t client,
                                   xpc_object_t event)
{
    AkAssistantLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    bool ok = false;

    for (auto &server: this->d->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                auto scaling = Scaling(xpc_dictionary_get_int64(event, "scaling"));

                if (device.scaling == scaling)
                    goto setScaling_end;

                device.scaling = scaling;
                auto notification = xpc_dictionary_create(NULL, NULL, 0);
                xpc_dictionary_set_int64(notification, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SCALING_CHANGED);
                xpc_dictionary_set_string(notification, "device", deviceId.c_str());
                xpc_dictionary_set_int64(notification, "scaling", scaling);

                for (auto &client: this->d->m_clients)
                    xpc_connection_send_message(client.second, notification);

                ok = true;

                goto setScaling_end;
            }

setScaling_end:

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::Assistant::setAspectRatio(xpc_connection_t client, xpc_object_t event)
{
    AkAssistantLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    bool ok = false;

    for (auto &server: this->d->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                auto aspectRatio = AspectRatio(xpc_dictionary_get_int64(event, "aspect"));

                if (device.aspectRatio == aspectRatio)
                    goto setAspectRatio_end;

                device.aspectRatio = aspectRatio;
                auto notification = xpc_dictionary_create(NULL, NULL, 0);
                xpc_dictionary_set_int64(notification, "message", AKVCAM_ASSISTANT_MSG_DEVICE_ASPECTRATIO_CHANGED);
                xpc_dictionary_set_string(notification, "device", deviceId.c_str());
                xpc_dictionary_set_int64(notification, "aspect", aspectRatio);

                for (auto &client: this->d->m_clients)
                    xpc_connection_send_message(client.second, notification);

                ok = true;

                goto setAspectRatio_end;
            }

setAspectRatio_end:

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::Assistant::frameReady(xpc_connection_t client,
                                   xpc_object_t event)
{
    UNUSED(client)
    AkAssistantLogMethod();

    for (auto &client: this->d->m_clients)
        xpc_connection_send_message(client.second, event);
}

void AkVCam::Assistant::listeners(xpc_connection_t client,
                                  xpc_object_t event)
{
    AkAssistantLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    int listeners = 0;

    for (auto &server: this->d->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                listeners = device.listeners;

                goto listeners_end;
            }

listeners_end:
    AkLoggerLog("Device: ", deviceId);
    AkLoggerLog("Listeners: ", listeners);
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_int64(reply, "listeners", listeners);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::Assistant::devices(xpc_connection_t client,
                                xpc_object_t event)
{
    AkAssistantLogMethod();
    auto devices = xpc_array_create(NULL, 0);

    for (auto &server: this->d->m_servers)
        for (auto &device: server.second.devices) {
            auto deviceObj = xpc_string_create(device.deviceId.c_str());
            xpc_array_append_value(devices, deviceObj);
        }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_value(reply, "devices", devices);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::Assistant::description(xpc_connection_t client,
                                    xpc_object_t event)
{
    AkAssistantLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    std::string description;

    for (auto &server: this->d->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                description = device.description;

                goto description_end;
            }

description_end:
    AkLoggerLog("Description for device ", deviceId, ": ", description);

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_string(reply, "description", description.c_str());
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::Assistant::formats(xpc_connection_t client,
                                xpc_object_t event)
{
    AkAssistantLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    auto formats = xpc_array_create(NULL, 0);

    for (auto &server: this->d->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                for (auto &format: device.formats) {
                    auto dictFormat = xpc_dictionary_create(nullptr, nullptr, 0);
                    xpc_dictionary_set_uint64(dictFormat, "fourcc", format.fourcc());
                    xpc_dictionary_set_int64(dictFormat, "width", format.width());
                    xpc_dictionary_set_int64(dictFormat, "height", format.height());
                    xpc_dictionary_set_double(dictFormat, "fps", format.minimumFrameRate());
                    xpc_array_append_value(formats, dictFormat);
                }

                goto formats_end;
            }

formats_end:
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_value(reply, "formats", formats);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::Assistant::broadcasting(xpc_connection_t client,
                                     xpc_object_t event)
{
    AkAssistantLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    bool broadcasting = false;

    for (auto &server: this->d->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                broadcasting = device.broadcasting;

                goto broadcasting_end;
            }

broadcasting_end:
    AkLoggerLog("Device: ", deviceId);
    AkLoggerLog("Broadcasting: ", broadcasting);
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "broadcasting", broadcasting);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::Assistant::mirroring(xpc_connection_t client, xpc_object_t event)
{
    AkAssistantLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    bool horizontalMirror = false;
    bool verticalMirror = false;

    for (auto &server: this->d->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                horizontalMirror = device.horizontalMirror;
                verticalMirror = device.verticalMirror;

                goto mirroring_end;
            }

mirroring_end:
    AkLoggerLog("Device: ", deviceId);
    AkLoggerLog("Horizontal mirror: ", horizontalMirror);
    AkLoggerLog("Vertical mirror: ", verticalMirror);
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "hmirror", horizontalMirror);
    xpc_dictionary_set_bool(reply, "vmirror", verticalMirror);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::Assistant::scaling(xpc_connection_t client, xpc_object_t event)
{
    AkAssistantLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    bool scaling = false;

    for (auto &server: this->d->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                scaling = device.scaling;

                goto scaling_end;
            }

scaling_end:
    AkLoggerLog("Device: ", deviceId);
    AkLoggerLog("Scaling: ", scaling);
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_int64(reply, "scaling", scaling);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::Assistant::aspectRatio(xpc_connection_t client,
                                    xpc_object_t event)
{
    AkAssistantLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    bool aspectRatio = false;

    for (auto &server: this->d->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                aspectRatio = device.aspectRatio;

                goto aspectRatio_end;
            }

aspectRatio_end:
    AkLoggerLog("Device: ", deviceId);
    AkLoggerLog("Aspect ratio: ", aspectRatio);
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_int64(reply, "aspect", aspectRatio);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::Assistant::addListener(xpc_connection_t client,
                                    xpc_object_t event)
{
    AkAssistantLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    bool ok = false;

    for (auto &server: this->d->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                device.listeners++;
                auto notification = xpc_dictionary_create(NULL, NULL, 0);
                xpc_dictionary_set_int64(notification, "message", AKVCAM_ASSISTANT_MSG_LISTENERS_CHANGED);
                xpc_dictionary_set_string(notification, "device", deviceId.c_str());
                xpc_dictionary_set_int64(notification, "listeners", device.listeners);

                for (auto &client: this->d->m_clients)
                    xpc_connection_send_message(client.second, notification);

                ok = true;

                goto addListener_end;
            }

addListener_end:

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::Assistant::removeListener(xpc_connection_t client,
                                       xpc_object_t event)
{
    AkAssistantLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    bool ok = false;

    for (auto &server: this->d->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                device.listeners--;
                auto notification = xpc_dictionary_create(NULL, NULL, 0);
                xpc_dictionary_set_int64(notification, "message", AKVCAM_ASSISTANT_MSG_LISTENERS_CHANGED);
                xpc_dictionary_set_string(notification, "device", deviceId.c_str());
                xpc_dictionary_set_int64(notification, "listeners", device.listeners);

                for (auto &client: this->d->m_clients)
                    xpc_connection_send_message(client.second, notification);

                ok = true;

                goto removeListener_end;
            }

removeListener_end:

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::Assistant::peerDied()
{
    AkAssistantLogMethod();

    std::map<std::string, xpc_connection_t> clients;

    for (auto &server: this->d->m_servers)
        clients[server.first] = server.second.connection;

    for (auto &client: this->d->m_clients)
        clients[client.first] = client.second;

    for (auto &client: clients) {
        auto dictionary = xpc_dictionary_create(NULL, NULL, 0);
        xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_ISALIVE);
        auto reply = xpc_connection_send_message_with_reply_sync(client.second,
                                                                 dictionary);
        xpc_release(dictionary);
        auto replyType = xpc_get_type(reply);
        bool alive = false;

        if (replyType == XPC_TYPE_DICTIONARY)
            alive = xpc_dictionary_get_bool(reply, "alive");

        xpc_release(reply);

        if (!alive)
            this->removePortByName(client.first);
    }
}

void AkVCam::Assistant::messageReceived(xpc_connection_t client,
                                        xpc_object_t event)
{
    AkAssistantLogMethod();
    auto type = xpc_get_type(event);

    if (type == XPC_TYPE_ERROR) {
        if (event == XPC_ERROR_CONNECTION_INVALID) {
            this->peerDied();
        } else {
            auto description = xpc_copy_description(event);
            AkLoggerLog("ERROR: ", description);
            free(description);
        }
    } else if (type == XPC_TYPE_DICTIONARY) {
        int64_t message = xpc_dictionary_get_int64(event, "message");

        if (this->d->m_messageHandlers.count(message))
            this->d->m_messageHandlers[message](client, event);
    }
}

uint64_t AkVCam::AssistantPrivate::id()
{
    static uint64_t id = 0;

    return id++;
}

bool AkVCam::AssistantPrivate::startTimer()
{
    AkLoggerLog("AkVCam::AssistantPrivate::startTimer()");

    if (this->m_timer || this->m_timeout <= 0.0)
        return false;

    // If no peer has been connected for 5 minutes shutdown the assistant.
    CFRunLoopTimerContext context {0, this, nullptr, nullptr, nullptr};
    this->m_timer =
            CFRunLoopTimerCreate(kCFAllocatorDefault,
                                 CFAbsoluteTimeGetCurrent() + this->m_timeout,
                                 0,
                                 0,
                                 0,
                                 AssistantPrivate::timerTimeout,
                                 &context);

    if (!this->m_timer)
        return false;

    CFRunLoopAddTimer(CFRunLoopGetMain(),
                      this->m_timer,
                      kCFRunLoopCommonModes);

    return true;
}

void AkVCam::AssistantPrivate::stopTimer()
{
    AkLoggerLog("AkVCam::AssistantPrivate::stopTimer()");

    if (!this->m_timer)
        return;

    CFRunLoopTimerInvalidate(this->m_timer);
    CFRunLoopRemoveTimer(CFRunLoopGetMain(),
                         this->m_timer,
                         kCFRunLoopCommonModes);
    CFRelease(this->m_timer);
    this->m_timer = nullptr;
}

void AkVCam::AssistantPrivate::timerTimeout(CFRunLoopTimerRef timer, void *info)
{
    AkLoggerLog("AkVCam::AssistantPrivate::timerTimeout()");
    UNUSED(timer)
    UNUSED(info)

    CFRunLoopStop(CFRunLoopGetMain());
}
