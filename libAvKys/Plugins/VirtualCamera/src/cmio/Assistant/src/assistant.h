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

#ifndef ASSISTANT_H
#define ASSISTANT_H

#include <string>
#include <xpc/xpc.h>

namespace AkVCam
{
    class AssistantPrivate;

    class Assistant
    {
        public:
            Assistant();
            Assistant(const Assistant &other) = delete;
            ~Assistant();

            void setTimeout(double timeout);
            void messageReceived(xpc_connection_t client, xpc_object_t event);

        private:
            AssistantPrivate *d;
    };
}

#endif // ASSISTANT_H
