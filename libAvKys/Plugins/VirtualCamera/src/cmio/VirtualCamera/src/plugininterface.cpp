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
#include <algorithm>
#include <sys/stat.h>
#include <IOKit/audio/IOAudioTypes.h>

#include "plugininterface.h"
#include "utils.h"
#include "VCamIPC/src/ipcbridge.h"
#include "Assistant/src/assistantglobals.h"

#define AkPluginPrivateIntefaceLog() \
    AkLoggerLog("PluginInterfacePrivate::" << __FUNCTION__ << "()")

#define AkPluginPrivateIntefaceLogID(x) \
    AkLoggerLog("PluginInterfacePrivate::" \
                << __FUNCTION__ \
                << "(id = " << x << ")")

namespace AkVCam
{
    struct PluginInterfacePrivate
    {
        public:
            CMIOHardwarePlugInInterface *pluginInterface;
            PluginInterface *self;
            ULONG m_ref;
            ULONG m_reserved;
            AkVCam::IpcBridge m_ipcBridge;

            static HRESULT QueryInterface(void *self,
                                          REFIID uuid,
                                          LPVOID *interface)
            {
                AkPluginPrivateIntefaceLog();

                if (!self)
                    return E_FAIL;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

                return _self->self->QueryInterface(uuid, interface);
            }

            static ULONG AddRef(void *self)
            {
                AkPluginPrivateIntefaceLog();

                if (!self)
                    return 0;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
                _self->m_ref++;

                return _self->m_ref;
            }

            static ULONG Release(void *self)
            {
                AkPluginPrivateIntefaceLog();

                if (!self)
                    return 0;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

                if (_self->m_ref > 0) {
                    _self->m_ref--;

                    if (_self->m_ref < 1) {
                        delete _self->self;

                        return 0UL;
                    }
                }

                return _self->m_ref;
            }

            static OSStatus Initialize(CMIOHardwarePlugInRef self)
            {
                AkPluginPrivateIntefaceLog();

                if (!self)
                    return kCMIOHardwareUnspecifiedError;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

                return _self->self->Initialize();
            }

            static OSStatus InitializeWithObjectID(CMIOHardwarePlugInRef self,
                                                   CMIOObjectID objectID)
            {
                AkPluginPrivateIntefaceLog();

                if (!self)
                    return kCMIOHardwareUnspecifiedError;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

                return _self->self->InitializeWithObjectID(objectID);
            }

            static OSStatus Teardown(CMIOHardwarePlugInRef self)
            {
                AkPluginPrivateIntefaceLog();

                if (!self)
                    return kCMIOHardwareUnspecifiedError;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

                return _self->self->Teardown();
            }

            static void ObjectShow(CMIOHardwarePlugInRef self,
                                   CMIOObjectID objectID)
            {
                AkPluginPrivateIntefaceLogID(objectID);

                if (!self)
                    return;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

                if (_self->self->objectID() == objectID)
                    _self->self->show();
                else if (auto object = _self->self->findObject(objectID))
                    object->show();
            }

            static Boolean ObjectHasProperty(CMIOHardwarePlugInRef self,
                                             CMIOObjectID objectID,
                                             const CMIOObjectPropertyAddress *address)
            {
                AkPluginPrivateIntefaceLogID(objectID);
                Boolean result = false;

                if (!self)
                    return result;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

                if (_self->self->objectID() == objectID)
                    result = _self->self->hasProperty(address);
                else if (auto object = _self->self->findObject(objectID))
                    result = object->hasProperty(address);

                return result;
            }

            static OSStatus ObjectIsPropertySettable(CMIOHardwarePlugInRef self,
                                                     CMIOObjectID objectID,
                                                     const CMIOObjectPropertyAddress *address,
                                                     Boolean *isSettable)
            {
                AkPluginPrivateIntefaceLogID(objectID);
                OSStatus status = kCMIOHardwareUnspecifiedError;

                if (!self)
                    return status;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

                if (_self->self->objectID() == objectID)
                    status = _self->self->isPropertySettable(address,
                                                             isSettable);
                else if (auto object = _self->self->findObject(objectID))
                    status = object->isPropertySettable(address,
                                                        isSettable);

                return status;
            }

            static OSStatus ObjectGetPropertyDataSize(CMIOHardwarePlugInRef self,
                                                      CMIOObjectID objectID,
                                                      const CMIOObjectPropertyAddress *address,
                                                      UInt32 qualifierDataSize,
                                                      const void *qualifierData,
                                                      UInt32 *dataSize)
            {
                AkPluginPrivateIntefaceLogID(objectID);
                OSStatus status = kCMIOHardwareUnspecifiedError;

                if (!self)
                    return status;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

                if (_self->self->objectID() == objectID)
                    status = _self->self->getPropertyDataSize(address,
                                                              qualifierDataSize,
                                                              qualifierData,
                                                              dataSize);
                else if (auto object = _self->self->findObject(objectID))
                    status = object->getPropertyDataSize(address,
                                                         qualifierDataSize,
                                                         qualifierData,
                                                         dataSize);

                return status;
            }

            static OSStatus ObjectGetPropertyData(CMIOHardwarePlugInRef self,
                                                  CMIOObjectID objectID,
                                                  const CMIOObjectPropertyAddress *address,
                                                  UInt32 qualifierDataSize,
                                                  const void *qualifierData,
                                                  UInt32 dataSize,
                                                  UInt32 *dataUsed,
                                                  void *data)
            {
                AkPluginPrivateIntefaceLogID(objectID);
                OSStatus status = kCMIOHardwareUnspecifiedError;

                if (!self)
                    return status;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

                if (_self->self->objectID() == objectID)
                    status = _self->self->getPropertyData(address,
                                                          qualifierDataSize,
                                                          qualifierData,
                                                          dataSize,
                                                          dataUsed,
                                                          data);
                else if (auto object = _self->self->findObject(objectID))
                    status = object->getPropertyData(address,
                                                     qualifierDataSize,
                                                     qualifierData,
                                                     dataSize,
                                                     dataUsed,
                                                     data);

                return status;
            }

            static OSStatus ObjectSetPropertyData(CMIOHardwarePlugInRef self,
                                                  CMIOObjectID objectID,
                                                  const CMIOObjectPropertyAddress *address,
                                                  UInt32 qualifierDataSize,
                                                  const void *qualifierData,
                                                  UInt32 dataSize,
                                                  const void *data)
            {
                AkPluginPrivateIntefaceLogID(objectID);
                OSStatus status = kCMIOHardwareUnspecifiedError;

                if (!self)
                    return status;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

                if (_self->self->objectID() == objectID)
                    status = _self->self->setPropertyData(address,
                                                          qualifierDataSize,
                                                          qualifierData,
                                                          dataSize,
                                                          data);
                else if (auto object = _self->self->findObject(objectID))
                    status = object->setPropertyData(address,
                                                     qualifierDataSize,
                                                     qualifierData,
                                                     dataSize,
                                                     data);

                return status;
            }

            static OSStatus DeviceSuspend(CMIOHardwarePlugInRef self,
                                          CMIODeviceID device)
            {
                AkPluginPrivateIntefaceLogID(device);
                OSStatus status = kCMIOHardwareUnspecifiedError;

                if (!self)
                    return status;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
                auto object = reinterpret_cast<Device *>(_self->self->findObject(device));

                if (object)
                    status = object->suspend();

                return status;
            }

            static OSStatus DeviceResume(CMIOHardwarePlugInRef self,
                                         CMIODeviceID device)
            {
                AkPluginPrivateIntefaceLogID(device);
                OSStatus status = kCMIOHardwareUnspecifiedError;

                if (!self)
                    return status;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
                auto object = reinterpret_cast<Device *>(_self->self->findObject(device));

                if (object)
                    status = object->resume();

                return status;
            }

            static OSStatus DeviceStartStream(CMIOHardwarePlugInRef self,
                                              CMIODeviceID device,
                                              CMIOStreamID stream)
            {
                AkPluginPrivateIntefaceLogID(device);
                OSStatus status = kCMIOHardwareUnspecifiedError;

                if (!self)
                    return status;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
                auto object = reinterpret_cast<Device *>(_self->self->findObject(device));

                if (object)
                    status = object->startStream(stream);

                return status;
            }

            static OSStatus DeviceStopStream(CMIOHardwarePlugInRef self,
                                             CMIODeviceID device,
                                             CMIOStreamID stream)
            {
                AkPluginPrivateIntefaceLogID(device);
                OSStatus status = kCMIOHardwareUnspecifiedError;

                if (!self)
                    return status;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
                auto object = reinterpret_cast<Device *>(_self->self->findObject(device));

                if (object)
                    status = object->stopStream(stream);

                return status;
            }

            static OSStatus DeviceProcessAVCCommand(CMIOHardwarePlugInRef self,
                                                    CMIODeviceID device,
                                                    CMIODeviceAVCCommand *ioAVCCommand)
            {
                AkPluginPrivateIntefaceLogID(device);
                OSStatus status = kCMIOHardwareUnspecifiedError;

                if (!self)
                    return status;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
                auto object = reinterpret_cast<Device *>(_self->self->findObject(device));

                if (object)
                    status = object->processAVCCommand(ioAVCCommand);

                return status;
            }

            static OSStatus DeviceProcessRS422Command(CMIOHardwarePlugInRef self,
                                                      CMIODeviceID device,
                                                      CMIODeviceRS422Command *ioRS422Command)
            {
                AkPluginPrivateIntefaceLogID(device);
                OSStatus status = kCMIOHardwareUnspecifiedError;

                if (!self)
                    return status;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
                auto object = reinterpret_cast<Device *>(_self->self->findObject(device));

                if (object)
                    status = object->processRS422Command(ioRS422Command);

                return status;
            }

            static OSStatus StreamCopyBufferQueue(CMIOHardwarePlugInRef self,
                                                  CMIOStreamID stream,
                                                  CMIODeviceStreamQueueAlteredProc queueAlteredProc,
                                                  void *queueAlteredRefCon,
                                                  CMSimpleQueueRef *queue)
            {
                AkPluginPrivateIntefaceLogID(stream);
                OSStatus status = kCMIOHardwareUnspecifiedError;

                if (!self)
                    return status;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
                auto object = reinterpret_cast<Stream *>(_self->self->findObject(stream));

                if (object)
                    status = object->copyBufferQueue(queueAlteredProc,
                                                     queueAlteredRefCon,
                                                     queue);

                return status;
            }

            static OSStatus StreamDeckPlay(CMIOHardwarePlugInRef self,
                                           CMIOStreamID stream)
            {
                AkPluginPrivateIntefaceLogID(stream);
                OSStatus status = kCMIOHardwareUnspecifiedError;

                if (!self)
                    return status;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
                auto object = reinterpret_cast<Stream *>(_self->self->findObject(stream));

                if (object)
                    status = object->deckPlay();

                return status;
            }

            static OSStatus StreamDeckStop(CMIOHardwarePlugInRef self,
                                           CMIOStreamID stream)
            {
                AkPluginPrivateIntefaceLogID(stream);
                OSStatus status = kCMIOHardwareUnspecifiedError;

                if (!self)
                    return status;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
                auto object = reinterpret_cast<Stream *>(_self->self->findObject(stream));

                if (object)
                    status = object->deckStop();

                return status;
            }

            static OSStatus StreamDeckJog(CMIOHardwarePlugInRef self,
                                          CMIOStreamID stream,
                                          SInt32 speed)
            {
                AkPluginPrivateIntefaceLogID(stream);
                OSStatus status = kCMIOHardwareUnspecifiedError;

                if (!self)
                    return status;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
                auto object = reinterpret_cast<Stream *>(_self->self->findObject(stream));

                if (object)
                    status = object->deckJog(speed);

                return status;
            }

            static OSStatus StreamDeckCueTo(CMIOHardwarePlugInRef self,
                                            CMIOStreamID stream,
                                            Float64 frameNumber,
                                            Boolean playOnCue)
            {
                AkPluginPrivateIntefaceLogID(stream);
                OSStatus status = kCMIOHardwareUnspecifiedError;

                if (!self)
                    return status;

                auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
                auto object = reinterpret_cast<Stream *>(_self->self->findObject(stream));

                if (object)
                    status = object->deckCueTo(frameNumber, playOnCue);

                return status;
            }
    };
}

AkVCam::PluginInterface::PluginInterface():
    ObjectInterface(),
    m_objectID(0)
{
    this->m_className = "PluginInterface";
    this->d = new PluginInterfacePrivate;
    this->d->self = this;
    this->d->pluginInterface = new CMIOHardwarePlugInInterface {
        //	Padding for COM
        NULL,

        // IUnknown Routines
        PluginInterfacePrivate::QueryInterface,
        PluginInterfacePrivate::AddRef,
        PluginInterfacePrivate::Release,

        // DAL Plug-In Routines
        PluginInterfacePrivate::Initialize,
        PluginInterfacePrivate::InitializeWithObjectID,
        PluginInterfacePrivate::Teardown,
        PluginInterfacePrivate::ObjectShow,
        PluginInterfacePrivate::ObjectHasProperty,
        PluginInterfacePrivate::ObjectIsPropertySettable,
        PluginInterfacePrivate::ObjectGetPropertyDataSize,
        PluginInterfacePrivate::ObjectGetPropertyData,
        PluginInterfacePrivate::ObjectSetPropertyData,
        PluginInterfacePrivate::DeviceSuspend,
        PluginInterfacePrivate::DeviceResume,
        PluginInterfacePrivate::DeviceStartStream,
        PluginInterfacePrivate::DeviceStopStream,
        PluginInterfacePrivate::DeviceProcessAVCCommand,
        PluginInterfacePrivate::DeviceProcessRS422Command,
        PluginInterfacePrivate::StreamCopyBufferQueue,
        PluginInterfacePrivate::StreamDeckPlay,
        PluginInterfacePrivate::StreamDeckStop,
        PluginInterfacePrivate::StreamDeckJog,
        PluginInterfacePrivate::StreamDeckCueTo
    };
    this->d->m_ref = 0;
    this->d->m_reserved = 0;

    auto homePath = std::string("/Users/") + getenv("USER");

    std::stringstream ss;
    ss << CMIO_DAEMONS_PATH << "/" << AKVCAM_ASSISTANT_NAME << ".plist";
    auto daemon = ss.str();

    if (daemon[0] == '~')
        daemon.replace(0, 1, homePath);

    struct stat fileInfo;

    if (stat(daemon.c_str(), &fileInfo) == 0)
        this->d->m_ipcBridge.registerEndPoint(true);

    this->d->m_ipcBridge.setDeviceAddedCallback(std::bind(&PluginInterface::deviceAdded,
                                                          this,
                                                          std::placeholders::_1));
    this->d->m_ipcBridge.setDeviceRemovedCallback(std::bind(&PluginInterface::deviceRemoved,
                                                            this,
                                                            std::placeholders::_1));
    this->d->m_ipcBridge.setFrameReadyCallback(std::bind(&PluginInterface::frameReady,
                                                         this,
                                                         std::placeholders::_1,
                                                         std::placeholders::_2));
    this->d->m_ipcBridge.setBroadcastingChangedCallback(std::bind(&PluginInterface::setBroadcasting,
                                                                  this,
                                                                  std::placeholders::_1,
                                                                  std::placeholders::_2));
    this->d->m_ipcBridge.setMirrorChangedCallback(std::bind(&PluginInterface::setMirror,
                                                            this,
                                                            std::placeholders::_1,
                                                            std::placeholders::_2,
                                                            std::placeholders::_3));
    this->d->m_ipcBridge.setScalingChangedCallback(std::bind(&PluginInterface::setScaling,
                                                             this,
                                                             std::placeholders::_1,
                                                             std::placeholders::_2));
    this->d->m_ipcBridge.setAspectRatioChangedCallback(std::bind(&PluginInterface::setAspectRatio,
                                                                 this,
                                                                 std::placeholders::_1,
                                                                 std::placeholders::_2));
}

AkVCam::PluginInterface::~PluginInterface()
{
    this->d->m_ipcBridge.unregisterEndPoint();
    delete this->d->pluginInterface;
    delete this->d;
}

CMIOObjectID AkVCam::PluginInterface::objectID() const
{
    return this->m_objectID;
}

CMIOHardwarePlugInRef AkVCam::PluginInterface::create()
{
    AkLoggerLog("Creating plugin interface.");

    auto pluginInterface = new PluginInterface();
    pluginInterface->d->AddRef(pluginInterface->d);

    return reinterpret_cast<CMIOHardwarePlugInRef>(pluginInterface->d);
}

AkVCam::Object *AkVCam::PluginInterface::findObject(CMIOObjectID objectID)
{
    for (auto device: this->m_devices)
        if (auto object = device->findObject(objectID))
            return object;

    return nullptr;
}

HRESULT AkVCam::PluginInterface::QueryInterface(REFIID uuid, LPVOID *interface)
{
    if (!interface)
        return E_POINTER;

    AkLoggerLog("AkVCam::PluginInterface::QueryInterface");

    if (uuidEqual(uuid, kCMIOHardwarePlugInInterfaceID)
        || uuidEqual(uuid, IUnknownUUID)) {
        AkLoggerLog("Found plugin interface.");
        this->d->AddRef(this->d);
        *interface = this->d;

        return S_OK;
    }

    return E_NOINTERFACE;
}

OSStatus AkVCam::PluginInterface::Initialize()
{
    AkLoggerLog("AkVCam::PluginInterface::Initialize");

    return this->InitializeWithObjectID(kCMIOObjectUnknown);
}

OSStatus AkVCam::PluginInterface::InitializeWithObjectID(CMIOObjectID objectID)
{
    AkLoggerLog("AkVCam::PluginInterface::InitializeWithObjectID: " << objectID);

    this->m_objectID = objectID;

#if defined(QT_DEBUG) && 1
    std::vector<VideoFormat> formats {
        {PixelFormatRGB32, 640, 480, {30.0}}
    };

    this->createDevice("org.webcamoid.cmio.AkVCam.Driver.Debug",
                       "Virtual Debug Camera (driver side)",
                       formats);
#endif

    for (auto deviceId: this->d->m_ipcBridge.listDevices())
        this->deviceAdded(deviceId);

    return kCMIOHardwareNoError;
}

OSStatus AkVCam::PluginInterface::Teardown()
{
    AkLoggerLog("AkVCam::PluginInterface::Teardown");

    return kCMIOHardwareNoError;
}

void AkVCam::PluginInterface::deviceAdded(const std::string &deviceId)
{
    AkLoggerLog("AkVCam::PluginInterface::deviceAdded");
    AkLoggerLog("Device Added: " << deviceId);

    auto description = this->d->m_ipcBridge.description(deviceId);
    auto formats = this->d->m_ipcBridge.formats(deviceId);

    this->createDevice(deviceId, description, formats);
}

void AkVCam::PluginInterface::deviceRemoved(const std::string &deviceId)
{
    AkLoggerLog("AkVCam::PluginInterface::deviceRemoved");
    AkLoggerLog("Device Removed: " << deviceId);

    this->destroyDevice(deviceId);
}

void AkVCam::PluginInterface::frameReady(const std::string &deviceId,
                                         const VideoFrame &frame)
{
    AkLoggerLog("AkVCam::PluginInterface::frameReady");

    for (auto device: this->m_devices)
        if (device->deviceId() == deviceId)
            device->frameReady(frame);
}

void AkVCam::PluginInterface::setBroadcasting(const std::string &deviceId,
                                              bool broadcasting)
{
    AkLoggerLog("AkVCam::PluginInterface::setBroadcasting");
    AkLoggerLog("Device: " << deviceId);
    AkLoggerLog("Broadcasting: " << broadcasting);

    for (auto device: this->m_devices)
        if (device->deviceId() == deviceId)
            device->setBroadcasting(broadcasting);
}

void AkVCam::PluginInterface::setMirror(const std::string &deviceId,
                                        bool horizontalMirror,
                                        bool verticalMirror)
{
    AkLoggerLog("AkVCam::PluginInterface::setMirror");

    for (auto device: this->m_devices)
        if (device->deviceId() == deviceId)
            device->setMirror(horizontalMirror, verticalMirror);
}

void AkVCam::PluginInterface::setScaling(const std::string &deviceId,
                                         VideoFrame::Scaling scaling)
{
    AkLoggerLog("AkVCam::PluginInterface::setScaling");

    for (auto device: this->m_devices)
        if (device->deviceId() == deviceId)
            device->setScaling(scaling);
}

void AkVCam::PluginInterface::setAspectRatio(const std::string &deviceId,
                                             AkVCam::VideoFrame::AspectRatio aspectRatio)
{
    AkLoggerLog("AkVCam::PluginInterface::setAspectRatio");

    for (auto device: this->m_devices)
        if (device->deviceId() == deviceId)
            device->setAspectRatio(aspectRatio);
}

bool AkVCam::PluginInterface::createDevice(const std::string &deviceId,
                                           const std::string &description,
                                           const std::vector<VideoFormat> &formats)
{
    StreamPtr stream;

    // Create one device.
    auto pluginRef = reinterpret_cast<CMIOHardwarePlugInRef>(this->d);
    auto device = std::make_shared<Device>(pluginRef);
    device->setDeviceId(deviceId);
    this->m_devices.push_back(device);

    // Define device properties.
    device->properties().setProperty(kCMIOObjectPropertyName,
                                     description.c_str());
    device->properties().setProperty(kCMIOObjectPropertyManufacturer,
                                     CMIO_PLUGIN_VENDOR);
    device->properties().setProperty(kCMIODevicePropertyModelUID,
                                     CMIO_PLUGIN_PRODUCT);
    device->properties().setProperty(kCMIODevicePropertyLinkedCoreAudioDeviceUID,
                                     "");
    device->properties().setProperty(kCMIODevicePropertyLinkedAndSyncedCoreAudioDeviceUID,
                                     "");
    device->properties().setProperty(kCMIODevicePropertySuspendedByUser,
                                     UInt32(0));
    device->properties().setProperty(kCMIODevicePropertyHogMode,
                                     pid_t(-1),
                                     false);
    device->properties().setProperty(kCMIODevicePropertyDeviceMaster,
                                     pid_t(-1));
    device->properties().setProperty(kCMIODevicePropertyExcludeNonDALAccess,
                                     UInt32(0));
    device->properties().setProperty(kCMIODevicePropertyDeviceIsAlive,
                                     UInt32(1));
    device->properties().setProperty(kCMIODevicePropertyDeviceUID,
                                     deviceId.c_str());
    device->properties().setProperty(kCMIODevicePropertyTransportType,
                                     UInt32(kIOAudioDeviceTransportTypePCI));
    device->properties().setProperty(kCMIODevicePropertyDeviceIsRunningSomewhere,
                                     UInt32(0));

    if (device->createObject() != kCMIOHardwareNoError)
        goto createDevice_failed;

    stream = device->addStream();

    // Register one stream for this device.
    if (!stream)
        goto createDevice_failed;

    stream->setFormats(formats);
    stream->properties().setProperty(kCMIOStreamPropertyDirection, UInt32(0));

    if (device->registerStreams() != kCMIOHardwareNoError) {
        device->registerStreams(false);

        goto createDevice_failed;
    }

    // Register the device.
    if (device->registerObject() != kCMIOHardwareNoError) {
        device->registerObject(false);
        device->registerStreams(false);

        goto createDevice_failed;
    }

    device->setBroadcasting(this->d->m_ipcBridge.broadcasting(deviceId));
    device->setMirror(this->d->m_ipcBridge.isHorizontalMirrored(deviceId),
                      this->d->m_ipcBridge.isVerticalMirrored(deviceId));
    device->setScaling(this->d->m_ipcBridge.scalingMode(deviceId));
    device->setAspectRatio(this->d->m_ipcBridge.aspectRatioMode(deviceId));

    return true;

createDevice_failed:
    this->m_devices.erase(std::prev(this->m_devices.end()));

    return false;
}

void AkVCam::PluginInterface::destroyDevice(const std::string &deviceId)
{
    for (auto it = this->m_devices.begin(); it != this->m_devices.end(); it++) {
        auto device = *it;

        std::string curDeviceId;
        device->properties().getProperty(kCMIODevicePropertyDeviceUID,
                                         &curDeviceId);

        if (curDeviceId == deviceId) {
            device->registerObject(false);
            device->registerStreams(false);
            this->m_devices.erase(it);

            break;
        }
    }
}
