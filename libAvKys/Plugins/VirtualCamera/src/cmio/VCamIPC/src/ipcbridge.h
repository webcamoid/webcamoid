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

#include <string>
#include <vector>
#include <functional>

namespace AkVCam
{
    class IpcBridgePrivate;
    class VideoFormat;
    class VideoFrame;

    class IpcBridge
    {
        typedef std::function<void (const std::string &deviceId)> DeviceChangedCallback;
        typedef std::function<void (const std::string &deviceId,
                                    const VideoFrame &frame)> FrameReadyCallback;
        typedef std::function<void (const std::string &deviceId,
                                    bool broadcasting)> BroadcastingChangedCallback;

        public:
            IpcBridge();
            ~IpcBridge();

            /* Server & Client */

            // Register the end point to the global server.
            bool registerEndPoint(bool asClient);

            // Unregister the end point to the global server.
            void unregisterEndPoint();

            // List available servers.
            std::vector<std::string> listDevices(bool all=true) const;

            // Return human readable description of the device.
            std::string description(const std::string &deviceId) const;

            // Return supported formats for the device.
            std::vector<VideoFormat> formats(const std::string &deviceId) const;

            // Return return the status of the device.
            bool broadcasting(const std::string &deviceId) const;

            /* Server */

            // Create a device definition.
            std::string deviceCreate(const std::string &description,
                                     const std::vector<VideoFormat> &formats);

            // Remove a device definition.
            void deviceDestroy(const std::string &deviceId);

            // Start frame transfer to the device.
            bool deviceStart(const std::string &deviceId);

            // Stop frame transfer to the device.
            void deviceStop(const std::string &deviceId);

            // Transfer a frame to the device.
            void write(const std::string &deviceId,
                       const VideoFrame &frame);

            /* Client */

            // Open device for frames reading.
            bool deviceOpen(const std::string &deviceId);

            // Close device for frames reading.
            bool deviceClose(const std::string &deviceId);

            // Set the function that will be called when a new frame
            // is available to the client.
            void setFrameReadyCallback(FrameReadyCallback callback);

            // Set the function that will be called when a new device definition
            // is added.
            void setDeviceAddedCallback(DeviceChangedCallback callback);

            // Set the function that will be called when a device definition
            // is removed.
            void setDeviceRemovedCallback(DeviceChangedCallback callback);

            // Set the function that will be called when a device is transmitting
            // video frames.
            void setBroadcastingChangedCallback(BroadcastingChangedCallback callback);

        private:
            IpcBridgePrivate *d;

        friend class IpcBridgePrivate;
    };
}

#endif // IPCBRIDGE_H
