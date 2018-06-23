/* Webcamoid, webcam capture application.
 * Copyright (C) 2018  Gonzalo Exequiel Pedone
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

#include <algorithm>
#include <fstream>
#include <cmath>
#include <memory>
#include <sstream>
#include <thread>
#include <dshow.h>

#include "ipcbridge.h"
#include "PlatformUtils/src/messageserver.h"
#include "PlatformUtils/src/mutex.h"
#include "PlatformUtils/src/sharedmemory.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/image/videoframe.h"
#include "VCamUtils/src/utils.h"

#define AkIpcBridgeLogMethod() \
    AkLoggerLog("IpcBridge::", __FUNCTION__, "()")

namespace AkVCam
{
    typedef std::shared_ptr<IMoniker> MonikerPtr;
    typedef std::shared_ptr<IBaseFilter> BaseFilterPtr;
    typedef std::shared_ptr<IPropertyBag> PropertyBagPtr;
    typedef std::shared_ptr<IPin> PinPtr;
    typedef std::shared_ptr<AM_MEDIA_TYPE> MediaTypePtr;

    struct DeviceSharedProperties
    {
        SharedMemory sharedMemory;
        Mutex mutex;
    };

    class IpcBridgePrivate
    {
        public:
            std::map<std::string, DeviceSharedProperties> devices;
            IpcBridge::FrameReadyCallback frameReadyCallback;
            IpcBridge::DeviceChangedCallback deviceAddedCallback;
            IpcBridge::DeviceChangedCallback deviceRemovedCallback;
            IpcBridge::BroadcastingChangedCallback broadcastingChangedCallback;
            IpcBridge::MirrorChangedCallback mirrorChangedCallback;
            IpcBridge::ScalingChangedCallback scalingChangedCallback;
            IpcBridge::AspectRatioChangedCallback aspectRatioChangedCallback;
            IpcBridge::SwapRgbChangedCallback swapRgbChangedCallback;
            IpcBridge::ListenerCallback listenerAddedCallback;
            IpcBridge::ListenerCallback listenerRemovedCallback;
            std::map<uint32_t, MessageHandler> messageHandlers;
            std::vector<std::string> broadcasting;
            std::map<std::string, std::string> options;
            std::vector<std::string> driverPaths;
            MessageServer messageServer;
            SharedMemory sharedMemory;
            Mutex globalMutex;
            std::string portName;

            IpcBridgePrivate();
            std::vector<MonikerPtr> listCameras() const;
            static void deleteUnknown(IUnknown *unknown);
            BaseFilterPtr filter(IMoniker *moniker) const;
            PropertyBagPtr propertyBag(IMoniker *moniker) const;
            bool isVirtualCamera(const MonikerPtr &moniker) const;
            bool isVirtualCamera(IBaseFilter *baseFilter) const;
            std::string cameraPath(const MonikerPtr &moniker) const;
            std::string cameraPath(IPropertyBag *propertyBag) const;
            std::string cameraDescription(const MonikerPtr &moniker) const;
            std::string cameraDescription(IPropertyBag *propertyBag) const;
            std::vector<PinPtr> enumPins(IBaseFilter *baseFilter) const;
            std::vector<VideoFormat> enumVideoFormats(IPin *pin) const;
            std::vector<std::wstring> findFiles(const std::wstring &path) const;
            std::vector<std::string> findFiles(const std::string &path,
                                               const std::string &fileName) const;
            std::vector<std::wstring> findFiles(const std::wstring &path,
                                                const std::wstring &fileName) const;
            std::string regAddLine(const std::string &key,
                                   const std::string &value,
                                   const std::string &data) const;
            std::string regAddLine(const std::string &key,
                                   const std::string &value,
                                   int data) const;
            std::string regDeleteLine(const std::string &key) const;
            std::string regDeleteLine(const std::string &key,
                                      const std::string &value) const;
            std::string regMoveLine(const std::string &fromKey,
                                    const std::string &toKey) const;
            std::string dirname(const std::string &path) const;
            void updateDeviceSharedProperties();
            void updateDeviceSharedProperties(const std::string &deviceId,
                                              const std::string &owner);
            std::string locateDriverPath() const;

            // Message handling methods
            void isAlive(Message *message);
            void frameReady(Message *message);
            void setBroadcasting(Message *message);
            void setMirror(Message *message);
            void setScaling(Message *message);
            void setAspectRatio(Message *message);
            void setSwapRgb(Message *message);
            void listenerAdd(Message *message);
            void listenerRemove(Message *message);

            // Execute commands with elevated privileges.
            int sudo(const std::vector<std::string> &parameters,
                     const std::wstring &directory={},
                     bool show=false);
    };

    static const int maxFrameWidth = 1920;
    static const int maxFrameHeight = 1080;
    static const size_t maxFrameSize = maxFrameWidth * maxFrameHeight;

    static const size_t maxBufferSize = sizeof(Frame) + 3 * maxFrameSize;
}

AkVCam::IpcBridge::IpcBridge()
{
    AkIpcBridgeLogMethod();
    this->d = new IpcBridgePrivate;
}

AkVCam::IpcBridge::~IpcBridge()
{
    this->unregisterPeer();
    delete this->d;
}

void AkVCam::IpcBridge::setOption(const std::string &key, const std::string &value)
{
    AkIpcBridgeLogMethod();

    if (value.empty())
        this->d->options.erase(key);
    else
        this->d->options[key] = value;
}

std::vector<std::string> AkVCam::IpcBridge::driverPaths() const
{
    AkIpcBridgeLogMethod();

    return this->d->driverPaths;
}

void AkVCam::IpcBridge::setDriverPaths(const std::vector<std::string> &driverPaths)
{
    AkIpcBridgeLogMethod();
    this->d->driverPaths = driverPaths;
}

std::vector<std::string> AkVCam::IpcBridge::availableRootMethods() const
{
    return {"runas"};
}

std::string AkVCam::IpcBridge::rootMethod() const
{
    return {"runas"};
}

bool AkVCam::IpcBridge::setRootMethod(const std::string &rootMethod)
{
    return rootMethod == "runas";
}

bool AkVCam::IpcBridge::registerPeer(bool asClient)
{
    AkIpcBridgeLogMethod();

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_REQUEST_PORT;
    message.dataSize = sizeof(MsgRequestPort);
    auto requestData = messageData<MsgRequestPort>(&message);
    requestData->client = asClient;

    if (!MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                                    &message))
        return false;

    std::string portName(requestData->port);
    std::string pipeName = "\\\\.\\pipe\\" + portName;
    this->d->messageServer.setPipeName(std::wstring(pipeName.begin(),
                                                    pipeName.end()));
    this->d->messageServer.setHandlers(this->d->messageHandlers);
    AkLoggerLog("Recommended port name: ", portName);

    if (!this->d->messageServer.start()) {
        AkLoggerLog("Can't start message server");

        return false;
    }

    message.clear();
    message.messageId = AKVCAM_ASSISTANT_MSG_ADD_PORT;
    message.dataSize = sizeof(MsgAddPort);
    auto addData = messageData<MsgAddPort>(&message);
    memcpy(addData->port,
           portName.c_str(),
           (std::min<size_t>)(portName.size(), MAX_STRING));
    memcpy(addData->pipeName,
           pipeName.c_str(),
           (std::min<size_t>)(pipeName.size(), MAX_STRING));

    AkLoggerLog("Registering port name: ", portName);

    if (!MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                                    &message)) {
        this->d->messageServer.stop(true);

        return false;
    }

    if (!addData->status) {
        this->d->messageServer.stop(true);

        return false;
    }

    this->d->sharedMemory.setName(L"Local\\"
                                  + std::wstring(portName.begin(), portName.end())
                                  + L".data");
    this->d->globalMutex = Mutex(std::wstring(portName.begin(), portName.end())
                                  + L".mutex");
    this->d->portName = portName;
    AkLoggerLog("Peer registered as ", portName);

    return true;
}

void AkVCam::IpcBridge::unregisterPeer()
{
    AkIpcBridgeLogMethod();

    if (this->d->portName.empty())
        return;

    this->d->sharedMemory.setName({});
    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_REMOVE_PORT;
    message.dataSize = sizeof(MsgRemovePort);
    auto data = messageData<MsgRemovePort>(&message);
    memcpy(data->port,
           this->d->portName.c_str(),
           (std::min<size_t>)(this->d->portName.size(), MAX_STRING));
    MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                               &message);
    this->d->messageServer.stop(true);
    this->d->portName.clear();
}

std::vector<std::string> AkVCam::IpcBridge::listDevices() const
{
    AkIpcBridgeLogMethod();
    std::vector<std::string> devices;

    for (auto camera: this->d->listCameras())
        if (this->d->isVirtualCamera(camera))
            devices.push_back(this->d->cameraPath(camera));

    return devices;
}

std::string AkVCam::IpcBridge::description(const std::string &deviceId) const
{
    AkIpcBridgeLogMethod();

    for (auto camera: this->d->listCameras()) {
        auto propertyBag = this->d->propertyBag(camera.get());

        if (this->d->isVirtualCamera(camera)
            && this->d->cameraPath(propertyBag.get()) == deviceId)
            return this->d->cameraDescription(propertyBag.get());
    }

    return std::string();
}

std::vector<AkVCam::PixelFormat> AkVCam::IpcBridge::supportedOutputPixelFormats() const
{
    return {
        PixelFormatRGB32,
        PixelFormatRGB24,
        PixelFormatRGB16,
        PixelFormatRGB15,
        PixelFormatUYVY,
        PixelFormatYUY2,
        PixelFormatNV12
    };
}

std::vector<AkVCam::VideoFormat> AkVCam::IpcBridge::formats(const std::string &deviceId) const
{
    AkIpcBridgeLogMethod();

    std::vector<AkVCam::VideoFormat> formats;

    for (auto camera: this->d->listCameras()) {
        auto baseFilter = this->d->filter(camera.get());

        if (this->d->isVirtualCamera(baseFilter.get())
            && this->d->cameraPath(camera) == deviceId) {
            auto pins = this->d->enumPins(baseFilter.get());

            if (!pins.empty())
                formats = this->d->enumVideoFormats(pins[0].get());

            break;
        }
    }

    return formats;
}

std::string AkVCam::IpcBridge::broadcaster(const std::string &deviceId) const
{
    AkIpcBridgeLogMethod();

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING;
    message.dataSize = sizeof(MsgBroadcasting);
    auto data = messageData<MsgBroadcasting>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));

    if (!MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                                    &message))
        return {};

    if (!data->status)
        return {};

    std::string broadcaster(data->broadcaster);

    AkLoggerLog("Device: ", deviceId);
    AkLoggerLog("Broadcaster: ", broadcaster);

    return broadcaster;
}

bool AkVCam::IpcBridge::isHorizontalMirrored(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_MIRRORING;
    message.dataSize = sizeof(MsgMirroring);
    auto data = messageData<MsgMirroring>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));

    if (!MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                                    &message))
        return false;

    if (!data->status)
        return false;

    return data->hmirror;
}

bool AkVCam::IpcBridge::isVerticalMirrored(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_MIRRORING;
    message.dataSize = sizeof(MsgMirroring);
    auto data = messageData<MsgMirroring>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));

    if (!MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                                    &message))
        return false;

    if (!data->status)
        return false;

    return data->vmirror;
}

AkVCam::Scaling AkVCam::IpcBridge::scalingMode(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_SCALING;
    message.dataSize = sizeof(MsgScaling);
    auto data = messageData<MsgScaling>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));

    if (!MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                                    &message))
        return ScalingFast;

    if (!data->status)
        return ScalingFast;

    return data->scaling;
}

AkVCam::AspectRatio AkVCam::IpcBridge::aspectRatioMode(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_ASPECTRATIO;
    message.dataSize = sizeof(MsgAspectRatio);
    auto data = messageData<MsgAspectRatio>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));

    if (!MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                                    &message))
        return AspectRatioIgnore;

    if (!data->status)
        return AspectRatioIgnore;

    return data->aspect;
}

bool AkVCam::IpcBridge::swapRgb(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_SWAPRGB;
    message.dataSize = sizeof(MsgSwapRgb);
    auto data = messageData<MsgSwapRgb>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));

    if (!MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                                    &message))
        return false;

    if (!data->status)
        return false;

    return data->swap;
}

std::vector<std::string> AkVCam::IpcBridge::listeners(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_LISTENERS;
    message.dataSize = sizeof(MsgListeners);
    auto data = messageData<MsgListeners>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));

    if (!MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                                    &message))
        return {};

    if (!data->status)
        return {};

    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER;
    std::vector<std::string> listeners;

    for (size_t i = 0; i < data->nlistener; i++) {
        data->nlistener = i;

        if (!MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                                        &message))
            continue;

        if (!data->status)
            continue;

        listeners.push_back(std::string(data->listener));
    }

    return listeners;
}

std::string AkVCam::IpcBridge::deviceCreate(const std::string &description,
                                            const std::vector<VideoFormat> &formats)
{
    AkIpcBridgeLogMethod();

    auto driverPath = this->d->locateDriverPath();

    if (driverPath.empty())
        return {};

    // Create a device path for the new device and add it's entry.
    auto devicePath = createDevicePath();

    if (devicePath.empty())
        return {};

    std::stringstream ss;
    ss << "@echo off" << std::endl;

    auto driverInstallPath = programFilesPath() + "\\" DSHOW_PLUGIN_NAME ".plugin";

    // Copy all plugins
    std::vector<std::string> installPaths;

    for (auto path: this->d->findFiles(std::wstring(driverPath.begin(),
                                                    driverPath.end()),
                                       DSHOW_PLUGIN_NAME_L L".dll")) {
        std::string path_(path.begin(), path.end());
        auto installPath = replace(path_, driverPath, driverInstallPath);

        if (!isEqualFile(path_, installPath))
            ss << "mkdir \""
               << this->d->dirname(installPath)
               << "\""
               << std::endl
               << "copy /y \""
               << path_
               << "\" \""
               << installPath
               << "\""
               << std::endl;

        installPaths.push_back(installPath);
    }

    // Copy all services
    std::vector<std::string> assistantInstallPaths;

    for (auto path: this->d->findFiles(std::wstring(driverPath.begin(),
                                                    driverPath.end()),
                                       DSHOW_PLUGIN_ASSISTANT_NAME_L L".exe")) {
        std::string path_(path.begin(), path.end());
        auto installPath = replace(path_, driverPath, driverInstallPath);

        if (!isEqualFile(path_, installPath))
            ss << "mkdir \""
               << this->d->dirname(installPath)
               << "\""
               << std::endl
               << "copy /y \""
               << path_
               << "\" \""
               << installPath
               << "\""
               << std::endl;

        std::string arch = isWow64()? "x64": DSHOW_PLUGIN_ARCH;

        if (installPath.find(arch + "\\" DSHOW_PLUGIN_ASSISTANT_NAME ".exe") != std::string::npos)
            assistantInstallPaths.push_back(installPath);
    }

    // List cameras and create a line with the number of cameras.
    auto nCameras = camerasCount();

    ss << this->d->regAddLine("HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras",
                              "size",
                              int(nCameras + 1))
       << std::endl;

    ss << this->d->regAddLine("HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                              + std::to_string(nCameras + 1),
                              "path",
                              std::string(devicePath.begin(), devicePath.end()))
       << std::endl;

    // Add description line.
    ss << this->d->regAddLine("HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                              + std::to_string(nCameras + 1),
                              "description",
                              description)
       << std::endl;

    // Set number of formats.
    ss << this->d->regAddLine("HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                              + std::to_string(nCameras + 1)
                              + "\\Formats",
                              "size",
                              int(formats.size()))
       << std::endl;

    // Setup formats.
    for (size_t i = 0; i < formats.size(); i++) {
        auto videoFormat = formats[i];
        auto format = VideoFormat::stringFromFourcc(videoFormat.fourcc());

        ss << this->d->regAddLine("HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                                  + std::to_string(nCameras + 1)
                                  + "\\Formats\\"
                                  + std::to_string(i + 1),
                                  "format",
                                  format)
           << std::endl;

        ss << this->d->regAddLine("HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                                  + std::to_string(nCameras + 1)
                                  + "\\Formats\\"
                                  + std::to_string(i + 1),
                                  "width",
                                  videoFormat.width())
           << std::endl;

        ss << this->d->regAddLine("HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                                  + std::to_string(nCameras + 1)
                                  + "\\Formats\\"
                                  + std::to_string(i + 1),
                                  "height",
                                  videoFormat.height())
           << std::endl;

        ss << this->d->regAddLine("HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                                  + std::to_string(nCameras + 1)
                                  + "\\Formats\\"
                                  + std::to_string(i + 1),
                                  "fps",
                                  lround(videoFormat.minimumFrameRate()))
           << std::endl;
    }

    for (auto path: installPaths)
        ss << "regsvr32 /s \"" << path << "\"" << std::endl;

    for (auto path: assistantInstallPaths)
        ss << "\"" << path << "\" --install" << std::endl;

    // Create the script.
    auto scriptPath = tempPath() + "\\device_create_" + timeStamp() + ".bat";
    std::fstream script;
    script.open(scriptPath, std::ios_base::out | std::ios_base::trunc);

    if (script.is_open()) {
        script << ss.str();
        script.close();

        // Execute the script with elevated privileges.
        if (this->d->sudo({"cmd", "/c", scriptPath}))
            devicePath.clear();

        std::wstring wScriptPath(scriptPath.begin(), scriptPath.end());
        DeleteFile(wScriptPath.c_str());
    } else {
        devicePath.clear();
    }

    return std::string(devicePath.begin(), devicePath.end());
}

void AkVCam::IpcBridge::deviceDestroy(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    auto camera = cameraFromId(std::wstring(deviceId.begin(), deviceId.end()));

    if (camera < 0)
        return;

    auto driverPath = this->d->locateDriverPath();

    if (driverPath.empty())
        return;

    std::stringstream ss;
    ss << "@echo off" << std::endl;

    auto driverInstallPath = programFilesPath() + "\\" DSHOW_PLUGIN_NAME ".plugin";
    std::vector<std::string> installPaths;

    for (auto path: this->d->findFiles(std::wstring(driverPath.begin(),
                                                    driverPath.end()),
                                       DSHOW_PLUGIN_NAME_L L".dll")) {
        std::string path_(path.begin(), path.end());
        auto installPath = replace(path_, driverPath, driverInstallPath);

        installPaths.push_back(installPath);
    }

    std::vector<std::string> assistantInstallPaths;

    for (auto path: this->d->findFiles(std::wstring(driverPath.begin(),
                                                    driverPath.end()),
                                       DSHOW_PLUGIN_ASSISTANT_NAME_L L".exe")) {
        std::string path_(path.begin(), path.end());
        auto installPath = replace(path_, driverPath, driverInstallPath);

        assistantInstallPaths.push_back(installPath);
    }

    // List cameras and create a line with the number of cameras.
    auto nCameras = camerasCount();

    if (nCameras > 1) {
        ss << this->d->regAddLine("HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras",
                                  "size",
                                  int(nCameras - 1))
           << std::endl;

        ss << this->d->regDeleteLine("HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                                     + std::to_string(camera + 1))
           << std::endl;

        for (DWORD i = DWORD(camera + 1); i < nCameras; i++) {
            ss << this->d->regMoveLine("HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                                       + std::to_string(i + 1),
                                       "HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                                                                          + std::to_string(i))
               << std::endl;
        }

        for (auto path: installPaths)
            ss << "regsvr32 /s \"" << path << "\"" << std::endl;
    } else {
        for (auto path: installPaths)
            ss << "regsvr32 /s /u \"" << path << "\"" << std::endl;

        for (auto path: assistantInstallPaths)
            ss << "\"" << path << "\" --uninstall" << std::endl;

        ss << this->d->regDeleteLine("HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras")
           << std::endl;

        std::wstring wDriverPath(driverPath.begin(), driverPath.end());
        std::wstring wDriverInstallPath(driverInstallPath.begin(),
                                        driverInstallPath.end());

        if (lstrcmpi(wDriverPath.c_str(), wDriverInstallPath.c_str()))
            ss << "rmdir /s /q \"" << driverInstallPath << "\"";
    }

    auto scriptPath = tempPath() + "\\device_destroy_" + timeStamp() + ".bat";
    std::fstream script;
    script.open(scriptPath, std::ios_base::out | std::ios_base::trunc);

    if (script.is_open()) {
        script << ss.str();
        script.close();
        this->d->sudo({"cmd", "/c", scriptPath});
        std::wstring wScriptPath(scriptPath.begin(), scriptPath.end());
        DeleteFile(wScriptPath.c_str());
    }
}

bool AkVCam::IpcBridge::changeDescription(const std::string &deviceId,
                                          const std::string &description)
{
    AkIpcBridgeLogMethod();

    auto camera = cameraFromId(std::wstring(deviceId.begin(), deviceId.end()));

    if (camera < 0)
        return false;

    auto driverPath = this->d->locateDriverPath();

    if (driverPath.empty())
        return false;

    std::stringstream ss;
    ss << "@echo off" << std::endl;

    auto driverInstallPath = programFilesPath() + "\\" DSHOW_PLUGIN_NAME ".plugin";
    std::vector<std::string> installPaths;

    for (auto path: this->d->findFiles(std::wstring(driverPath.begin(),
                                                    driverPath.end()),
                                       DSHOW_PLUGIN_NAME_L L".dll")) {
        std::string path_(path.begin(), path.end());
        auto installPath = replace(path_, driverPath, driverInstallPath);
        installPaths.push_back(installPath);
    }

    ss << this->d->regAddLine("HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                              + std::to_string(camera + 1),
                              "description",
                              description)
       << std::endl;

    for (auto path: installPaths)
        ss << "regsvr32 /s \"" << path << "\"" << std::endl;

    auto scriptPath = tempPath()
                    + "\\device_change_description_" + timeStamp() + ".bat";
    std::fstream script;
    script.open(scriptPath, std::ios_base::out | std::ios_base::trunc);
    bool ok = false;

    if (script.is_open()) {
        script << ss.str();
        script.close();
        ok = this->d->sudo({"cmd", "/c", scriptPath}) == 0;
        std::wstring wScriptPath(scriptPath.begin(), scriptPath.end());
        DeleteFile(wScriptPath.c_str());
    }

    return ok;
}

bool AkVCam::IpcBridge::destroyAllDevices()
{
    AkIpcBridgeLogMethod();

    auto driverPath = this->d->locateDriverPath();

    if (driverPath.empty())
        return false;

    std::stringstream ss;
    ss << "@echo off" << std::endl;

    auto driverInstallPath = programFilesPath() + "\\" DSHOW_PLUGIN_NAME ".plugin";

    for (auto path: this->d->findFiles(std::wstring(driverPath.begin(),
                                                    driverPath.end()),
                                       DSHOW_PLUGIN_NAME_L L".dll")) {
        std::string path_(path.begin(), path.end());
        auto installPath = replace(path_, driverPath, driverInstallPath);
        ss << "regsvr32 /s /u \"" << installPath << "\"" << std::endl;
    }

    for (auto path: this->d->findFiles(std::wstring(driverPath.begin(),
                                                    driverPath.end()),
                                       DSHOW_PLUGIN_ASSISTANT_NAME_L L".exe")) {
        std::string path_(path.begin(), path.end());
        auto installPath = replace(path_, driverPath, driverInstallPath);
        ss << "\"" << installPath << "\" --uninstall" << std::endl;
    }

    ss << this->d->regDeleteLine("HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras")
       << std::endl;

    std::wstring wDriverPath(driverPath.begin(), driverPath.end());
    std::wstring wDriverInstallPath(driverInstallPath.begin(),
                                    driverInstallPath.end());

    if (lstrcmpi(wDriverPath.c_str(), wDriverInstallPath.c_str()))
        ss << "rmdir /s /q \"" << driverInstallPath << "\"";

    auto scriptPath = tempPath()
                    + "\\device_remove_all_" + timeStamp() + ".bat";
    std::fstream script;
    script.open(scriptPath, std::ios_base::out | std::ios_base::trunc);
    bool ok = false;

    if (script.is_open()) {
        script << ss.str();
        script.close();
        ok = this->d->sudo({"cmd", "/c", scriptPath}) == 0;
        std::wstring wScriptPath(scriptPath.begin(), scriptPath.end());
        DeleteFile(wScriptPath.c_str());
    }

    return ok;
}

bool AkVCam::IpcBridge::deviceStart(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    auto it = std::find(this->d->broadcasting.begin(),
                        this->d->broadcasting.end(),
                        deviceId);

    if (it != this->d->broadcasting.end())
        return false;

    std::wstring portName(this->d->portName.begin(),
                          this->d->portName.end());
    this->d->sharedMemory.setName(L"Local\\" + portName + L".data");
    this->d->globalMutex = Mutex(portName + L".mutex");

    if (!this->d->sharedMemory.open(maxBufferSize, SharedMemory::OpenModeWrite))
        return false;

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING;
    message.dataSize = sizeof(MsgBroadcasting);
    auto data = messageData<MsgBroadcasting>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));
    memcpy(data->broadcaster,
           this->d->portName.c_str(),
           (std::min<size_t>)(this->d->portName.size(), MAX_STRING));

    if (!MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                                    &message)) {
        this->d->sharedMemory.close();

        return false;
    }

    if (!data->status)
        return false;

    this->d->broadcasting.push_back(deviceId);

    return true;
}

void AkVCam::IpcBridge::deviceStop(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    auto it = std::find(this->d->broadcasting.begin(),
                        this->d->broadcasting.end(),
                        deviceId);

    if (it == this->d->broadcasting.end())
        return;

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING;
    message.dataSize = sizeof(MsgBroadcasting);
    auto data = messageData<MsgBroadcasting>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));

    MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                               &message);
    this->d->sharedMemory.close();
    this->d->broadcasting.erase(it);

    return;
}

bool AkVCam::IpcBridge::write(const std::string &deviceId,
                              const VideoFrame &frame)
{
    AkIpcBridgeLogMethod();

    if (!frame.format())
        return false;

    auto buffer =
            reinterpret_cast<Frame *>(this->d->sharedMemory.lock(&this->d->globalMutex));

    if (!buffer)
        return false;

    if (size_t(frame.format().width() * frame.format().height()) > maxFrameSize) {
        auto scaledFrame = frame.scaled(maxFrameSize);
        buffer->format = scaledFrame.format().fourcc();
        buffer->width = scaledFrame.format().width();
        buffer->height = scaledFrame.format().height();
        buffer->size = uint32_t(frame.dataSize());
        memcpy(buffer->data, scaledFrame.data().get(), scaledFrame.dataSize());
    } else {
        buffer->format = frame.format().fourcc();
        buffer->width = frame.format().width();
        buffer->height = frame.format().height();
        buffer->size = uint32_t(frame.dataSize());
        memcpy(buffer->data, frame.data().get(), frame.dataSize());
    }

    this->d->sharedMemory.unlock(&this->d->globalMutex);

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_FRAME_READY;
    message.dataSize = sizeof(MsgFrameReady);
    auto data = messageData<MsgFrameReady>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));
    memcpy(data->port,
           this->d->portName.c_str(),
           (std::min<size_t>)(this->d->portName.size(), MAX_STRING));

    return MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                                      &message);
}

void AkVCam::IpcBridge::setMirroring(const std::string &deviceId,
                                     bool horizontalMirrored,
                                     bool verticalMirrored)
{
    AkIpcBridgeLogMethod();

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_SETMIRRORING;
    message.dataSize = sizeof(MsgMirroring);
    auto data = messageData<MsgMirroring>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));
    data->hmirror = horizontalMirrored;
    data->vmirror = verticalMirrored;

    MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                               &message);
}

void AkVCam::IpcBridge::setScaling(const std::string &deviceId,
                                   Scaling scaling)
{
    AkIpcBridgeLogMethod();

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_SETSCALING;
    message.dataSize = sizeof(MsgScaling);
    auto data = messageData<MsgScaling>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));
    data->scaling = scaling;

    MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                               &message);
}

void AkVCam::IpcBridge::setAspectRatio(const std::string &deviceId,
                                       AspectRatio aspectRatio)
{
    AkIpcBridgeLogMethod();

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_SETASPECTRATIO;
    message.dataSize = sizeof(MsgAspectRatio);
    auto data = messageData<MsgAspectRatio>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));
    data->aspect = aspectRatio;

    MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                               &message);
}

void AkVCam::IpcBridge::setSwapRgb(const std::string &deviceId, bool swap)
{
    AkIpcBridgeLogMethod();

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_SETSWAPRGB;
    message.dataSize = sizeof(MsgSwapRgb);
    auto data = messageData<MsgSwapRgb>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));
    data->swap = swap;

    MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                               &message);
}

void AkVCam::IpcBridge::setListenerAddedCallback(ListenerCallback callback)
{
    this->d->listenerAddedCallback = callback;
}

void AkVCam::IpcBridge::setListenerRemovedCallback(ListenerCallback callback)
{
    this->d->listenerRemovedCallback = callback;
}

bool AkVCam::IpcBridge::addListener(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_ADD;
    message.dataSize = sizeof(MsgListeners);
    auto data = messageData<MsgListeners>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));
    memcpy(data->listener,
           this->d->portName.c_str(),
           (std::min<size_t>)(this->d->portName.size(), MAX_STRING));

    if (!MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                                    &message))
        return false;

    return data->status;
}

bool AkVCam::IpcBridge::removeListener(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_REMOVE;
    message.dataSize = sizeof(MsgListeners);
    auto data = messageData<MsgListeners>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));
    memcpy(data->listener,
           this->d->portName.c_str(),
           (std::min<size_t>)(this->d->portName.size(), MAX_STRING));

    if (!MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                                    &message))
        return false;

    return data->status;
}

void AkVCam::IpcBridge::setFrameReadyCallback(FrameReadyCallback callback)
{
    this->d->frameReadyCallback = callback;
}

void AkVCam::IpcBridge::setDeviceAddedCallback(DeviceChangedCallback callback)
{
    this->d->deviceAddedCallback = callback;
}

void AkVCam::IpcBridge::setDeviceRemovedCallback(DeviceChangedCallback callback)
{
    this->d->deviceRemovedCallback = callback;
}

void AkVCam::IpcBridge::setBroadcastingChangedCallback(BroadcastingChangedCallback callback)
{
    this->d->broadcastingChangedCallback = callback;
}

void AkVCam::IpcBridge::setMirrorChangedCallback(MirrorChangedCallback callback)
{
    this->d->mirrorChangedCallback = callback;
}

void AkVCam::IpcBridge::setScalingChangedCallback(ScalingChangedCallback callback)
{
    this->d->scalingChangedCallback = callback;
}

void AkVCam::IpcBridge::setAspectRatioChangedCallback(AspectRatioChangedCallback callback)
{
    this->d->aspectRatioChangedCallback = callback;
}

void AkVCam::IpcBridge::setSwapRgbChangedCallback(SwapRgbChangedCallback callback)
{
    this->d->swapRgbChangedCallback = callback;
}

AkVCam::IpcBridgePrivate::IpcBridgePrivate()
{
    this->updateDeviceSharedProperties();

    this->messageHandlers = std::map<uint32_t, MessageHandler> {
        {AKVCAM_ASSISTANT_MSG_ISALIVE               , AKVCAM_BIND_FUNC(IpcBridgePrivate::isAlive)        },
        {AKVCAM_ASSISTANT_MSG_FRAME_READY           , AKVCAM_BIND_FUNC(IpcBridgePrivate::frameReady)     },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING, AKVCAM_BIND_FUNC(IpcBridgePrivate::setBroadcasting)},
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETMIRRORING   , AKVCAM_BIND_FUNC(IpcBridgePrivate::setMirror)      },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETSCALING     , AKVCAM_BIND_FUNC(IpcBridgePrivate::setScaling)     },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETASPECTRATIO , AKVCAM_BIND_FUNC(IpcBridgePrivate::setAspectRatio) },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETSWAPRGB     , AKVCAM_BIND_FUNC(IpcBridgePrivate::setSwapRgb)     },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_ADD   , AKVCAM_BIND_FUNC(IpcBridgePrivate::listenerAdd)    },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_REMOVE, AKVCAM_BIND_FUNC(IpcBridgePrivate::listenerRemove) },
    };
}

std::vector<AkVCam::MonikerPtr> AkVCam::IpcBridgePrivate::listCameras() const
{
    std::vector<MonikerPtr> cameras;

    // Create the System Device Enumerator.
    ICreateDevEnum *deviceEnumerator = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum,
                                  nullptr,
                                  CLSCTX_INPROC_SERVER,
                                  IID_ICreateDevEnum,
                                  reinterpret_cast<void **>(&deviceEnumerator));

    if (FAILED(hr))
        return cameras;

    // Create an enumerator for the category.
    IEnumMoniker *enumMoniker = nullptr;

    if (deviceEnumerator->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
                                                &enumMoniker,
                                                0) == S_OK) {
        enumMoniker->Reset();
        IMoniker *moniker = nullptr;

        while (enumMoniker->Next(1, &moniker, nullptr) == S_OK)
            cameras.push_back(MonikerPtr(moniker,
                                         IpcBridgePrivate::deleteUnknown));

        enumMoniker->Release();
    }

    deviceEnumerator->Release();

    return cameras;
}

void AkVCam::IpcBridgePrivate::deleteUnknown(IUnknown *unknown)
{
    if (unknown)
        unknown->Release();
}

AkVCam::BaseFilterPtr AkVCam::IpcBridgePrivate::filter(IMoniker *moniker) const
{
    if (!moniker)
        return BaseFilterPtr();

    IBaseFilter *baseFilter = nullptr;

    if (FAILED(moniker->BindToObject(nullptr,
                                     nullptr,
                                     IID_IBaseFilter,
                                     reinterpret_cast<void **>(&baseFilter)))) {
        return BaseFilterPtr();
    }

    return BaseFilterPtr(baseFilter, IpcBridgePrivate::deleteUnknown);
}

AkVCam::PropertyBagPtr AkVCam::IpcBridgePrivate::propertyBag(IMoniker *moniker) const
{
    if (!moniker)
        return PropertyBagPtr();

    IPropertyBag *propertyBag = nullptr;

    if (FAILED(moniker->BindToStorage(nullptr,
                                      nullptr,
                                      IID_IPropertyBag,
                                      reinterpret_cast<void **>(&propertyBag)))) {
        return PropertyBagPtr();
    }

    return PropertyBagPtr(propertyBag, IpcBridgePrivate::deleteUnknown);
}

bool AkVCam::IpcBridgePrivate::isVirtualCamera(const MonikerPtr &moniker) const
{
    auto baseFilter = this->filter(moniker.get());

    if (!baseFilter)
        return false;

    return this->isVirtualCamera(baseFilter.get());
}

bool AkVCam::IpcBridgePrivate::isVirtualCamera(IBaseFilter *baseFilter) const
{
    if (!baseFilter)
        return false;

    CLSID clsid;
    memset(&clsid, 0, sizeof(CLSID));
    baseFilter->GetClassID(&clsid);

    return cameraFromId(clsid) >= 0;
}

std::string AkVCam::IpcBridgePrivate::cameraPath(const MonikerPtr &moniker) const
{
    auto propertyBag = this->propertyBag(moniker.get());

    return this->cameraPath(propertyBag.get());
}

std::string AkVCam::IpcBridgePrivate::cameraPath(IPropertyBag *propertyBag) const
{
    VARIANT var;
    VariantInit(&var);

    if (FAILED(propertyBag->Read(L"DevicePath", &var, nullptr)))
        return std::string();

    std::wstring wstr(var.bstrVal);
    std::string devicePath = std::string(wstr.begin(), wstr.end());
    VariantClear(&var);

    return devicePath;
}

std::string AkVCam::IpcBridgePrivate::cameraDescription(const MonikerPtr &moniker) const
{
    auto propertyBag = this->propertyBag(moniker.get());

    return this->cameraDescription(propertyBag.get());
}

std::string AkVCam::IpcBridgePrivate::cameraDescription(IPropertyBag *propertyBag) const
{
    VARIANT var;
    VariantInit(&var);

    if (FAILED(propertyBag->Read(L"Description", &var, nullptr)))
        if (FAILED(propertyBag->Read(L"FriendlyName", &var, nullptr)))
            return std::string();

    std::wstring wstr(var.bstrVal);
    std::string devicePath = std::string(wstr.begin(), wstr.end());
    VariantClear(&var);

    return devicePath;
}

std::vector<AkVCam::PinPtr> AkVCam::IpcBridgePrivate::enumPins(IBaseFilter *baseFilter) const
{
    std::vector<PinPtr> pins;
    IEnumPins *enumPins = nullptr;

    if (SUCCEEDED(baseFilter->EnumPins(&enumPins))) {
        enumPins->Reset();
        IPin *pin = nullptr;

        while (enumPins->Next(1, &pin, nullptr) == S_OK) {
            PIN_DIRECTION direction = PINDIR_INPUT;

            if (SUCCEEDED(pin->QueryDirection(&direction))
                && direction == PINDIR_OUTPUT) {
                pins.push_back(PinPtr(pin, IpcBridgePrivate::deleteUnknown));

                continue;
            }

            pin->Release();
        }

        enumPins->Release();
    }

    return pins;
}

std::vector<AkVCam::VideoFormat> AkVCam::IpcBridgePrivate::enumVideoFormats(IPin *pin) const
{
    std::vector<AkVCam::VideoFormat> mediaTypes;
    IEnumMediaTypes *pEnum = nullptr;

    if (FAILED(pin->EnumMediaTypes(&pEnum)))
        return mediaTypes;

    pEnum->Reset();
    AM_MEDIA_TYPE *mediaType = nullptr;

    while (pEnum->Next(1, &mediaType, nullptr) == S_OK) {
        auto format = formatFromMediaType(mediaType);
        deleteMediaType(&mediaType);

        if (format)
            mediaTypes.push_back(format);
    }

    pEnum->Release();

    return mediaTypes;
}

std::vector<std::wstring> AkVCam::IpcBridgePrivate::findFiles(const std::wstring &path) const
{
    std::wstring path_ = path;

    auto attributes = GetFileAttributes(path.c_str());

    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
        path_ += L"\\*";

    WIN32_FIND_DATA data;
    memset(&data, 0, sizeof(WIN32_FIND_DATA));
    auto find = FindFirstFile(path_.c_str(), &data);

    if (find == INVALID_HANDLE_VALUE)
        return {};

    std::vector<std::wstring> paths;

    do {
        std::wstring fileName(data.cFileName);

        if (fileName == L"." || fileName == L"..")
            continue;

        std::wstring filePath = path + L"\\" + fileName;

        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            for (auto path: this->findFiles(filePath))
                paths.push_back(path);
        else
            paths.push_back(filePath);
    } while (FindNextFile(find, &data));

    FindClose(find);

    return paths;
}

std::vector<std::string> AkVCam::IpcBridgePrivate::findFiles(const std::string &path,
                                                             const std::string &fileName) const
{
    auto wfiles = this->findFiles(std::wstring(path.begin(), path.end()),
                                  std::wstring(fileName.begin(), fileName.end()));

    std::vector<std::string> files;

    for (auto &file: wfiles)
        files.push_back(std::string(file.begin(), file.end()));

    return files;
}

std::vector<std::wstring> AkVCam::IpcBridgePrivate::findFiles(const std::wstring &path,
                                                              const std::wstring &fileName) const
{
    std::vector<std::wstring> plugins;

    for (auto file: this->findFiles(path)) {
        auto pos = file.rfind(L"\\");
        auto fName = file.substr(pos + 1);

        if (!lstrcmpi(fName.c_str(), fileName.c_str()))
            plugins.push_back(file);
    }

    return plugins;
}

std::string AkVCam::IpcBridgePrivate::regAddLine(const std::string &key,
                                                 const std::string &value,
                                                 const std::string &data) const
{
    std::stringstream ss;
    ss << "reg add \""
       << key
       << "\" /v "
       << value
       << " /d \""
       << data
       << "\" /f";

    return ss.str();
}

std::string AkVCam::IpcBridgePrivate::regAddLine(const std::string &key,
                                                 const std::string &value,
                                                 int data) const
{
    std::stringstream ss;
    ss << "reg add \""
       << key
       << "\" /v "
       << value
       << " /t REG_DWORD"
       << " /d "
       << data
       << " /f";

    return ss.str();
}

std::string AkVCam::IpcBridgePrivate::regDeleteLine(const std::string &key) const
{
    return std::string("reg delete \"" + key + "\" /f");
}

std::string AkVCam::IpcBridgePrivate::regDeleteLine(const std::string &key,
                                                    const std::string &value) const
{
    return std::string("reg delete \"" + key + "\" /v \"" + value + "\" /f");
}

std::string AkVCam::IpcBridgePrivate::regMoveLine(const std::string &fromKey,
                                                  const std::string &toKey) const
{
    std::stringstream ss;
    ss << "reg copy \"" << fromKey << "\" \"" << toKey << "\" /s /f"
       << std::endl
       << regDeleteLine(fromKey);

    return ss.str();
}

std::string AkVCam::IpcBridgePrivate::dirname(const std::string &path) const
{
    return path.substr(0, path.rfind("\\"));
}

void AkVCam::IpcBridgePrivate::updateDeviceSharedProperties()
{
    for (DWORD i = 0; i < camerasCount(); i++) {
        auto cameraPath = AkVCam::cameraPath(i);
        std::string deviceId(cameraPath.begin(), cameraPath.end());

        Message message;
        message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING;
        message.dataSize = sizeof(MsgBroadcasting);
        auto data = messageData<MsgBroadcasting>(&message);
        memcpy(data->device,
               deviceId.c_str(),
               (std::min<size_t>)(deviceId.size(), MAX_STRING));
        MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                                   &message);
        this->updateDeviceSharedProperties(deviceId,
                                           std::string(data->broadcaster));
    }
}

void AkVCam::IpcBridgePrivate::updateDeviceSharedProperties(const std::string &deviceId,
                                                            const std::string &owner)
{
    if (owner.empty()) {
        this->devices[deviceId] = {SharedMemory(), Mutex()};
    } else {
        Mutex mutex(std::wstring(owner.begin(), owner.end()) + L".mutex");
        SharedMemory sharedMemory;
        sharedMemory.setName(L"Local\\"
                             + std::wstring(owner.begin(), owner.end())
                             + L".data");

        if (sharedMemory.open())
            this->devices[deviceId] = {sharedMemory, mutex};
    }
}

std::string AkVCam::IpcBridgePrivate::locateDriverPath() const
{
    std::string driverPath;

    for (auto it = this->driverPaths.rbegin();
         it != this->driverPaths.rend();
         it++) {
        auto path = *it;
        path = replace(path, "/", "\\");

        if (path.back() != '\\')
            path += '\\';

        path += DSHOW_PLUGIN_NAME ".plugin";

        if (this->findFiles(path, DSHOW_PLUGIN_NAME ".dll").empty())
            continue;

        if (this->findFiles(path, DSHOW_PLUGIN_ASSISTANT_NAME ".exe").empty())
            continue;

        driverPath = path;

        break;
    }

    return driverPath;
}

void AkVCam::IpcBridgePrivate::isAlive(Message *message)
{
    auto data = messageData<MsgIsAlive>(message);
    data->alive = true;
}

void AkVCam::IpcBridgePrivate::frameReady(Message *message)
{
    auto data = messageData<MsgFrameReady>(message);
    std::string deviceId(data->device);

    if (this->devices.count(deviceId) < 1) {
        this->updateDeviceSharedProperties(deviceId, std::string(data->port));

        return;
    }

    auto frame =
            reinterpret_cast<Frame *>(this->devices[deviceId]
                                      .sharedMemory
                                      .lock(&this->devices[deviceId].mutex));

    if (!frame)
        return;

    VideoFrame videoFrame({frame->format, frame->width, frame->height},
                          frame->data, frame->size);
    this->devices[deviceId].sharedMemory.unlock(&this->devices[deviceId].mutex);

    if (this->frameReadyCallback)
        this->frameReadyCallback(deviceId, videoFrame);
}

void AkVCam::IpcBridgePrivate::setBroadcasting(Message *message)
{
    auto data = messageData<MsgBroadcasting>(message);
    std::string deviceId(data->device);
    std::string broadcaster(data->broadcaster);
    this->updateDeviceSharedProperties(deviceId, broadcaster);

    if (this->broadcastingChangedCallback)
        this->broadcastingChangedCallback(deviceId, broadcaster);
}

void AkVCam::IpcBridgePrivate::setMirror(Message *message)
{
    auto data = messageData<MsgMirroring>(message);
    std::string deviceId(data->device);

    if (this->mirrorChangedCallback)
        this->mirrorChangedCallback(deviceId, data->hmirror, data->vmirror);
}

void AkVCam::IpcBridgePrivate::setScaling(Message *message)
{
    auto data = messageData<MsgScaling>(message);
    std::string deviceId(data->device);

    if (this->scalingChangedCallback)
        this->scalingChangedCallback(deviceId, data->scaling);
}

void AkVCam::IpcBridgePrivate::setAspectRatio(Message *message)
{
    auto data = messageData<MsgAspectRatio>(message);
    std::string deviceId(data->device);

    if (this->aspectRatioChangedCallback)
        this->aspectRatioChangedCallback(deviceId, data->aspect);
}

void AkVCam::IpcBridgePrivate::setSwapRgb(Message *message)
{
    auto data = messageData<MsgSwapRgb>(message);
    std::string deviceId(data->device);

    if (this->swapRgbChangedCallback)
        this->swapRgbChangedCallback(deviceId, data->swap);
}

void AkVCam::IpcBridgePrivate::listenerAdd(Message *message)
{
    auto data = messageData<MsgListeners>(message);
    std::string deviceId(data->device);

    if (this->listenerAddedCallback)
        this->listenerAddedCallback(deviceId, std::string(data->listener));
}

void AkVCam::IpcBridgePrivate::listenerRemove(Message *message)
{
    auto data = messageData<MsgListeners>(message);
    std::string deviceId(data->device);

    if (this->listenerRemovedCallback)
        this->listenerRemovedCallback(deviceId, std::string(data->listener));
}

int AkVCam::IpcBridgePrivate::sudo(const std::vector<std::string> &parameters,
                                   const std::wstring &directory,
                                   bool show)
{
    if (parameters.size() < 1)
        return E_FAIL;

    auto command = parameters[0];
    std::wstring wcommand(command.begin(), command.end());
    std::wstring wparameters;

    for (size_t i = 1; i < parameters.size(); i++) {
        auto param = parameters[i];

        if (i > 1)
            wparameters += L" ";

        wparameters += std::wstring(param.begin(), param.end());
    }

    SHELLEXECUTEINFO execInfo;
    memset(&execInfo, 0, sizeof(SHELLEXECUTEINFO));
    execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    execInfo.hwnd = nullptr;
    execInfo.lpVerb = L"runas";
    execInfo.lpFile = wcommand.data();
    execInfo.lpParameters = wparameters.data();
    execInfo.lpDirectory = directory.data();
    execInfo.nShow = show? SW_SHOWNORMAL: SW_HIDE;
    execInfo.hInstApp = nullptr;
    ShellExecuteEx(&execInfo);

    if (!execInfo.hProcess)
        return E_FAIL;

    WaitForSingleObject(execInfo.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(execInfo.hProcess, &exitCode);
    CloseHandle(execInfo.hProcess);

    return int(exitCode);
}
