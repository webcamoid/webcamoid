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

/* # Definitions #
 *
 * Server: The program sending video frames, in this case Webcamoid. Each
 *         program has an instance of IpcBridge, and each IpcBridge can create
 *         as many devices as required.
 *
 * Client: The program receiving video frames. Depending on platform, the
 *         program reciving the frames will call the driver and request an
 *         instance to interact with it. Each instance will also contains an
 *         IpcBridge instance.
 *
 * Device Definition: A platform dependent way of announcing clients about
 *         avalible virtual devices. Definitions contains information like the
 *         device description, device id (ie. /dev/video in Linux), and the
 *         formats availble to the program receiving the frames.
 *
 * Bridge: A platform dependent way of transfering video frames from the servers
 *         to the client, it can be for example a memory file mapping, or a
 *         hardware accelerated mapping.
 *
 * # The Server #
 *
 * The server start creating a new virtual device by calling 'deviceCreate',
 * this will create, for example, a file in a folder common to the server and
 * client (ie. a temporal directory), each definition file can be named with
 * the following format:
 *
 *     {process name}-{process pid}
 *
 * For example, let's say you want to create a new device in Webcamoid, the
 * IpcBridge will create a file named:
 *
 *     webcamoid-12345
 *
 * The server will also call 'cleanDevices' regularly to remove definitions
 * files of programs that died or exited without removing it's definition files.
 * A definition file will be removed if there is not any process and pid
 * matching it's filename.
 * The server must call 'deviceDestroy' before exit to cleanup all it's
 * definition files.
 * A server must call 'deviceStart' before sending frames, to create a bridge
 * between servers and clients.
 * The server must send frames in a format that the driver may be able to
 * recognize, and the driver may be able to convert to the formats described in
 * device definition file.
 *
 * # The Device Definition File #
 *
 * The definition file can have any format recognized by the IpcBridge, for
 * example an INI, JSON or XML file. Let's say we choose the INI format, we can
 * define a new device for example as:
 *
 *     [Device_0]
 *     desciption="New virtual camera"
 *     deviceId=/dev/video0
 *     formats=A45F78CD,2348FF1A
 *
 *     [Format_A45F78CD]
 *     format=ARGB
 *     width=640
 *     height=480
 *     fps=30
 *
 *     [Format_2348FF1A]
 *     format=I420
 *     width=1920
 *     height=1280
 *     fps=60
 *
 * Servers and clients may be able to detect when a definition file is created
 * removed or modified and react to it.
 *
 * # The Client #
 *
 * A client can list all available servers, and regularly clean definition files
 * from those other clients that did not cleaned properly.
 * A client subscribe to receive device adding, removal and modification
 * notifications. Also, it must subscribe for receive incoming frames and call
 * 'deviceOpen' after subscribing to start receibing the frames from the bridge.
 */

class IpcBridgePrivate;

class IpcBridge
{
    typedef std::function<void (const std::string &deviceId)> DeviceChangedCallback;
    typedef std::function<void (const std::string &deviceId,
                                const VideoFormat &format,
                                const void *data)> FrameReadCallback;

    public:
        IpcBridge();
        ~IpcBridge();

        /* Server & Client */

        // Remove non-existent devices definitions.
        void cleanDevices();

        // List available servers.
        std::vector<std::string> listDevices(bool all=true) const;

        // Return human readable description of the device.
        std::string description(const std::string &deviceId) const;

        /* Server */

        // Create a device definition.
        std::string deviceCreate(const std::vector<VideoFormat> &formats,
                                 const std::string &description);

        // Remove a device definition.
        void deviceDestroy(const std::string &deviceId);

        // Start frame transfer to the device.
        bool deviceStart(const std::string &deviceId);

        // Stop frame transfer to the device.
        void deviceStop(const std::string &deviceId);

        // Transfer a frame to the device.
        void write(const std::string &deviceId,
                   const VideoFormat &format,
                   const void *data);

        // Set device description.
        void setDescription(const std::string &deviceId,
                            const std::string &description);

        // Set the formats that will be available in the clients.
        void setFormats(const std::string &deviceId,
                        const std::vector<VideoFormat> &formats);

        /* Client */

        // Open device for frames reading.
        bool deviceOpen(const std::string &deviceId);

        // Close device for frames reading.
        bool deviceClose(const std::string &deviceId);

        // Set the function that will be called when a new frame is new frame
        // is available to the client.
        void setFrameReadCallback(const std::string &deviceId,
                                  FrameReadCallback callback);

        // Set the function that will be called when a new device definition
        // is added.
        void setDeviceAddedCallback(DeviceChangedCallback callback);

        // Set the function that will be called when a device definition
        // is removed.
        void setDeviceRemovedCallback(DeviceChangedCallback callback);

        // Set the function that will be called when the parameters on a device
        // definition are changed.
        void setDeviceModifiedCallback(DeviceChangedCallback callback);

    private:
        IpcBridgePrivate *d;
};

#endif // IPCBRIDGE_H
