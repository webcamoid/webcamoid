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

#ifndef ASSISTANT_H
#define ASSISTANT_H

#include "assistantglobals.h"

namespace AkVCam
{
    class Assistant
    {
        public:
            Assistant();
            ~Assistant();

            void requestPort(xpc_connection_t client, xpc_object_t event);
            void addPort(xpc_connection_t client, xpc_object_t event);
            void removePortByName(const std::string &portName);
            void removePort(xpc_connection_t client, xpc_object_t event);
            void deviceCreate(xpc_connection_t client, xpc_object_t event);
            void deviceDestroyById(const std::string &deviceId);
            void deviceDestroy(xpc_connection_t client, xpc_object_t event);
            void setBroadcasting(xpc_connection_t client, xpc_object_t event);
            void setMirroring(xpc_connection_t client, xpc_object_t event);
            void setScaling(xpc_connection_t client, xpc_object_t event);
            void setAspectRatio(xpc_connection_t client, xpc_object_t event);
            void frameReady(xpc_connection_t client, xpc_object_t event);
            void devices(xpc_connection_t client, xpc_object_t event);
            void description(xpc_connection_t client, xpc_object_t event);
            void formats(xpc_connection_t client, xpc_object_t event);
            void broadcasting(xpc_connection_t client, xpc_object_t event);
            void mirroring(xpc_connection_t client, xpc_object_t event);
            void scaling(xpc_connection_t client, xpc_object_t event);
            void aspectRatio(xpc_connection_t client, xpc_object_t event);
            void messageReceived(xpc_connection_t client, xpc_object_t event);

        private:
            AssistantServers m_servers;
            AssistantClients m_clients;
            std::map<int64_t, XpcMessage> m_messageHandlers;

            inline static uint64_t id()
            {
                static uint64_t id = 0;

                return id++;
            }
    };
}

#endif // ASSISTANT_H
