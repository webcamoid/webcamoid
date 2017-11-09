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

#ifndef IPCBRIDGE_H
#define IPCBRIDGE_H

#include <vector>
#include <functional>

#include "videoformat.h"

class IpcBridgePrivate;

class IpcBridge
{
    typedef std::function<void (const std::string &server)> ServerAddedCallback;
    typedef std::function<void (const std::string &server)> ServerRemovedCallback;
    typedef std::function<void (const std::string &server,
                                const VideoFormat &format,
                                const void *data)> FrameReadCallback;

    public:
        IpcBridge();
        ~IpcBridge();

        // Server & Client
        std::string listServers() const;
        std::string deviceId(const std::string &server) const;

        // Server
        std::string serverCreate(const std::vector<VideoFormat> &formats,
                                 const std::string &description);
        void serverDestroy(const std::string &server);
        void write(const std::string &server,
                   const VideoFormat &format,
                   const void *data);

        // Client
        void cleanServers();
        bool serverOpen(const std::string &server);
        bool serverClose(const std::string &server);
        void setFrameReadCallback(const std::string &server,
                                  FrameReadCallback callback);
        void setServerAddedCallback(ServerAddedCallback callback);
        void setServerRemovedCallback(ServerRemovedCallback callback);

    private:
        IpcBridgePrivate *d;
};

#endif // IPCBRIDGE_H
