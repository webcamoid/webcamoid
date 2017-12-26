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

#include <vector>

#include "device.h"
#include "utils.h"

AkVCam::Device::Device(CMIOHardwarePlugInRef pluginInterface,
                       bool registerObject):
    AkVCam::Object(pluginInterface)
{
    this->m_className = "Device";
    this->m_classID = kCMIODeviceClassID;

    if (registerObject) {
        this->createObject();
        this->registerObject();
    }
}

AkVCam::Device::~Device()
{
    this->registerStreams(false);
    this->registerObject(false);
}

OSStatus AkVCam::Device::createObject()
{
    AkObjectLogMethod();

    if (!this->m_pluginInterface
        || !*this->m_pluginInterface)
        return kCMIOHardwareUnspecifiedError;

    CMIOObjectID deviceID = 0;

    auto status =
            CMIOObjectCreate(this->m_pluginInterface,
                             kCMIOObjectSystemObject,
                             this->m_classID,
                             &deviceID);

    if (status == kCMIOHardwareNoError) {
        this->m_isCreated = true;
        this->m_objectID = deviceID;
        AkLoggerLog("Created device: " << this->m_objectID);
    }

    return status;
}

OSStatus AkVCam::Device::registerObject(bool regist)
{
    AkObjectLogMethod();
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!this->m_isCreated
        || !this->m_pluginInterface
        || !*this->m_pluginInterface)
        return status;

    if (regist) {
        status = CMIOObjectsPublishedAndDied(this->m_pluginInterface,
                                             kCMIOObjectSystemObject,
                                             1,
                                             &this->m_objectID,
                                             0,
                                             nullptr);
    } else {
        status = CMIOObjectsPublishedAndDied(this->m_pluginInterface,
                                             kCMIOObjectSystemObject,
                                             0,
                                             nullptr,
                                             1,
                                             &this->m_objectID);
    }

    return status;
}

AkVCam::StreamPtr AkVCam::Device::addStream()
{
    AkObjectLogMethod();
    auto stream = StreamPtr(new Stream(false, this));

    if (stream->createObject() == kCMIOHardwareNoError) {
        this->m_streams[stream->objectID()] = stream;
        this->updateStreamsProperty();

        return stream;
    }

    return StreamPtr();
}

std::list<AkVCam::StreamPtr> AkVCam::Device::addStreams(int n)
{
    AkObjectLogMethod();
    std::list<StreamPtr> streams;

    for (int i = 0; i < n; i++) {
        auto stream = StreamPtr(new Stream(false, this));

        if (stream->createObject() != kCMIOHardwareNoError)
            return std::list<StreamPtr>();

        streams.push_back(stream);
    }

    for (auto &stream: streams) {
        this->m_streams[stream->objectID()] = stream;
        this->updateStreamsProperty();
    }

    return streams;
}

OSStatus AkVCam::Device::registerStreams(bool regist)
{
    AkObjectLogMethod();
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!this->m_isCreated
        || !this->m_pluginInterface
        || !*this->m_pluginInterface
        || this->m_streams.empty())
        return status;

    std::vector<CMIOObjectID> streams;

    for (auto &stream: this->m_streams)
        streams.push_back(stream.first);

    if (regist) {
        status = CMIOObjectsPublishedAndDied(this->m_pluginInterface,
                                             kCMIOObjectSystemObject,
                                             UInt32(streams.size()),
                                             streams.data(),
                                             0,
                                             nullptr);
    } else {
        status = CMIOObjectsPublishedAndDied(this->m_pluginInterface,
                                             kCMIOObjectSystemObject,
                                             0,
                                             nullptr,
                                             UInt32(streams.size()),
                                             streams.data());
    }

    return status;
}

std::string AkVCam::Device::deviceId() const
{
    return this->m_deviceId;
}

void AkVCam::Device::setDeviceId(const std::string &deviceId) const
{
    this->m_deviceId = deviceId;
}

void AkVCam::Device::frameReady(const AkVCam::VideoFrame &frame)
{
    for (auto &stream: this->m_streams)
        stream.second->frameReady(frame);
}

void AkVCam::Device::setBroadcasting(bool broadcasting)
{
    for (auto &stream: this->m_streams)
        stream.second->setBroadcasting(broadcasting);
}

void AkVCam::Device::setMirror(bool horizontalMirror, bool verticalMirror)
{
    for (auto &stream: this->m_streams)
        stream.second->setMirror(horizontalMirror, verticalMirror);
}

void AkVCam::Device::setScaling(AkVCam::VideoFrame::Scaling scaling)
{
    for (auto &stream: this->m_streams)
        stream.second->setScaling(scaling);
}

OSStatus AkVCam::Device::suspend()
{
    AkObjectLogMethod();

    AkLoggerLog("STUB");

    return kCMIOHardwareUnspecifiedError;
}

OSStatus AkVCam::Device::resume()
{
    AkObjectLogMethod();

    AkLoggerLog("STUB");

    return kCMIOHardwareUnspecifiedError;
}

OSStatus AkVCam::Device::startStream(CMIOStreamID stream)
{
    AkObjectLogMethod();

    UInt32 isRunning = 0;
    this->m_properties.getProperty(kCMIODevicePropertyDeviceIsRunning,
                                   &isRunning);

    if (isRunning)
        return kCMIOHardwareUnspecifiedError;

    if (!this->m_streams.count(stream))
        return kCMIOHardwareNotRunningError;

    if (!this->m_streams[stream]->start())
        return kCMIOHardwareNotRunningError;

    bool deviceRunning = true;

    for (auto &stream: this->m_streams)
        deviceRunning &= stream.second->running();

    if (deviceRunning) {
        this->m_properties.setProperty(kCMIODevicePropertyDeviceIsRunning,
                                       UInt32(1));
        auto address = this->address(kCMIODevicePropertyDeviceIsRunning);
        this->propertyChanged(1, &address);
    }

    return kCMIOHardwareNoError;
}

OSStatus AkVCam::Device::stopStream(CMIOStreamID stream)
{
    AkObjectLogMethod();

    UInt32 isRunning = 0;
    this->m_properties.getProperty(kCMIODevicePropertyDeviceIsRunning,
                                   &isRunning);

    if (!isRunning)
        return kCMIOHardwareNotRunningError;

    if (!this->m_streams.count(stream))
        return kCMIOHardwareNotRunningError;

    this->m_streams[stream]->stop();
    bool deviceRunning = false;

    for (auto &stream: this->m_streams)
        deviceRunning |= stream.second->running();

    if (!deviceRunning) {
        this->m_properties.setProperty(kCMIODevicePropertyDeviceIsRunning,
                                       UInt32(0));
        auto address = this->address(kCMIODevicePropertyDeviceIsRunning);
        this->propertyChanged(1, &address);
    }

    return kCMIOHardwareNoError;
}

OSStatus AkVCam::Device::processAVCCommand(CMIODeviceAVCCommand *ioAVCCommand)
{
    AkObjectLogMethod();
    UNUSED(ioAVCCommand)

    AkLoggerLog("STUB");

    return kCMIOHardwareUnspecifiedError;
}

OSStatus AkVCam::Device::processRS422Command(CMIODeviceRS422Command *ioRS422Command)
{
    AkObjectLogMethod();
    UNUSED(ioRS422Command)

    AkLoggerLog("STUB");

    return kCMIOHardwareUnspecifiedError;
}

void AkVCam::Device::updateStreamsProperty()
{
    std::vector<ObjectPtr> streams;

    for (auto &stream: this->m_streams)
        streams.push_back(stream.second);

    this->m_properties.setProperty(kCMIODevicePropertyStreams, streams);
}
