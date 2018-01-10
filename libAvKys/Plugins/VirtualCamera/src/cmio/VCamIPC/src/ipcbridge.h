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

#include "VCamUtils/src/image/videoformattypes.h"
#include "VCamUtils/src/image/videoframetypes.h"

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
        typedef std::function<void (const std::string &deviceId,
                                    bool horizontalMirror,
                                    bool verticalMirror)> MirrorChangedCallback;
        typedef std::function<void (const std::string &deviceId,
                                    Scaling scaling)> ScalingChangedCallback;
        typedef std::function<void (const std::string &deviceId,
                                    AspectRatio aspectRatio)> AspectRatioChangedCallback;
        typedef std::function<void (const std::string &deviceId,
                                    int listeners)> ListenersChangedCallback;

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

            // Output pixel formats supported by the driver.
            std::vector<PixelFormat> supportedOutputPixelFormats() const;

            // Return supported formats for the device.
            std::vector<VideoFormat> formats(const std::string &deviceId) const;

            // Return return the status of the device.
            bool broadcasting(const std::string &deviceId) const;

            // Device is horizontal mirrored,
            bool isHorizontalMirrored(const std::string &deviceId);

            // Device is vertical mirrored,
            bool isVerticalMirrored(const std::string &deviceId);

            // Scaling mode for frames shown in clients.
            Scaling scalingMode(const std::string &deviceId);

            // Aspect ratio mode for frames shown in clients.
            AspectRatio aspectRatioMode(const std::string &deviceId);

            // How many programs are using the virtual camera now.
            int listeners(const std::string &deviceId);

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
            bool write(const std::string &deviceId,
                       const VideoFrame &frame);

            // Set mirroring options for device,
            void setMirroring(const std::string &deviceId,
                              bool horizontalMirrored,
                              bool verticalMirrored);

            // Set scaling options for device.
            void setScaling(const std::string &deviceId,
                            Scaling scaling);

            // Set aspect ratio options for device.
            void setAspectRatio(const std::string &deviceId,
                                AspectRatio aspectRatio);

            // Set the function that will be called when the number of listeners
            // changes for a device.
            void setListenersChangedCallback(ListenersChangedCallback callback);

            /* Client */

            // Increment the count of device listeners
            bool addListener(const std::string &deviceId);

            // Decrement the count of device listeners
            bool removeListener(const std::string &deviceId);

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

            // Set the function that will be called when the mirroring option
            // changes for a device.
            void setMirrorChangedCallback(MirrorChangedCallback callback);

            // Set the function that will be called when the scaling option
            // changes for a device.
            void setScalingChangedCallback(ScalingChangedCallback callback);

            // Set the function that will be called when the aspect ratio option
            // changes for a device.
            void setAspectRatioChangedCallback(AspectRatioChangedCallback callback);

        private:
            IpcBridgePrivate *d;

        friend class IpcBridgePrivate;
    };
}

#endif // IPCBRIDGE_H
