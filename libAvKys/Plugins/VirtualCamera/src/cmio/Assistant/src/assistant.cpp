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
#include "VCamUtils/src/utils.h"

#define AkAssistantLogMethod() \
    AkLoggerLog("Assistant::" << __FUNCTION__ << "()")

AkVCam::Assistant::Assistant()
{
    this->m_messageHandlers = {
        {AKVCAM_ASSISTANT_MSG_REQUEST_PORT          , AKVCAM_BIND_FUNC(Assistant::requestPort)    },
        {AKVCAM_ASSISTANT_MSG_ADD_PORT              , AKVCAM_BIND_FUNC(Assistant::addPort)        },
        {AKVCAM_ASSISTANT_MSG_REMOVE_PORT           , AKVCAM_BIND_FUNC(Assistant::removePort)     },
        {AKVCAM_ASSISTANT_MSG_DEVICE_CREATE         , AKVCAM_BIND_FUNC(Assistant::deviceCreate)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_DESTROY        , AKVCAM_BIND_FUNC(Assistant::deviceDestroy)  },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING, AKVCAM_BIND_FUNC(Assistant::setBroadcasting)},
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETMIRRORING   , AKVCAM_BIND_FUNC(Assistant::setMirroring)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETSCALING     , AKVCAM_BIND_FUNC(Assistant::setScaling)     },
        {AKVCAM_ASSISTANT_MSG_FRAME_READY           , AKVCAM_BIND_FUNC(Assistant::frameReady)     },
        {AKVCAM_ASSISTANT_MSG_DEVICES               , AKVCAM_BIND_FUNC(Assistant::devices)        },
        {AKVCAM_ASSISTANT_MSG_DESCRIPTION           , AKVCAM_BIND_FUNC(Assistant::description)    },
        {AKVCAM_ASSISTANT_MSG_FORMATS               , AKVCAM_BIND_FUNC(Assistant::formats)        },
        {AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING   , AKVCAM_BIND_FUNC(Assistant::broadcasting)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_MIRRORING      , AKVCAM_BIND_FUNC(Assistant::mirroring)      },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SCALING        , AKVCAM_BIND_FUNC(Assistant::scaling)        }
    };
}

AkVCam::Assistant::~Assistant()
{
    std::vector<std::string> ports;

    for (auto &server: this->m_servers)
        ports.push_back(server.first);

    for (auto &client: this->m_clients)
        ports.push_back(client.first);

    for (auto &port: ports)
        this->removePortByName(port);
}

void AkVCam::Assistant::requestPort(xpc_connection_t client,
                                    xpc_object_t event)
{
    AkAssistantLogMethod();

    bool asClient = xpc_dictionary_get_bool(event, "client");
    std::string portName = asClient?
                AKVCAM_ASSISTANT_CLIENT_NAME:
                AKVCAM_ASSISTANT_SERVER_NAME;
    portName += std::to_string(this->id());

    AkLoggerLog("Returning Port: " << portName);

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
        for (auto &client: this->m_clients)
            if (client.first == portName) {
                ok = false;

                break ;
            }

        if (ok) {
            AkLoggerLog("Adding Client: " << portName);
            this->m_clients[portName] = connection;
        }
    } else {
        for (auto &server: this->m_servers)
            if (server.first == portName) {
                ok = false;

                break ;
            }

        if (ok) {
            AkLoggerLog("Adding Server: " << portName);
            this->m_servers[portName] = {connection, {}};
        }
    }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::Assistant::removePortByName(const std::string &portName)
{
    for (auto &server: this->m_servers)
        if (server.first == portName) {
            std::vector<std::string> devices;

            for (auto &device: server.second.devices)
                devices.push_back(device.deviceId);

            for (auto &device: devices)
                this->deviceDestroyById(device);

            xpc_release(server.second.connection);
            this->m_servers.erase(portName);

            break;
        }

    for (auto &client: this->m_clients)
        if (client.first == portName) {
            xpc_release(client.second);
            this->m_clients.erase(portName);

            break;
        }
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

    for (auto &server: this->m_servers)
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
            ss << portName << "/Device" << this->id();
            auto deviceId = ss.str();
            server.second.devices.push_back({deviceId,
                                             description,
                                             formats,
                                             false,
                                             false,
                                             false,
                                             VideoFrame::ScalingFast});

            auto notification = xpc_dictionary_create(NULL, NULL, 0);
            xpc_dictionary_set_int64(notification, "message", AKVCAM_ASSISTANT_MSG_DEVICE_CREATED);
            xpc_dictionary_set_string(notification, "device", deviceId.c_str());

            for (auto &client: this->m_clients)
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
    for (auto &server: this->m_servers)
        for (size_t i = 0; i < server.second.devices.size(); i++)
            if (server.second.devices[i].deviceId == deviceId) {
                auto notification = xpc_dictionary_create(NULL, NULL, 0);
                xpc_dictionary_set_int64(notification, "message", AKVCAM_ASSISTANT_MSG_DEVICE_DESTROYED);
                xpc_dictionary_set_string(notification, "device", deviceId.c_str());

                for (auto &client: this->m_clients)
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

    for (auto &server: this->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                bool broadcasting = xpc_dictionary_get_bool(event, "broadcasting");

                if (device.broadcasting == broadcasting)
                    goto setBroadcasting_end;

                device.broadcasting = broadcasting;
                auto notification = xpc_dictionary_create(NULL, NULL, 0);
                xpc_dictionary_set_int64(notification, "message", AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING_CHANGED);
                xpc_dictionary_set_string(notification, "device", deviceId.c_str());
                xpc_dictionary_set_bool(notification, "broadcasting", broadcasting);

                for (auto &client: this->m_clients)
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

    for (auto &server: this->m_servers)
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

                for (auto &client: this->m_clients)
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

void AkVCam::Assistant::setScaling(xpc_connection_t client, xpc_object_t event)
{
    AkAssistantLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    bool ok = false;

    for (auto &server: this->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                auto scaling = VideoFrame::Scaling(xpc_dictionary_get_int64(event, "scaling"));

                if (device.scaling == scaling)
                    goto setScaling_end;

                device.scaling = scaling;
                auto notification = xpc_dictionary_create(NULL, NULL, 0);
                xpc_dictionary_set_int64(notification, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SCALING_CHANGED);
                xpc_dictionary_set_string(notification, "device", deviceId.c_str());
                xpc_dictionary_set_int64(notification, "scaling", scaling);

                for (auto &client: this->m_clients)
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

void AkVCam::Assistant::frameReady(xpc_connection_t client,
                                   xpc_object_t event)
{
    UNUSED(client)
    AkAssistantLogMethod();

    for (auto &client: this->m_clients)
        xpc_connection_send_message(client.second, event);
}

void AkVCam::Assistant::devices(xpc_connection_t client,
                                xpc_object_t event)
{
    AkAssistantLogMethod();

    auto devices = xpc_array_create(NULL, 0);

    for (auto &server: this->m_servers)
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

    for (auto &server: this->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                AkLoggerLog("Description for device "
                            << deviceId
                            << ": "
                            << device.description);

                auto reply = xpc_dictionary_create_reply(event);
                xpc_dictionary_set_string(reply, "description", device.description.c_str());
                xpc_connection_send_message(client, reply);
                xpc_release(reply);

                return;
            }
}

void AkVCam::Assistant::formats(xpc_connection_t client,
                                xpc_object_t event)
{
    AkAssistantLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");

    for (auto &server: this->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                auto formats = xpc_array_create(NULL, 0);

                for (auto &format: device.formats) {
                    auto dictFormat = xpc_dictionary_create(nullptr, nullptr, 0);
                    xpc_dictionary_set_uint64(dictFormat, "fourcc", format.fourcc());
                    xpc_dictionary_set_int64(dictFormat, "width", format.width());
                    xpc_dictionary_set_int64(dictFormat, "height", format.height());
                    xpc_dictionary_set_double(dictFormat, "fps", format.minimumFrameRate());
                    xpc_array_append_value(formats, dictFormat);
                }

                auto reply = xpc_dictionary_create_reply(event);
                xpc_dictionary_set_value(reply, "formats", formats);
                xpc_connection_send_message(client, reply);
                xpc_release(reply);

                return;
            }
}

void AkVCam::Assistant::broadcasting(xpc_connection_t client,
                                     xpc_object_t event)
{
    AkAssistantLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");

    for (auto &server: this->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                auto reply = xpc_dictionary_create_reply(event);
                xpc_dictionary_set_bool(reply, "broadcasting", device.broadcasting);
                xpc_connection_send_message(client, reply);
                xpc_release(reply);

                return;
            }
}

void AkVCam::Assistant::mirroring(xpc_connection_t client, xpc_object_t event)
{
    AkAssistantLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");

    for (auto &server: this->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                auto reply = xpc_dictionary_create_reply(event);
                xpc_dictionary_set_bool(reply, "hmirror", device.horizontalMirror);
                xpc_dictionary_set_bool(reply, "vmirror", device.verticalMirror);
                xpc_connection_send_message(client, reply);
                xpc_release(reply);

                return;
            }
}

void AkVCam::Assistant::scaling(xpc_connection_t client, xpc_object_t event)
{
    AkAssistantLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");

    for (auto &server: this->m_servers)
        for (auto &device: server.second.devices)
            if (device.deviceId == deviceId) {
                auto reply = xpc_dictionary_create_reply(event);
                xpc_dictionary_set_int64(reply, "scaling", device.scaling);
                xpc_connection_send_message(client, reply);
                xpc_release(reply);

                return;
            }
}

void AkVCam::Assistant::messageReceived(xpc_connection_t client,
                                        xpc_object_t event)
{
    AkAssistantLogMethod();
    auto type = xpc_get_type(event);

    if (type == XPC_TYPE_ERROR) {
        auto description = xpc_copy_description(event);
        AkLoggerLog("ERROR: " << description);
        free(description);
    } else if (type == XPC_TYPE_DICTIONARY) {
        int64_t message = xpc_dictionary_get_int64(event, "message");

        if (this->m_messageHandlers.count(message))
            this->m_messageHandlers[message](client, event);
    }
}
