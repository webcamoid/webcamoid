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

#ifndef DEVICE_H
#define DEVICE_H

#include <map>
#include <list>
#include <memory>

#include "stream.h"

namespace AkVCam
{
    class Device;
    typedef std::shared_ptr<Device> DevicePtr;

    class Device: public Object
    {
        AKVCAM_SIGNAL(AddListener, const std::string &deviceId)
        AKVCAM_SIGNAL(RemoveListener, const std::string &deviceId)

        public:
            Device(CMIOHardwarePlugInRef pluginInterface,
                   bool createObject=false);
            ~Device();

            OSStatus createObject();
            OSStatus registerObject(bool regist=true);
            StreamPtr addStream();
            std::list<StreamPtr> addStreams(int n);
            OSStatus registerStreams(bool regist=true);
            std::string deviceId() const;
            void setDeviceId(const std::string &deviceId);
            void stopStreams();

            void frameReady(const VideoFrame &frame);
            void setBroadcasting(const std::string &broadcaster);
            void setMirror(bool horizontalMirror, bool verticalMirror);
            void setScaling(Scaling scaling);
            void setAspectRatio(AspectRatio aspectRatio);
            void setSwapRgb(bool swap);

            // Device Interface
            OSStatus suspend();
            OSStatus resume();
            OSStatus startStream(CMIOStreamID stream);
            OSStatus stopStream(CMIOStreamID stream);
            OSStatus processAVCCommand(CMIODeviceAVCCommand *ioAVCCommand);
            OSStatus processRS422Command(CMIODeviceRS422Command *ioRS422Command);

        private:
            std::string m_deviceId;
            std::map<CMIOObjectID, StreamPtr> m_streams;

            void updateStreamsProperty();
    };
}

#endif // DEVICE_H
