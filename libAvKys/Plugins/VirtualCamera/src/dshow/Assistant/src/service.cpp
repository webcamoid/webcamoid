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

#include <iostream>
#include <thread>
#include <windows.h>
#include <shellapi.h>
#include <sddl.h>

#include "service.h"
#include "VCamUtils/src/cstream/cstream.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class ServicePrivate
    {
        public:
            SERVICE_STATUS m_status;
            SERVICE_STATUS_HANDLE m_statusHandler;
            HANDLE m_pipe;
            HANDLE m_finnishEvent;
            std::thread m_thread;
            bool m_running;

            ServicePrivate();
            bool main();
            bool init();
            void uninit();
            void sendStatus(DWORD currentState, DWORD exitCode, DWORD wait);
            bool readMessage(HANDLE *events,
                             uint32_t *messageId,
                             char **data,
                             uint32_t *dataSize);
            static WINAPI DWORD controlHandler(DWORD control,
                                               DWORD eventType,
                                               LPVOID eventData,
                                               LPVOID context);
            static WINAPI BOOL controlDebugHandler(DWORD control);
    };

    struct Message
    {
        uint32_t messageId;
        uint32_t dataSize;
    };

    GLOBAL_STATIC(ServicePrivate, servicePrivate)
}

AkVCam::Service::Service()
{
}

AkVCam::Service::~Service()
{
}

bool AkVCam::Service::install()
{
    WCHAR fileName[MAX_PATH];

    if (!GetModuleFileName(nullptr, fileName, MAX_PATH))
       return false;

    auto scManager =
            OpenSCManager(nullptr,
                          nullptr,
                          SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);

    if (!scManager)
        return false;

    bool ok = false;

    auto service =
            CreateService(scManager,
                          TEXT(DSHOW_PLUGIN_ASSISTANT_NAME),
                          TEXT(DSHOW_PLUGIN_ASSISTANT_DESCRIPTION),
                          SERVICE_QUERY_STATUS,
                          SERVICE_WIN32_OWN_PROCESS,
                          SERVICE_DEMAND_START,
                          SERVICE_ERROR_NORMAL,
                          fileName,
                          nullptr,
                          nullptr,
                          nullptr,
                          nullptr,
                          nullptr);

    if (service) {
        ok = true;
        CloseServiceHandle(service);
    }

    CloseServiceHandle(scManager);

    return ok;
}

void AkVCam::Service::uninstall()
{
    auto scManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);

    if (!scManager)
        return;

    auto sevice = OpenService(scManager,
                              TEXT(DSHOW_PLUGIN_ASSISTANT_NAME),
                              DELETE | SERVICE_STOP | SERVICE_QUERY_STATUS);

    if (sevice) {
        if (ControlService(sevice,
                           SERVICE_CONTROL_STOP,
                           &servicePrivate()->m_status)) {
            do {
                Sleep(1000);
                QueryServiceStatus(sevice, &servicePrivate()->m_status);
            } while(servicePrivate()->m_status.dwCurrentState != SERVICE_STOPPED);
        }

        DeleteService(sevice);
        CloseServiceHandle(sevice);
    }

    CloseServiceHandle(scManager);
}

void AkVCam::Service::debug()
{
    SetConsoleCtrlHandler(ServicePrivate::controlDebugHandler, TRUE);

    if (servicePrivate()->init()) {
        servicePrivate()->m_thread.join();
        servicePrivate()->uninit();
    }
}

void AkVCam::Service::showHelp(int argc, char **argv)
{
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

void AkVCam::Service::main(DWORD dwArgc, LPTSTR *lpszArgv)
{
    UNUSED(dwArgc)
    UNUSED(lpszArgv)

    AkLoggerLog("Setting service control handler");

    servicePrivate()->m_statusHandler =
            RegisterServiceCtrlHandlerEx(TEXT(DSHOW_PLUGIN_ASSISTANT_NAME),
                                         ServicePrivate::controlHandler,
                                         nullptr);

    if (!servicePrivate()->m_statusHandler)
        return;

    servicePrivate()->sendStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    AkLoggerLog("Setting up service");

    if (servicePrivate()->init()) {
        servicePrivate()->sendStatus(SERVICE_RUNNING, NO_ERROR, 0);
        AkLoggerLog("Service started");
        servicePrivate()->m_thread.join();
        AkLoggerLog("Stopping service");
        servicePrivate()->sendStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
        servicePrivate()->uninit();
    }

    AkLoggerLog("Service stopped");
    servicePrivate()->sendStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

AkVCam::ServicePrivate::ServicePrivate()
{
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
    this->m_pipe = INVALID_HANDLE_VALUE;
    this->m_finnishEvent = INVALID_HANDLE_VALUE;
    this->m_running = false;
}

bool AkVCam::ServicePrivate::main()
{
    HANDLE events[] = {
        this->m_finnishEvent,
        CreateEvent(nullptr, TRUE, FALSE, nullptr)
    };

    if (!events[1])
        return false;

    while (this->m_running) {
        OVERLAPPED overlaped;
        memset(&overlaped, 0, sizeof(OVERLAPPED));
        overlaped.hEvent = events[1];
        ResetEvent(overlaped.hEvent);

        // Wait for a connection.
        ConnectNamedPipe(this->m_pipe, &overlaped);

        if (GetLastError() == ERROR_IO_PENDING
            && WaitForMultipleObjects(2, events, FALSE, INFINITE) != WAIT_OBJECT_0 + 1)
            break;

        uint32_t messageId = 0;
        char *data = nullptr;
        uint32_t dataSize = 0;
        this->readMessage(events, &messageId, &data, &dataSize);
        std::shared_ptr<char> buffer(data, [](char *data) { delete [] data; });
        CStreamRead stream(buffer.get());

        // Process message.

        DisconnectNamedPipe(this->m_pipe);
    }

    if (events[1])
       CloseHandle(events[1]);

    return true;
}

bool AkVCam::ServicePrivate::init()
{
    bool ok = false;

    // Define who can read and write from pipe.

    /* Define the SDDL for the DACL.
     *
     * https://msdn.microsoft.com/en-us/library/windows/desktop/aa379570(v=vs.85).aspx
     */
    WCHAR descriptor[] =
            L"D:"                  // Discretionary ACL
            "(D;OICI;GA;;;BG)"     // Deny access to Built-in Guests
            "(D;OICI;GA;;;AN)"     // Deny access to Anonymous Logon
            "(A;OICI;GRGWGX;;;AU)" // Allow read/write/execute to Authenticated Users
            "(A;OICI;GA;;;BA)";    // Allow full control to Administrators

    std::wstring pipeName = L"\\\\.\\pipe\\";
    pipeName += DSHOW_PLUGIN_ASSISTANT_NAME_L;

    SECURITY_ATTRIBUTES securityAttributes;
    PSECURITY_DESCRIPTOR securityDescriptor =
            malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);

    if (!securityDescriptor)
        goto init_failed;

    if (!InitializeSecurityDescriptor(securityDescriptor,
                                      SECURITY_DESCRIPTOR_REVISION))
        goto init_failed;

    if (!ConvertStringSecurityDescriptorToSecurityDescriptor(descriptor,
                                                             SDDL_REVISION_1,
                                                             &securityDescriptor,
                                                             nullptr))
        goto init_failed;

    securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    securityAttributes.lpSecurityDescriptor = securityDescriptor;
    securityAttributes.bInheritHandle = TRUE;

    // Create a read/write message type pipe.
    this->m_pipe = CreateNamedPipe(pipeName.c_str(),
                                   FILE_FLAG_OVERLAPPED
                                   | PIPE_ACCESS_DUPLEX,
                                   PIPE_TYPE_MESSAGE
                                   | PIPE_READMODE_MESSAGE
                                   | PIPE_WAIT,
                                   1,
                                   0,
                                   0,
                                   1000,
                                   &securityAttributes);

    if (this->m_pipe == INVALID_HANDLE_VALUE)
        goto init_failed;

    this->m_finnishEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    if (this->m_finnishEvent == INVALID_HANDLE_VALUE)
        goto init_failed;

    ResetEvent(this->m_finnishEvent);

    // Launch service thread.
    this->m_running = true;
    this->m_thread = std::thread(&ServicePrivate::main, this);
    ok = true;

init_failed:

    if (securityDescriptor)
        free(securityDescriptor);

    return ok;
}

void AkVCam::ServicePrivate::uninit()
{
    if (this->m_finnishEvent != INVALID_HANDLE_VALUE)
        CloseHandle(this->m_finnishEvent);

    if (this->m_pipe != INVALID_HANDLE_VALUE)
        CloseHandle(this->m_pipe);
}

void AkVCam::ServicePrivate::sendStatus(DWORD currentState,
                                        DWORD exitCode,
                                        DWORD wait)
{
    if (currentState == SERVICE_START_PENDING)
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

bool AkVCam::ServicePrivate::readMessage(HANDLE *events,
                                         uint32_t *messageId,
                                         char **data,
                                         uint32_t *dataSize)
{
    // Read message header
    Message message;
    memset(&message, 0, sizeof(Message));

    DWORD bytesRead = 0;

    OVERLAPPED overlaped;
    memset(&overlaped, 0, sizeof(OVERLAPPED));
    overlaped.hEvent = events[1];
    ResetEvent(overlaped.hEvent);

    auto ok = ReadFile(this->m_pipe,
                       &message,
                       sizeof(Message),
                       &bytesRead,
                       &overlaped);

    if (!ok
        && GetLastError() == ERROR_IO_PENDING
        && WaitForMultipleObjects(2, events, FALSE, INFINITE) != WAIT_OBJECT_0 + 1)
        return false;

    // Read message body
    char *buffer = new char[message.dataSize];

    memset(&overlaped, 0, sizeof(OVERLAPPED));
    overlaped.hEvent = events[1];
    ResetEvent(overlaped.hEvent);

    ok = ReadFile(this->m_pipe,
                  buffer,
                  message.dataSize,
                  &bytesRead,
                  &overlaped);

    if (!ok
        && GetLastError() == ERROR_IO_PENDING
        && WaitForMultipleObjects(2, events, FALSE, INFINITE) != WAIT_OBJECT_0 + 1) {
        delete [] buffer;

        return false;
    }

    if (messageId)
        *messageId = message.messageId;

    if (data)
        *data = buffer;

    if (dataSize)
        *dataSize = message.dataSize;

    return true;
}

DWORD AkVCam::ServicePrivate::controlHandler(DWORD control,
                                             DWORD  eventType,
                                             LPVOID eventData,
                                             LPVOID context)
{
    UNUSED(eventType)
    UNUSED(eventData)
    UNUSED(context)

    DWORD result = ERROR_CALL_NOT_IMPLEMENTED;

    switch (control) {
        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:
            servicePrivate()->sendStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
            servicePrivate()->m_running = false;
            SetEvent(servicePrivate()->m_finnishEvent);
            result = NO_ERROR;

            break;

        case SERVICE_CONTROL_INTERROGATE:
            result = NO_ERROR;

            break;

        default:
            break;
    }

    servicePrivate()->sendStatus(servicePrivate()->m_status.dwCurrentState,
                                 NO_ERROR,
                                 0);

    return result;
}

BOOL AkVCam::ServicePrivate::controlDebugHandler(DWORD control)
{
    if (control == CTRL_BREAK_EVENT || control == CTRL_C_EVENT) {
        servicePrivate()->m_running = false;
        SetEvent(servicePrivate()->m_finnishEvent);

        return TRUE;
    }

    return FALSE;
}
