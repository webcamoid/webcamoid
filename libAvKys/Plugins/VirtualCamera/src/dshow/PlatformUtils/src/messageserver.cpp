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

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <windows.h>
#include <sddl.h>

#include "messageserver.h"
#include "utils.h"
#include "VCamUtils/src/logger/logger.h"

#define AK_CUR_INTERFACE "MessageServer"

namespace AkVCam
{
    class MessageServerPrivate
    {
        public:
            MessageServer *self;
            std::wstring m_pipeName;
            std::map<uint32_t, MessageHandler> m_handlers;
            MessageServer::ServerMode m_mode;
            MessageServer::PipeState m_pipeState;
            HANDLE m_pipe;
            OVERLAPPED m_overlapped;
            std::thread m_thread;
            std::mutex m_mutex;
            std::condition_variable_any m_exitCheckLoop;
            int m_checkInterval;
            bool m_running;

            MessageServerPrivate(MessageServer *self);
            bool startReceive(bool wait=false);
            void stopReceive(bool wait=false);
            bool startSend();
            void stopSend();
            void messagesLoop();
            void checkLoop();
            HRESULT waitResult(DWORD *bytesTransferred);
            bool readMessage(Message *message);
            bool writeMessage(const Message &message);
    };
}

AkVCam::MessageServer::MessageServer()
{
    this->d = new MessageServerPrivate(this);
}

AkVCam::MessageServer::~MessageServer()
{
    this->stop(true);
    delete this->d;
}

std::wstring AkVCam::MessageServer::pipeName() const
{
    return this->d->m_pipeName;
}

std::wstring &AkVCam::MessageServer::pipeName()
{
    return this->d->m_pipeName;
}

void AkVCam::MessageServer::setPipeName(const std::wstring &pipeName)
{
    this->d->m_pipeName = pipeName;
}

AkVCam::MessageServer::ServerMode AkVCam::MessageServer::mode() const
{
    return this->d->m_mode;
}

AkVCam::MessageServer::ServerMode &AkVCam::MessageServer::mode()
{
    return this->d->m_mode;
}

void AkVCam::MessageServer::setMode(ServerMode mode)
{
    this->d->m_mode = mode;
}

int AkVCam::MessageServer::checkInterval() const
{
    return this->d->m_checkInterval;
}

int &AkVCam::MessageServer::checkInterval()
{
    return this->d->m_checkInterval;
}

void AkVCam::MessageServer::setCheckInterval(int checkInterval)
{
    this->d->m_checkInterval = checkInterval;
}

void AkVCam::MessageServer::setHandlers(const std::map<uint32_t, MessageHandler> &handlers)
{
    this->d->m_handlers = handlers;
}

bool AkVCam::MessageServer::start(bool wait)
{
    AkLogMethod();

    switch (this->d->m_mode) {
    case ServerModeReceive:
        AkLoggerLog("Starting mode receive");

        return this->d->startReceive(wait);

    case ServerModeSend:
        AkLoggerLog("Starting mode send");

        return this->d->startSend();

    default:
        break;
    }

    return false;
}

void AkVCam::MessageServer::stop(bool wait)
{
    AkLogMethod();

    if (this->d->m_mode == ServerModeReceive)
        this->d->stopReceive(wait);
    else
        this->d->stopSend();
}

bool AkVCam::MessageServer::sendMessage(Message *message,
                                        uint32_t timeout)
{
    return this->sendMessage(this->d->m_pipeName, message, timeout);
}

bool AkVCam::MessageServer::sendMessage(const Message &messageIn,
                                        Message *messageOut,
                                        uint32_t timeout)
{
    return this->sendMessage(this->d->m_pipeName,
                             messageIn,
                             messageOut,
                             timeout);
}

bool AkVCam::MessageServer::sendMessage(const std::string &pipeName,
                                        Message *message,
                                        uint32_t timeout)
{
    return sendMessage(std::wstring(pipeName.begin(), pipeName.end()),
                       message,
                       timeout);
}

bool AkVCam::MessageServer::sendMessage(const std::wstring &pipeName,
                                        Message *message,
                                        uint32_t timeout)
{
    return sendMessage(pipeName,
                       *message,
                       message,
                       timeout);
}

bool AkVCam::MessageServer::sendMessage(const std::wstring &pipeName,
                                        const Message &messageIn,
                                        Message *messageOut,
                                        uint32_t timeout)
{
    DWORD bytesTransferred = 0;

    return CallNamedPipe(pipeName.c_str(),
                         const_cast<Message *>(&messageIn),
                         DWORD(sizeof(Message)),
                         messageOut,
                         DWORD(sizeof(Message)),
                         &bytesTransferred,
                         timeout);
}

AkVCam::MessageServerPrivate::MessageServerPrivate(MessageServer *self):
    self(self),
    m_mode(MessageServer::ServerModeReceive),
    m_pipeState(MessageServer::PipeStateGone),
    m_pipe(INVALID_HANDLE_VALUE),
    m_checkInterval(5000),
    m_running(false)
{
    memset(&this->m_overlapped, 0, sizeof(OVERLAPPED));
}

bool AkVCam::MessageServerPrivate::startReceive(bool wait)
{
    AKVCAM_EMIT(this->self, StateChanged, MessageServer::StateAboutToStart)
    bool ok = false;

    // Define who can read and write from pipe.

    /* Define the SDDL for the DACL.
     *
     * https://msdn.microsoft.com/en-us/library/windows/desktop/aa379570(v=vs.85).aspx
     */
    WCHAR descriptor[] =
            L"D:"                   // Discretionary ACL
            L"(D;OICI;GA;;;BG)"     // Deny access to Built-in Guests
            L"(D;OICI;GA;;;AN)"     // Deny access to Anonymous Logon
            L"(A;OICI;GRGWGX;;;AU)" // Allow read/write/execute to Authenticated Users
            L"(A;OICI;GA;;;BA)";    // Allow full control to Administrators

    SECURITY_ATTRIBUTES securityAttributes;
    PSECURITY_DESCRIPTOR securityDescriptor =
            LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);

    if (!securityDescriptor)
        goto startReceive_failed;

    if (!InitializeSecurityDescriptor(securityDescriptor,
                                      SECURITY_DESCRIPTOR_REVISION))
        goto startReceive_failed;

    if (!ConvertStringSecurityDescriptorToSecurityDescriptor(descriptor,
                                                             SDDL_REVISION_1,
                                                             &securityDescriptor,
                                                             nullptr))
        goto startReceive_failed;

    securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    securityAttributes.lpSecurityDescriptor = securityDescriptor;
    securityAttributes.bInheritHandle = TRUE;

    // Create a read/write message type pipe.
    this->m_pipe = CreateNamedPipe(this->m_pipeName.c_str(),
                                   PIPE_ACCESS_DUPLEX
                                   | FILE_FLAG_OVERLAPPED,
                                   PIPE_TYPE_MESSAGE
                                   | PIPE_READMODE_BYTE
                                   | PIPE_WAIT,
                                   PIPE_UNLIMITED_INSTANCES,
                                   sizeof(Message),
                                   sizeof(Message),
                                   NMPWAIT_USE_DEFAULT_WAIT,
                                   &securityAttributes);

    if (this->m_pipe == INVALID_HANDLE_VALUE)
        goto startReceive_failed;

    memset(&this->m_overlapped, 0, sizeof(OVERLAPPED));
    this->m_overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    AKVCAM_EMIT(this->self, StateChanged, MessageServer::StateStarted)
    this->m_running = true;

    if (wait)
        this->messagesLoop();
    else
        this->m_thread =
            std::thread(&MessageServerPrivate::messagesLoop, this);

    ok = true;

startReceive_failed:

    if (!ok) {
        AkLoggerLog("Error starting server: ",
                    errorToString(GetLastError()),
                    " (", GetLastError(), ")");
        AKVCAM_EMIT(this->self, StateChanged, MessageServer::StateStopped)
    }

    if (securityDescriptor)
        LocalFree(securityDescriptor);

    return ok;
}

void AkVCam::MessageServerPrivate::stopReceive(bool wait)
{
    if (!this->m_running)
        return;

    this->m_running = false;
    SetEvent(this->m_overlapped.hEvent);

    if (wait)
        this->m_thread.join();
}

bool AkVCam::MessageServerPrivate::startSend()
{
    this->m_running = true;
    this->m_thread = std::thread(&MessageServerPrivate::checkLoop, this);

    return true;
}

void AkVCam::MessageServerPrivate::stopSend()
{
    if (!this->m_running)
        return;

    this->m_running = false;
    this->m_mutex.lock();
    this->m_exitCheckLoop.notify_all();
    this->m_mutex.unlock();
    this->m_thread.join();
    this->m_pipeState = MessageServer::PipeStateGone;
}

void AkVCam::MessageServerPrivate::messagesLoop()
{
    DWORD bytesTransferred = 0;

    while (this->m_running) {
        HRESULT result = S_OK;

        // Wait for a connection.
        if (!ConnectNamedPipe(this->m_pipe, &this->m_overlapped))
            result = this->waitResult(&bytesTransferred);

        if (result == S_OK) {
            Message message;

            if (this->readMessage(&message)) {
                if (this->m_handlers.count(message.messageId))
                    this->m_handlers[message.messageId](&message);

                this->writeMessage(message);
            }
        }

        DisconnectNamedPipe(this->m_pipe);
    }

    AKVCAM_EMIT(this->self, StateChanged, MessageServer::StateStopped)

    if (this->m_overlapped.hEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(this->m_overlapped.hEvent);
        memset(&this->m_overlapped, 0, sizeof(OVERLAPPED));
    }

    if (this->m_pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(this->m_pipe);
        this->m_pipe = INVALID_HANDLE_VALUE;
    }

    AKVCAM_EMIT(this->self, StateChanged, MessageServer::StateStopped)
}

void AkVCam::MessageServerPrivate::checkLoop()
{
    while (this->m_running) {
        auto result = WaitNamedPipe(this->m_pipeName.c_str(), NMPWAIT_NOWAIT);

        if (result
            && this->m_pipeState != AkVCam::MessageServer::PipeStateAvailable) {
            AkLoggerLog("Pipe Available: ",
                        std::string(this->m_pipeName.begin(),
                                    this->m_pipeName.end()));
            this->m_pipeState = AkVCam::MessageServer::PipeStateAvailable;
            AKVCAM_EMIT(this->self, PipeStateChanged, this->m_pipeState);
        } else if (!result
                   && this->m_pipeState != AkVCam::MessageServer::PipeStateGone
                   && GetLastError() != ERROR_SEM_TIMEOUT) {
            AkLoggerLog("Pipe Gone: ",
                        std::string(this->m_pipeName.begin(),
                                    this->m_pipeName.end()));
            this->m_pipeState = AkVCam::MessageServer::PipeStateGone;
            AKVCAM_EMIT(this->self, PipeStateChanged, this->m_pipeState);
        }

        if (!this->m_running)
            break;

        this->m_mutex.lock();
        this->m_exitCheckLoop.wait_for(this->m_mutex,
                                       std::chrono::milliseconds(this->m_checkInterval));
        this->m_mutex.unlock();
    }
}

HRESULT AkVCam::MessageServerPrivate::waitResult(DWORD *bytesTransferred)
{
    auto lastError = GetLastError();

    if (lastError == ERROR_IO_PENDING) {
        if (WaitForSingleObject(this->m_overlapped.hEvent,
                                INFINITE) == WAIT_OBJECT_0) {
             if (!GetOverlappedResult(this->m_pipe,
                                      &this->m_overlapped,
                                      bytesTransferred,
                                      FALSE))
                 return S_FALSE;
         } else {
             CancelIo(this->m_pipe);

             return S_FALSE;
         }
    } else {
        AkLoggerLog("Wait result failed: ",
                    errorToString(lastError),
                    " (", lastError, ")");

        return E_FAIL;
    }

    return S_OK;
}

bool AkVCam::MessageServerPrivate::readMessage(Message *message)
{
    DWORD bytesTransferred = 0;
    HRESULT result = S_OK;

    if (!ReadFile(this->m_pipe,
                  message,
                  DWORD(sizeof(Message)),
                  &bytesTransferred,
                  &this->m_overlapped))
        result = this->waitResult(&bytesTransferred);

    return result == S_OK;
}

bool AkVCam::MessageServerPrivate::writeMessage(const Message &message)
{
    DWORD bytesTransferred = 0;
    HRESULT result = S_OK;

    if (!WriteFile(this->m_pipe,
                   &message,
                   DWORD(sizeof(Message)),
                   &bytesTransferred,
                   &this->m_overlapped))
        result = this->waitResult(&bytesTransferred);

    return result == S_OK;
}
