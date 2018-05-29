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

#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include "device.h"

namespace AkVCam
{
    struct PluginInterfacePrivate;

    class PluginInterface: public ObjectInterface
    {
        public:
            explicit PluginInterface();
            ~PluginInterface();

            CMIOObjectID objectID() const;
            static CMIOHardwarePlugInRef create();
            Object *findObject(CMIOObjectID objectID);

            HRESULT QueryInterface(REFIID uuid, LPVOID *interface);
            OSStatus Initialize();
            OSStatus InitializeWithObjectID(CMIOObjectID objectID);
            OSStatus Teardown();

        private:
            PluginInterfacePrivate *d;
            CMIOObjectID m_objectID;
            std::vector<DevicePtr> m_devices;

            void deviceAdded(const std::string &deviceId);
            void deviceRemoved(const std::string &deviceId);
            void frameReady(const std::string &deviceId,
                            const VideoFrame &frame);
            void setBroadcasting(const std::string &deviceId,
                                 bool broadcasting);
            void setMirror(const std::string &deviceId,
                           bool horizontalMirror,
                           bool verticalMirror);
            void setScaling(const std::string &deviceId,
                            Scaling scaling);
            void setAspectRatio(const std::string &deviceId,
                                AspectRatio aspectRatio);
            void setSwapRgb(const std::string &deviceId, bool swap);
            void addListener(const std::string &deviceId);
            void removeListener(const std::string &deviceId);
            bool createDevice(const std::string &deviceId,
                              const std::string &description,
                              const std::vector<VideoFormat> &formats);
            void destroyDevice(const std::string &deviceId);
    };
}

#endif // PLUGININTERFACE_H
