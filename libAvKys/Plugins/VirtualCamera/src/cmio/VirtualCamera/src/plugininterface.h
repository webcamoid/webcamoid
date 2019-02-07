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

#include "ipcbridge.h"
#include "device.h"

namespace AkVCam
{
    struct PluginInterfacePrivate;

    class PluginInterface: public ObjectInterface
    {
        public:
            PluginInterface();
            PluginInterface(const PluginInterface &other) = delete;
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

            static void serverStateChanged(void *userData,
                                           IpcBridge::ServerState state);
            static void deviceAdded(void *userData,
                                    const std::string &deviceId);
            static void deviceRemoved(void *userData,
                                      const std::string &deviceId);
            static void frameReady(void *userData,
                                   const std::string &deviceId,
                                   const VideoFrame &frame);
            static void setBroadcasting(void *userData,
                                        const std::string &deviceId,
                                        const std::string &broadcaster);
            static void setMirror(void *userData,
                                  const std::string &deviceId,
                                  bool horizontalMirror,
                                  bool verticalMirror);
            static void setScaling(void *userData,
                                   const std::string &deviceId,
                                   Scaling scaling);
            static void setAspectRatio(void *userData,
                                       const std::string &deviceId,
                                       AspectRatio aspectRatio);
            static void setSwapRgb(void *userData,
                                   const std::string &deviceId,
                                   bool swap);
            static void addListener(void *userData,
                                    const std::string &deviceId);
            static void removeListener(void *userData,
                                       const std::string &deviceId);
            bool createDevice(const std::string &deviceId,
                              const std::wstring &description,
                              const std::vector<VideoFormat> &formats);
            void destroyDevice(const std::string &deviceId);

        friend struct PluginInterfacePrivate;
    };
}

#endif // PLUGININTERFACE_H
