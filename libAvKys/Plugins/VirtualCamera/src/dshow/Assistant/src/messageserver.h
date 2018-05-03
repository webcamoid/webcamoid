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

#ifndef MESSAGESERVER_H
#define MESSAGESERVER_H

#include <map>

#include "assistantglobals.h"

namespace AkVCam
{
    enum State
    {
        StateAboutToStart,
        StateStarted,
        StateAboutToStop,
        StateStopped
    };

    typedef void (*StateChangedCallBack)(State state, void *userData);

    class MessageServerPrivate;

    class MessageServer
    {
        public:
            MessageServer();
            ~MessageServer();

            std::wstring pipeName() const;
            std::wstring &pipeName();
            void setPipeName(const std::wstring &pipeName);
            void setHandlers(const std::map<uint32_t, MessageHandler> &handlers);
            void setStateChangedCallBack(StateChangedCallBack callback,
                                         void *userData);
            bool start(bool wait=false);
            void stop(bool wait=false);
            static bool sendMessage(const std::wstring &pipeName,
                                    Message *message);
            static bool sendMessage(const std::wstring &pipeName,
                                    const Message &messageIn,
                                    Message *messageOut);

        private:
            MessageServerPrivate *d;
    };
}

#endif // MESSAGESERVER_H
