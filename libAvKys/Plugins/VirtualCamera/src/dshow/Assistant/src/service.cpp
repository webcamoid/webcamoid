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
        std::string pipeName;
        std::vector<AssistantDevice> devices;
    };

    typedef std::map<std::string, AssistantServer> AssistantServers;
    typedef std::map<std::string, std::string> AssistantClients;

    class ServicePrivate
    {
        public:
            SERVICE_STATUS m_status;
            SERVICE_STATUS_HANDLE m_statusHandler;
            MessageServer m_messageServer;
            AssistantServers m_servers;
            AssistantClients m_clients;

            ServicePrivate();
            static void stateChanged(State state, void *userData);
            void sendStatus(DWORD currentState, DWORD exitCode, DWORD wait);
            inline static uint64_t id();
            void removePortByName(const std::string &portName);
            void requestPort(Message *message);
            void addPort(Message *message);
            void removePort(Message *message);
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

    if (portName.find(AKVCAM_ASSISTANT_CLIENT_NAME) != std::string::npos) {
        for (auto &client: this->m_clients)
            if (client.first == portName) {
                ok = false;

                break ;
            }

        if (ok) {
            AkLoggerLog("Adding Client: ", portName);
            this->m_clients[portName] = pipeName;
        }
    } else {
        for (auto &server: this->m_servers)
            if (server.first == portName) {
                ok = false;

                break ;
            }

        if (ok) {
            AkLoggerLog("Adding Server: ", portName);
            this->m_servers[portName] = {pipeName, {}};
        }
    }

    data->status = ok;
}

void AkVCam::ServicePrivate::removePort(AkVCam::Message *message)
{
    auto data = messageData<MsgRemovePort>(message);
    this->removePortByName(data->port);
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
