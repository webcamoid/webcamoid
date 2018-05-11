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

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>
#include <shellapi.h>
#include <sddl.h>

#include "service.h"
#include "PlatformUtils/src/messageserver.h"
#include "VCamUtils/src/cstream/cstreamread.h"
#include "VCamUtils/src/cstream/cstreamwrite.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/image/videoframe.h"
#include "VCamUtils/src/utils.h"

#define AkServiceLogMethod() \
    AkLoggerLog("Service::", __FUNCTION__, "()")

#define AkServicePrivateLogMethod() \
    AkLoggerLog("ServicePrivate::", __FUNCTION__, "()")

namespace AkVCam
{
    struct AssistantDevice
    {
        std::string owner;
        int listeners;
        bool horizontalMirror;
        bool verticalMirror;
        Scaling scaling;
        AspectRatio aspectRatio;
    };

    typedef std::map<std::string, std::string> AssistantPeers;
    typedef std::map<std::string, AssistantDevice> DeviceConfigs;

    class ServicePrivate
    {
        public:
            SERVICE_STATUS m_status;
            SERVICE_STATUS_HANDLE m_statusHandler;
            MessageServer m_messageServer;
            AssistantPeers m_servers;
            AssistantPeers m_clients;
            DeviceConfigs m_deviceConfigs;

            ServicePrivate();
            static void stateChanged(State state, void *userData);
            void sendStatus(DWORD currentState, DWORD exitCode, DWORD wait);
            inline static uint64_t id();
            void removePortByName(const std::string &portName);
            void requestPort(Message *message);
            void addPort(Message *message);
            void removePort(Message *message);
            void setBroadCasting(Message *message);
            void setMirroring(Message *message);
            void setScaling(Message *message);
            void setAspectRatio(Message *message);
            void frameReady(Message *message);
            void listeners(Message *message);
            void broadcasting(Message *message);
            void mirroring(Message *message);
            void scaling(Message *message);
            void aspectRatio(Message *message);
            void addListener(Message *message);
            void removeListener(Message *message);
    };

    GLOBAL_STATIC(ServicePrivate, servicePrivate)
}

DWORD WINAPI controlHandler(DWORD control,
                            DWORD eventType,
                            LPVOID eventData,
                            LPVOID context);
BOOL WINAPI controlDebugHandler(DWORD control);

AkVCam::Service::Service()
{
}

AkVCam::Service::~Service()
{
}

bool AkVCam::Service::install()
{
    AkServiceLogMethod();
    WCHAR fileName[MAX_PATH];

    if (!GetModuleFileName(nullptr, fileName, MAX_PATH)) {
        AkLoggerLog("Can't read module file name");

       return false;
    }

    auto scManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);

    if (!scManager) {
        AkLoggerLog("Can't open SCManager");

        return false;
    }

    auto service =
            CreateService(scManager,
                          TEXT(DSHOW_PLUGIN_ASSISTANT_NAME),
                          TEXT(DSHOW_PLUGIN_ASSISTANT_DESCRIPTION),
                          SERVICE_ALL_ACCESS,
                          SERVICE_WIN32_OWN_PROCESS,
                          SERVICE_AUTO_START,
                          SERVICE_ERROR_NORMAL,
                          fileName,
                          nullptr,
                          nullptr,
                          nullptr,
                          nullptr,
                          nullptr);

    if (!service) {
        AkLoggerLog("Can't create service");
        CloseServiceHandle(scManager);

        return false;
    }

    SERVICE_DESCRIPTION serviceDescription;
    WCHAR description[] = TEXT(DSHOW_PLUGIN_DESCRIPTION_EXT);
    serviceDescription.lpDescription = description;
    auto result =
            ChangeServiceConfig2(service,
                                 SERVICE_CONFIG_DESCRIPTION,
                                 &serviceDescription);    
    StartService(service, 0, nullptr);
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);

    return result;
}

void AkVCam::Service::uninstall()
{
    AkServiceLogMethod();
    auto scManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);

    if (!scManager) {
        AkLoggerLog("Can't open SCManager");

        return;
    }

    auto sevice = OpenService(scManager,
                              TEXT(DSHOW_PLUGIN_ASSISTANT_NAME),
                              SERVICE_ALL_ACCESS);

    if (sevice) {
        if (ControlService(sevice,
                           SERVICE_CONTROL_STOP,
                           &servicePrivate()->m_status)) {
            AkLoggerLog("Stopping service");

            do {
                Sleep(1000);
                QueryServiceStatus(sevice, &servicePrivate()->m_status);
            } while(servicePrivate()->m_status.dwCurrentState != SERVICE_STOPPED);
        }

        if (!DeleteService(sevice))
            AkLoggerLog("Delete service failed");

        CloseServiceHandle(sevice);
    } else {
        AkLoggerLog("Can't open service");
    }

    CloseServiceHandle(scManager);
}

void AkVCam::Service::debug()
{
    AkServiceLogMethod();
    SetConsoleCtrlHandler(controlDebugHandler, TRUE);
    servicePrivate()->m_messageServer.start(true);
}

void AkVCam::Service::showHelp(int argc, char **argv)
{
    AkServiceLogMethod();
    UNUSED(argc)

    auto programName = strrchr(argv[0], '\\');

    if (!programName)
        programName = strrchr(argv[0], '/');

    if (!programName)
        programName = argv[0];
    else
        programName++;

    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Webcamoid virtual camera server." << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << std::endl;
    std::cout << "\t-i, --install\tInstall the service." << std::endl;
    std::cout << "\t-u, --uninstall\tUnistall the service." << std::endl;
    std::cout << "\t-d, --debug\tDebug the service." << std::endl;
    std::cout << "\t-h, --help\tShow this help." << std::endl;
}

AkVCam::ServicePrivate::ServicePrivate()
{
    AkServicePrivateLogMethod();

    this->m_status = {
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_STOPPED,
        0,
        NO_ERROR,
        NO_ERROR,
        0,
        0
    };
    this->m_statusHandler = nullptr;
    this->m_messageServer.setPipeName(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L);
    this->m_messageServer.setHandlers({
        {AKVCAM_ASSISTANT_MSG_REQUEST_PORT          , AKVCAM_BIND_FUNC(ServicePrivate::requestPort)    },
        {AKVCAM_ASSISTANT_MSG_ADD_PORT              , AKVCAM_BIND_FUNC(ServicePrivate::addPort)        },
        {AKVCAM_ASSISTANT_MSG_REMOVE_PORT           , AKVCAM_BIND_FUNC(ServicePrivate::removePort)     },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING, AKVCAM_BIND_FUNC(ServicePrivate::setBroadCasting)},
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETMIRRORING   , AKVCAM_BIND_FUNC(ServicePrivate::setMirroring)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETSCALING     , AKVCAM_BIND_FUNC(ServicePrivate::setScaling)     },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETASPECTRATIO , AKVCAM_BIND_FUNC(ServicePrivate::setAspectRatio) },
        {AKVCAM_ASSISTANT_MSG_FRAME_READY           , AKVCAM_BIND_FUNC(ServicePrivate::frameReady)     },
        {AKVCAM_ASSISTANT_MSG_LISTENERS             , AKVCAM_BIND_FUNC(ServicePrivate::listeners)      },
        {AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING   , AKVCAM_BIND_FUNC(ServicePrivate::broadcasting)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_MIRRORING      , AKVCAM_BIND_FUNC(ServicePrivate::mirroring)      },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SCALING        , AKVCAM_BIND_FUNC(ServicePrivate::scaling)        },
        {AKVCAM_ASSISTANT_MSG_DEVICE_ASPECTRATIO    , AKVCAM_BIND_FUNC(ServicePrivate::aspectRatio)    },
        {AKVCAM_ASSISTANT_MSG_ADD_LISTENER          , AKVCAM_BIND_FUNC(ServicePrivate::addListener)    },
        {AKVCAM_ASSISTANT_MSG_REMOVE_LISTENER       , AKVCAM_BIND_FUNC(ServicePrivate::removeListener) },
    });
}

void AkVCam::ServicePrivate::stateChanged(AkVCam::State state, void *userData)
{
    UNUSED(userData)

    switch (state) {
    case StateAboutToStart:
        AkVCam::servicePrivate()->sendStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
        break;

    case StateStarted:
        AkVCam::servicePrivate()->sendStatus(SERVICE_RUNNING, NO_ERROR, 0);
        break;

    case StateAboutToStop:
        AkVCam::servicePrivate()->sendStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
        break;

    case StateStopped:
        AkVCam::servicePrivate()->sendStatus(SERVICE_STOPPED, NO_ERROR, 0);
        break;
    }
}

void AkVCam::ServicePrivate::sendStatus(DWORD currentState,
                                        DWORD exitCode,
                                        DWORD wait)
{
    AkServicePrivateLogMethod();

    this->m_status.dwControlsAccepted =
            currentState == SERVICE_START_PENDING? 0: SERVICE_ACCEPT_STOP;
    this->m_status.dwCurrentState = currentState;
    this->m_status.dwWin32ExitCode = exitCode;
    this->m_status.dwWaitHint = wait;

    if (currentState == SERVICE_RUNNING || currentState == SERVICE_STOPPED)
        this->m_status.dwCheckPoint = 0;
    else
        this->m_status.dwCheckPoint++;

    SetServiceStatus(this->m_statusHandler, &this->m_status);
}

uint64_t AkVCam::ServicePrivate::id()
{
    static uint64_t id = 0;

    return id++;
}

void AkVCam::ServicePrivate::removePortByName(const std::string &portName)
{
    AkServicePrivateLogMethod();
    AkLoggerLog("Port: ", portName);

    for (auto &server: this->m_servers)
        if (server.first == portName) {
            this->m_servers.erase(portName);

            break;
        }

    for (auto &client: this->m_clients)
        if (client.first == portName) {
            this->m_clients.erase(portName);

            break;
        }
}

void AkVCam::ServicePrivate::requestPort(AkVCam::Message *message)
{
    AkServicePrivateLogMethod();

    auto data = messageData<MsgRequestPort>(message);
    std::string portName = data->client?
                AKVCAM_ASSISTANT_CLIENT_NAME:
                AKVCAM_ASSISTANT_SERVER_NAME;
    portName += std::to_string(this->id());

    AkLoggerLog("Returning Port: ", portName);
    memcpy(data->port,
           portName.c_str(),
           (std::min<size_t>)(portName.size(), MAX_STRING));
}

void AkVCam::ServicePrivate::addPort(AkVCam::Message *message)
{
    AkServicePrivateLogMethod();

    auto data = messageData<MsgAddPort>(message);
    std::string portName(data->port);
    std::string pipeName(data->pipeName);
    bool ok = true;

    AssistantPeers *peers;

    if (portName.find(AKVCAM_ASSISTANT_CLIENT_NAME) != std::string::npos)
        peers = &this->m_clients;
    else
        peers = &this->m_servers;

    for (auto &peer: *peers)
        if (peer.first == portName) {
            ok = false;

            break ;
        }

    if (ok) {
        AkLoggerLog("Adding Peer: ", portName);
        peers->at(portName) = pipeName;
    }

    data->status = ok;
}

void AkVCam::ServicePrivate::removePort(AkVCam::Message *message)
{
    AkServicePrivateLogMethod();

    auto data = messageData<MsgRemovePort>(message);
    this->removePortByName(data->port);
}

void AkVCam::ServicePrivate::setBroadCasting(AkVCam::Message *message)
{
    AkServicePrivateLogMethod();
    auto data = messageData<MsgBroadcasting>(message);
    std::string deviceId(data->device);
    data->status = false;

    for (auto &device: this->m_deviceConfigs)
        if (device.first == deviceId) {
            std::string owner(data->owner);

            if (device.second.owner == owner)
                break;

            AkLoggerLog("Device: ", deviceId);
            AkLoggerLog("Owner: ", owner);
            device.second.owner = owner;
            data->status = true;

            Message msg(message);
            msg.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING_CHANGED;

            for (auto &client: this->m_clients)
                MessageServer::sendMessage(client.second, &msg);

            break;
        }
}

void AkVCam::ServicePrivate::setMirroring(AkVCam::Message *message)
{
    AkServicePrivateLogMethod();
    auto data = messageData<MsgMirroring>(message);
    std::string deviceId(data->device);
    data->status = false;

    for (auto &device: this->m_deviceConfigs)
        if (device.first == deviceId) {
            if (device.second.horizontalMirror == data->hmirror
                && device.second.verticalMirror == data->vmirror)
                break;

            device.second.horizontalMirror = data->hmirror;
            device.second.verticalMirror = data->vmirror;
            data->status = true;

            Message msg(message);
            msg.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_MIRRORING_CHANGED;

            for (auto &client: this->m_clients)
                MessageServer::sendMessage(client.second, &msg);

            break;
        }
}

void AkVCam::ServicePrivate::setScaling(AkVCam::Message *message)
{
    AkServicePrivateLogMethod();
    auto data = messageData<MsgScaling>(message);
    std::string deviceId(data->device);
    data->status = false;

    for (auto &device: this->m_deviceConfigs)
        if (device.first == deviceId) {
            if (device.second.scaling == data->scaling)
                break;

            device.second.scaling = data->scaling;
            data->status = true;

            Message msg(message);
            msg.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_SCALING_CHANGED;

            for (auto &client: this->m_clients)
                MessageServer::sendMessage(client.second, &msg);

            break;
        }
}

void AkVCam::ServicePrivate::setAspectRatio(AkVCam::Message *message)
{
    AkServicePrivateLogMethod();
    auto data = messageData<MsgAspectRatio>(message);
    std::string deviceId(data->device);
    data->status = false;

    for (auto &device: this->m_deviceConfigs)
        if (device.first == deviceId) {
            if (device.second.aspectRatio == data->aspect)
                break;

            device.second.aspectRatio = data->aspect;
            data->status = true;

            Message msg(message);
            msg.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_ASPECTRATIO_CHANGED;

            for (auto &client: this->m_clients)
                MessageServer::sendMessage(client.second, &msg);

            break;
        }
}

void AkVCam::ServicePrivate::frameReady(AkVCam::Message *message)
{
    AkServicePrivateLogMethod();

    for (auto &client: this->m_clients)
        MessageServer::sendMessage(client.second, message);
}

void AkVCam::ServicePrivate::listeners(AkVCam::Message *message)
{
    AkServicePrivateLogMethod();
    auto data = messageData<MsgListeners>(message);
    std::string deviceId(data->device);
    data->status = false;

    for (auto &device: this->m_deviceConfigs)
        if (device.first == deviceId) {
            data->listeners = device.second.listeners;
            data->status = true;

            break;
        }
}

void AkVCam::ServicePrivate::broadcasting(AkVCam::Message *message)
{
    AkServicePrivateLogMethod();
    auto data = messageData<MsgBroadcasting>(message);
    std::string deviceId(data->device);
    data->status = false;

    for (auto &device: this->m_deviceConfigs)
        if (device.first == deviceId) {
            memcpy(data->owner,
                   device.second.owner.c_str(),
                   std::min<size_t>(device.second.owner.size(), MAX_STRING));
            data->status = true;

            break;
        }
}

void AkVCam::ServicePrivate::mirroring(AkVCam::Message *message)
{
    AkServicePrivateLogMethod();
    auto data = messageData<MsgMirroring>(message);
    std::string deviceId(data->device);
    data->status = false;

    for (auto &device: this->m_deviceConfigs)
        if (device.first == deviceId) {
            data->hmirror = device.second.horizontalMirror;
            data->vmirror = device.second.verticalMirror;
            data->status = true;

            break;
        }
}

void AkVCam::ServicePrivate::scaling(AkVCam::Message *message)
{
    AkServicePrivateLogMethod();
    auto data = messageData<MsgScaling>(message);
    std::string deviceId(data->device);
    data->status = false;

    for (auto &device: this->m_deviceConfigs)
        if (device.first == deviceId) {
            data->scaling = device.second.scaling;
            data->status = true;

            break;
        }
}

void AkVCam::ServicePrivate::aspectRatio(AkVCam::Message *message)
{
    AkServicePrivateLogMethod();
    auto data = messageData<MsgAspectRatio>(message);
    std::string deviceId(data->device);
    data->status = false;

    for (auto &device: this->m_deviceConfigs)
        if (device.first == deviceId) {
            data->aspect = device.second.aspectRatio;
            data->status = true;

            break;
        }
}

void AkVCam::ServicePrivate::addListener(AkVCam::Message *message)
{
    AkServicePrivateLogMethod();
    auto data = messageData<MsgListeners>(message);
    std::string deviceId(data->device);
    data->status = false;

    for (auto &device: this->m_deviceConfigs)
        if (device.first == deviceId) {
            device.second.listeners++;
            data->status = true;

            Message msg(message);
            msg.messageId = AKVCAM_ASSISTANT_MSG_LISTENERS_CHANGED;

            for (auto &client: this->m_clients)
                MessageServer::sendMessage(client.second, &msg);

            break;
        }
}

void AkVCam::ServicePrivate::removeListener(AkVCam::Message *message)
{
    AkServicePrivateLogMethod();
    auto data = messageData<MsgListeners>(message);
    std::string deviceId(data->device);
    data->status = false;

    for (auto &device: this->m_deviceConfigs)
        if (device.first == deviceId) {
            device.second.listeners--;
            data->status = true;

            Message msg(message);
            msg.messageId = AKVCAM_ASSISTANT_MSG_LISTENERS_CHANGED;

            for (auto &client: this->m_clients)
                MessageServer::sendMessage(client.second, &msg);

            break;
        }
}

DWORD WINAPI controlHandler(DWORD control,
                            DWORD  eventType,
                            LPVOID eventData,
                            LPVOID context)
{
    UNUSED(eventType)
    UNUSED(eventData)
    UNUSED(context)
    AkLoggerLog("controlHandler()");

    DWORD result = ERROR_CALL_NOT_IMPLEMENTED;

    switch (control) {
        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:
            AkVCam::servicePrivate()->sendStatus(SERVICE_STOP_PENDING,
                                                 NO_ERROR,
                                                 0);
            AkVCam::servicePrivate()->m_messageServer.stop();
            result = NO_ERROR;

            break;

        case SERVICE_CONTROL_INTERROGATE:
            result = NO_ERROR;

            break;

        default:
            break;
    }

    auto state = AkVCam::servicePrivate()->m_status.dwCurrentState;
    AkVCam::servicePrivate()->sendStatus(state, NO_ERROR, 0);

    return result;
}

BOOL WINAPI controlDebugHandler(DWORD control)
{
    AkLoggerLog("controlDebugHandler()");

    if (control == CTRL_BREAK_EVENT || control == CTRL_C_EVENT) {
        AkVCam::servicePrivate()->m_messageServer.stop();

        return TRUE;
    }

    return FALSE;
}

void WINAPI serviceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
    UNUSED(dwArgc)
    UNUSED(lpszArgv)
    AkLoggerLog("serviceMain()");
    AkLoggerLog("Setting service control handler");

    AkVCam::servicePrivate()->m_statusHandler =
            RegisterServiceCtrlHandlerEx(TEXT(DSHOW_PLUGIN_ASSISTANT_NAME),
                                         controlHandler,
                                         nullptr);

    if (!AkVCam::servicePrivate()->m_statusHandler)
        return;

    AkVCam::servicePrivate()->sendStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    AkLoggerLog("Setting up service");
    AkVCam::servicePrivate()->m_messageServer
            .setStateChangedCallBack(&AkVCam::ServicePrivate::stateChanged,
                                     AkVCam::servicePrivate());
    AkVCam::servicePrivate()->m_messageServer.start(true);
}
