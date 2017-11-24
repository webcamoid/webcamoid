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

            CFDataRef requestPort(bool asClient) const;
            CFDataRef addPort(const std::string &portName);
            CFDataRef removePort(const std::string &port);
            CFDataRef deviceCreate(const std::string &port,
                                   const std::string &description,
                                   const std::vector<VideoFormat> &formats);
            CFDataRef deviceDestroy(const std::string &deviceId);
            CFDataRef frameReady(CFDataRef data) const;
            CFDataRef devices() const;
            CFDataRef description(const std::string &deviceId) const;
            CFDataRef formats(const std::string &deviceId) const;
            static CFDataRef messageReceived(CFMessagePortRef local,
                                             SInt32 msgid,
                                             CFDataRef data,
                                             void *info);
            void messagePortInvalidated(CFMessagePortRef messagePort);

        private:
            AssistantServers m_servers;
            AssistantClients m_clients;

            inline static uint64_t id()
            {
                static uint64_t id = 0;

                return id++;
            }
    };
}

#endif // ASSISTANT_H
