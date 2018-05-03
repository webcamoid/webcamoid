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

#include <thread>
#include <windows.h>
#include <sddl.h>

#include "messageserver.h"

namespace AkVCam
{
    class MessageServerPrivate
    {
        public:
            std::wstring m_pipeName;
            std::map<uint32_t, MessageHandler> m_handlers;
            StateChangedCallBack m_stateChangedCallBack;
            void *m_userData;
            HANDLE m_pipe;
            OVERLAPPED m_overlapped;
            std::thread m_thread;
            bool m_running;

            void messagesLoop();
            HRESULT waitResult(DWORD *bytesTransferred);
            bool readMessage(Message *message);
            bool writeMessage(const Message &message);
    };
}

AkVCam::MessageServer::MessageServer()
{
    this->d = new MessageServerPrivate;
    this->d->m_stateChangedCallBack = nullptr;
    this->d->m_userData = nullptr;
    this->d->m_pipe = INVALID_HANDLE_VALUE;
    memset(&this->d->m_overlapped, 0, sizeof(OVERLAPPED));
    this->d->m_running = false;
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

void AkVCam::MessageServer::setHandlers(const std::map<uint32_t, MessageHandler> &handlers)
{
    this->d->m_handlers = handlers;
}

void AkVCam::MessageServer::setStateChangedCallBack(StateChangedCallBack callback,
                                                    void *userData)
{
    this->d->m_stateChangedCallBack = callback;
    this->d->m_userData = userData;
}

bool AkVCam::MessageServer::start(bool wait)
{
    if (this->d->m_stateChangedCallBack)
        this->d->m_stateChangedCallBack(StateAboutToStart, this->d->m_userData);

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
    this->d->m_pipe = CreateNamedPipe(this->d->m_pipeName.c_str(),
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

    if (this->d->m_pipe == INVALID_HANDLE_VALUE)
        goto init_failed;

    memset(&this->d->m_overlapped, 0, sizeof(OVERLAPPED));
    this->d->m_overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    if (this->d->m_stateChangedCallBack)
        this->d->m_stateChangedCallBack(StateStarted, this->d->m_userData);


    this->d->m_running = true;

    if (wait)
        this->d->messagesLoop();
    else
        this->d->m_thread =
                std::thread(&MessageServerPrivate::messagesLoop, this->d);

    ok = true;

init_failed:

    if (!ok && this->d->m_stateChangedCallBack)
        this->d->m_stateChangedCallBack(StateStopped, this->d->m_userData);

    if (securityDescriptor)
        LocalFree(securityDescriptor);

    return ok;
}

void AkVCam::MessageServer::stop(bool wait)
{
    if (!this->d->m_running)
        return;

    this->d->m_running = false;
    SetEvent(this->d->m_overlapped.hEvent);

    if (wait)
        this->d->m_thread.join();
}

bool AkVCam::MessageServer::sendMessage(const std::wstring &pipeName,
                                        AkVCam::Message *message)
{
    return sendMessage(pipeName, *message, message);
}

bool AkVCam::MessageServer::sendMessage(const std::wstring &pipeName,
                                        const AkVCam::Message &messageIn,
                                        AkVCam::Message *messageOut)
{
    DWORD bytesTransferred = 0;

    return CallNamedPipe(pipeName.c_str(),
                         const_cast<Message *>(&messageIn),
                         DWORD(sizeof(Message)),
                         messageOut,
                         DWORD(sizeof(Message)),
                         &bytesTransferred,
                         NMPWAIT_WAIT_FOREVER);
}

void AkVCam::MessageServerPrivate::messagesLoop()
{
    DWORD bytesTransferred = 0;

    while (this->m_running) {
        HRESULT result = S_OK;

        // Wait for a connection.
        if (!ConnectNamedPipe(this->m_pipe, &this->m_overlapped))
            result = this->waitResult(&bytesTransferred);

        if (result == E_FAIL) {
            continue;
        } else if (result == S_OK) {
            Message message;

            if (this->readMessage(&message)) {
                if (this->m_handlers.count(message.messageId))
                    this->m_handlers[message.messageId](&message);

                this->writeMessage(message);
            }
        }

        DisconnectNamedPipe(this->m_pipe);
    }

    if (this->m_stateChangedCallBack)
        this->m_stateChangedCallBack(StateStopped, this->m_userData);

    if (this->m_overlapped.hEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(this->m_overlapped.hEvent);
        memset(&this->m_overlapped, 0, sizeof(OVERLAPPED));
    }

    if (this->m_pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(this->m_pipe);
        this->m_pipe = INVALID_HANDLE_VALUE;
    }

    if (this->m_stateChangedCallBack)
        this->m_stateChangedCallBack(StateStopped, this->m_userData);
}

HRESULT AkVCam::MessageServerPrivate::waitResult(DWORD *bytesTransferred)
{
    if (GetLastError() == ERROR_IO_PENDING) {
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
