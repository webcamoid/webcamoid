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

#include "ipcbridge.h"

class IpcBridgePrivate
{
    public:
};

IpcBridge::IpcBridge()
{
    this->d = new IpcBridgePrivate();
}

IpcBridge::~IpcBridge()
{
    delete this->d;
}

std::string IpcBridge::listServers() const
{

}

std::string IpcBridge::deviceId(const std::string &server) const
{

}

std::string IpcBridge::serverCreate(const std::vector<VideoFormat> &formats,
                                    const std::string &description)
{

}

void IpcBridge::serverDestroy(const std::string &server)
{

}

void IpcBridge::write(const std::string &server,
                      const VideoFormat &format,
                      const void *data)
{

}

void IpcBridge::cleanServers()
{

}

bool IpcBridge::serverOpen(const std::string &server)
{

}

bool IpcBridge::serverClose(const std::string &server)
{

}

void IpcBridge::setFrameReadCallback(const std::string &server,
                                     IpcBridge::FrameReadCallback callback)
{

}

void IpcBridge::setServerAddedCallback(IpcBridge::ServerAddedCallback callback)
{

}

void IpcBridge::setServerRemovedCallback(IpcBridge::ServerRemovedCallback callback)
{

}
