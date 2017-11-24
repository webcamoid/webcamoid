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

#include <sstream>
#include <mach/mach.h>
#include <servers/bootstrap.h>

#include "ipcbridge.h"
#include "../../Assistant/src/assistantglobals.h"

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

void IpcBridge::cleanDevices()
{

}

std::vector<std::string> IpcBridge::listDevices(bool all) const
{
    return {};
}

std::string IpcBridge::description(const std::string &deviceId) const
{
    return {};
}

std::string IpcBridge::deviceCreate(const std::vector<AkVCam::VideoFormat> &formats,
                                    const std::string &description)
{
    return {};
}

void IpcBridge::deviceDestroy(const std::string &deviceId)
{

}

bool IpcBridge::deviceStart(const std::string &deviceId)
{
    return false;
}

void IpcBridge::deviceStop(const std::string &deviceId)
{

}

void IpcBridge::write(const std::string &deviceId,
                      const AkVCam::VideoFormat &format,
                      const void *data)
{

}

void IpcBridge::setDescription(const std::string &deviceId,
                               const std::string &description)
{

}

void IpcBridge::setFormats(const std::string &deviceId,
                           const std::vector<AkVCam::VideoFormat> &formats)
{

}

bool IpcBridge::deviceOpen(const std::string &deviceId)
{
    return false;
}

bool IpcBridge::deviceClose(const std::string &deviceId)
{
    return false;
}

void IpcBridge::setFrameReadCallback(const std::string &deviceId,
                                     FrameReadCallback callback)
{

}

void IpcBridge::setDeviceAddedCallback(DeviceChangedCallback callback)
{

}

void IpcBridge::setDeviceRemovedCallback(DeviceChangedCallback callback)
{

}

void IpcBridge::setDeviceModifiedCallback(DeviceChangedCallback callback)
{

}

bool IpcBridge::startAssistant()
{
    mach_port_t assistantServicePort;
    name_t assistantServiceName = AKVCAM_ASSISTANT_NAME;
    auto result = bootstrap_look_up(bootstrap_port,
                                    assistantServiceName,
                                    &assistantServicePort);
    mach_port_deallocate(mach_task_self(), assistantServicePort);

    if (result != BOOTSTRAP_SUCCESS) {
        std::stringstream launchctl;
        launchctl << "launchctl load -w /Library/LaunchDaemons/"
                  << AKVCAM_ASSISTANT_NAME
                  << ".plist";
        result = system(launchctl.str().c_str());

        return result == BOOTSTRAP_SUCCESS;
    }

    return true;
}
