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
#include <mutex>
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
#include "VCamUtils/src/logger/logger.h"

#define AkIpcBridgeLogMethod() \
    AkLoggerLog("IpcBridge::", __FUNCTION__, "()")

#define AkIpcBridgePrivateLogMethod() \
    AkLoggerLog("IpcBridgePrivate::", __FUNCTION__, "()")

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
            IpcBridge *self;
            std::map<std::string, DeviceSharedProperties> m_devices;
            std::map<uint32_t, MessageHandler> m_messageHandlers;
            std::vector<std::string> m_broadcasting;
            std::map<std::string, std::string> m_options;
            MessageServer m_messageServer;
            MessageServer m_mainServer;
            SharedMemory m_sharedMemory;
            Mutex m_globalMutex;
            std::string m_portName;
            std::wstring m_error;
            bool m_asClient;

            explicit IpcBridgePrivate(IpcBridge *self);
            ~IpcBridgePrivate();

            static inline std::vector<std::wstring> *driverPaths();
            std::vector<MonikerPtr> listCameras() const;
            BaseFilterPtr filter(IMoniker *moniker) const;
            PropertyBagPtr propertyBag(IMoniker *moniker) const;
            bool isVirtualCamera(const MonikerPtr &moniker) const;
            bool isVirtualCamera(IBaseFilter *baseFilter) const;
            std::string cameraPath(const MonikerPtr &moniker) const;
            std::string cameraPath(IPropertyBag *propertyBag) const;
            std::wstring cameraDescription(const MonikerPtr &moniker) const;
            std::wstring cameraDescription(IPropertyBag *propertyBag) const;
            std::vector<PinPtr> enumPins(IBaseFilter *baseFilter) const;
            std::vector<VideoFormat> enumVideoFormats(IPin *pin) const;
            std::vector<std::wstring> findFiles(const std::wstring &path) const;
            std::vector<std::string> findFiles(const std::string &path,
                                               const std::string &fileName) const;
            std::vector<std::wstring> findFiles(const std::wstring &path,
                                                const std::wstring &fileName) const;
            std::wstring regAddLine(const std::wstring &key,
                                   const std::wstring &value,
                                   const std::wstring &data,
                                   BOOL wow=false) const;
            std::wstring regAddLine(const std::wstring &key,
                                   const std::wstring &value,
                                   int data,
                                   BOOL wow=false) const;
            std::wstring regDeleteLine(const std::wstring &key,
                                      BOOL wow=false) const;
            std::wstring regDeleteLine(const std::wstring &key,
                                      const std::wstring &value,
                                      BOOL wow=false) const;
            std::wstring regMoveLine(const std::wstring &fromKey,
                                    const std::wstring &toKey,
                                    BOOL wow=false) const;
            std::wstring dirname(const std::wstring &path) const;
            void updateDeviceSharedProperties();
            void updateDeviceSharedProperties(const std::string &deviceId,
                                              const std::string &owner);
            std::wstring locateDriverPath() const;
            static void pipeStateChanged(void *userData,
                                         MessageServer::PipeState state);

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
    this->d = new IpcBridgePrivate(this);
}

AkVCam::IpcBridge::~IpcBridge()
{
    delete this->d;
}

std::wstring AkVCam::IpcBridge::errorMessage() const
{
    return this->d->m_error;
}

void AkVCam::IpcBridge::setOption(const std::string &key, const std::string &value)
{
    AkIpcBridgeLogMethod();

    if (value.empty())
        this->d->m_options.erase(key);
    else
        this->d->m_options[key] = value;
}

std::vector<std::wstring> AkVCam::IpcBridge::driverPaths() const
{
    AkIpcBridgeLogMethod();

    return *this->d->driverPaths();
}

void AkVCam::IpcBridge::setDriverPaths(const std::vector<std::wstring> &driverPaths)
{
    AkIpcBridgeLogMethod();
    *this->d->driverPaths() = driverPaths;
}

std::vector<std::string> AkVCam::IpcBridge::availableDrivers() const
{
    return {"AkVirtualCamera"};
}

std::string AkVCam::IpcBridge::driver() const
{
    return {"AkVirtualCamera"};
}

bool AkVCam::IpcBridge::setDriver(const std::string &driver)
{
    return driver == "AkVirtualCamera";
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

void AkVCam::IpcBridge::connectService(bool asClient)
{
    AkIpcBridgeLogMethod();
    this->d->m_asClient = asClient;
    this->d->m_mainServer.start();
}

void AkVCam::IpcBridge::disconnectService()
{
    AkIpcBridgeLogMethod();
    this->d->m_mainServer.stop(true);
    this->d->m_asClient = false;
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
    auto pipeName = "\\\\.\\pipe\\" + portName;
    this->d->m_messageServer.setPipeName(std::wstring(pipeName.begin(),
                                                      pipeName.end()));
    this->d->m_messageServer.setHandlers(this->d->m_messageHandlers);
    AkLoggerLog("Recommended port name: ", portName);

    if (!this->d->m_messageServer.start()) {
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
        this->d->m_messageServer.stop(true);

        return false;
    }

    if (!addData->status) {
        this->d->m_messageServer.stop(true);

        return false;
    }

    this->d->m_sharedMemory.setName(L"Local\\"
                                  + std::wstring(portName.begin(),
                                                 portName.end())
                                  + L".data");
    this->d->m_globalMutex = Mutex(std::wstring(portName.begin(),
                                                portName.end())
                                  + L".mutex");
    this->d->m_portName = portName;
    AkLoggerLog("Peer registered as ", portName);

    return true;
}

void AkVCam::IpcBridge::unregisterPeer()
{
    AkIpcBridgeLogMethod();

    if (this->d->m_portName.empty())
        return;

    this->d->m_sharedMemory.setName({});
    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_REMOVE_PORT;
    message.dataSize = sizeof(MsgRemovePort);
    auto data = messageData<MsgRemovePort>(&message);
    memcpy(data->port,
           this->d->m_portName.c_str(),
           (std::min<size_t>)(this->d->m_portName.size(), MAX_STRING));
    MessageServer::sendMessage(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L,
                               &message);
    this->d->m_messageServer.stop(true);
    this->d->m_portName.clear();
}

std::vector<std::string> AkVCam::IpcBridge::listDevices() const
{
    AkIpcBridgeLogMethod();
    std::vector<std::string> devices;

    for (auto camera: this->d->listCameras())
        if (this->d->isVirtualCamera(camera))
            devices.push_back(this->d->cameraPath(camera));

#ifdef QT_DEBUG
    AkLoggerLog("Devices:");

    for (auto &device:  devices)
        AkLoggerLog("    ", device);
#endif

    return devices;
}

std::wstring AkVCam::IpcBridge::description(const std::string &deviceId) const
{
    AkIpcBridgeLogMethod();

    for (auto camera: this->d->listCameras()) {
        auto propertyBag = this->d->propertyBag(camera.get());

        if (this->d->isVirtualCamera(camera)
            && this->d->cameraPath(propertyBag.get()) == deviceId)
            return this->d->cameraDescription(propertyBag.get());
    }

    return {};
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

AkVCam::PixelFormat AkVCam::IpcBridge::defaultOutputPixelFormat() const
{
    return PixelFormatYUY2;
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

    if (!this->d->m_mainServer.sendMessage(&message))
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

    if (!this->d->m_mainServer.sendMessage(&message))
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

    if (!this->d->m_mainServer.sendMessage(&message))
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

    if (!this->d->m_mainServer.sendMessage(&message))
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

    if (!this->d->m_mainServer.sendMessage(&message))
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

    if (!this->d->m_mainServer.sendMessage(&message))
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

    if (!this->d->m_mainServer.sendMessage(&message))
        return {};

    if (!data->status)
        return {};

    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER;
    std::vector<std::string> listeners;

    for (size_t i = 0; i < data->nlistener; i++) {
        data->nlistener = i;

        if (!this->d->m_mainServer.sendMessage(&message))
            continue;

        if (!data->status)
            continue;

        listeners.push_back(std::string(data->listener));
    }

    return listeners;
}

std::string AkVCam::IpcBridge::deviceCreate(const std::wstring &description,
                                            const std::vector<VideoFormat> &formats)
{
    AkIpcBridgeLogMethod();

    auto driverPath = this->d->locateDriverPath();

    if (driverPath.empty()) {
        this->d->m_error = L"Driver not found";

        return {};
    }

    // Create a device path for the new device and add it's entry.
    auto devicePath = createDevicePath();

    if (devicePath.empty()) {
        this->d->m_error = L"Can't create a device";

        return {};
    }

    std::wstringstream ss;
    ss << L"@echo off" << std::endl;
    ss << L"chcp " << GetACP() << std::endl;

    auto driverInstallPath =
            programFilesPath() + L"\\" DSHOW_PLUGIN_NAME_L L".plugin";

    // Copy all plugins
    std::vector<std::wstring> installPaths;

    for (auto path: this->d->findFiles(driverPath,
                                       DSHOW_PLUGIN_NAME_L L".dll")) {
        auto installPath = replace(path, driverPath, driverInstallPath);

        if (!isEqualFile(path, installPath))
            ss << L"mkdir \""
               << this->d->dirname(installPath)
               << L"\""
               << std::endl
               << L"copy /y \""
               << path
               << L"\" \""
               << installPath
               << L"\""
               << std::endl;

        installPaths.push_back(installPath);
    }

    // Copy all services
    std::vector<std::wstring> assistantInstallPaths;

    for (auto path: this->d->findFiles(driverPath,
                                       DSHOW_PLUGIN_ASSISTANT_NAME_L L".exe")) {
        auto installPath = replace(path, driverPath, driverInstallPath);

        if (!isEqualFile(path, installPath))
            ss << L"mkdir \""
               << this->d->dirname(installPath)
               << L"\""
               << std::endl
               << L"copy /y \""
               << path
               << L"\" \""
               << installPath
               << L"\""
               << std::endl;

        assistantInstallPaths.push_back(installPath);
    }

    // Copy shared files
    for (auto path: this->d->findFiles(driverPath + L"/share")) {
        auto installPath = replace(path, driverPath, driverInstallPath);

        if (!isEqualFile(path, installPath))
            ss << L"mkdir \""
               << this->d->dirname(installPath)
               << L"\""
               << std::endl
               << L"copy /y \""
               << path
               << L"\" \""
               << installPath
               << L"\""
               << std::endl;
    }

    BOOL wow = isWow64();

    // List cameras and create a line with the number of cameras.
    auto nCameras = camerasCount();

    ss << this->d->regAddLine(L"HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras",
                              L"size",
                              int(nCameras + 1),
                              wow)
       << std::endl;

    ss << this->d->regAddLine(L"HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                              + std::to_wstring(nCameras + 1),
                              L"path",
                              devicePath,
                              wow)
       << std::endl;

    // Add description line.
    ss << this->d->regAddLine(L"HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                              + std::to_wstring(nCameras + 1),
                              L"description",
                              description,
                              wow)
       << std::endl;

    // Set number of formats.
    ss << this->d->regAddLine(L"HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                              + std::to_wstring(nCameras + 1)
                              + L"\\Formats",
                              L"size",
                              int(formats.size()),
                              wow)
       << std::endl;

    // Setup formats.
    for (size_t i = 0; i < formats.size(); i++) {
        auto videoFormat = formats[i];
        auto format = VideoFormat::wstringFromFourcc(videoFormat.fourcc());

        ss << this->d->regAddLine(L"HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                                  + std::to_wstring(nCameras + 1)
                                  + L"\\Formats\\"
                                  + std::to_wstring(i + 1),
                                  L"format",
                                  format,
                                  wow)
           << std::endl;

        ss << this->d->regAddLine(L"HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                                  + std::to_wstring(nCameras + 1)
                                  + L"\\Formats\\"
                                  + std::to_wstring(i + 1),
                                  L"width",
                                  videoFormat.width(),
                                  wow)
           << std::endl;

        ss << this->d->regAddLine(L"HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                                  + std::to_wstring(nCameras + 1)
                                  + L"\\Formats\\"
                                  + std::to_wstring(i + 1),
                                  L"height",
                                  videoFormat.height(),
                                  wow)
           << std::endl;

        ss << this->d->regAddLine(L"HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                                  + std::to_wstring(nCameras + 1)
                                  + L"\\Formats\\"
                                  + std::to_wstring(i + 1),
                                  L"fps",
                                  videoFormat.minimumFrameRate().toWString(),
                                  wow)
           << std::endl;
    }

    for (auto path: installPaths)
        ss << L"regsvr32 /s \"" << path << L"\"" << std::endl;

    std::vector<std::wstring> preferredArch;

    if (wow)
        preferredArch.push_back(L"x64");

    preferredArch.push_back(DSHOW_PLUGIN_ARCH_L);

    if (wcscmp(DSHOW_PLUGIN_ARCH_L, L"x64") == 0)
        preferredArch.push_back(L"x32");

    for (auto &arch: preferredArch) {
        auto assistantPath = driverInstallPath
                           + L"\\"
                           + arch
                           + L"\\" DSHOW_PLUGIN_ASSISTANT_NAME_L L".exe";

        if (std::find(assistantInstallPaths.begin(),
                      assistantInstallPaths.end(),
                      assistantPath) != assistantInstallPaths.end()) {
            ss << "\"" << assistantPath << "\" --install" << std::endl;

            break;
        }
    }

    // Create the script.
    auto temp = tempPath();
    auto scriptPath = std::string(temp.begin(), temp.end())
                    + "\\device_create_"
                    + timeStamp()
                    + ".bat";
    std::wfstream script;
    script.imbue(std::locale(""));
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

bool AkVCam::IpcBridge::deviceDestroy(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    auto camera = cameraFromId(std::wstring(deviceId.begin(), deviceId.end()));

    if (camera < 0)
        return false;

    auto driverPath = this->d->locateDriverPath();

    if (driverPath.empty()) {
        this->d->m_error = L"Driver not found";

        return false;
    }

    std::wstringstream ss;
    ss << L"@echo off" << std::endl;
    ss << L"chcp " << GetACP() << std::endl;

    auto driverInstallPath =
            programFilesPath() + L"\\" DSHOW_PLUGIN_NAME_L L".plugin";
    std::vector<std::wstring> installPaths;

    for (auto path: this->d->findFiles(std::wstring(driverPath.begin(),
                                                    driverPath.end()),
                                       DSHOW_PLUGIN_NAME_L L".dll")) {
        auto installPath = replace(path, driverPath, driverInstallPath);

        installPaths.push_back(installPath);
    }

    std::vector<std::wstring> assistantInstallPaths;

    for (auto path: this->d->findFiles(std::wstring(driverPath.begin(),
                                                    driverPath.end()),
                                       DSHOW_PLUGIN_ASSISTANT_NAME_L L".exe")) {
        auto installPath = replace(path, driverPath, driverInstallPath);

        assistantInstallPaths.push_back(installPath);
    }

    BOOL wow = isWow64();

    // List cameras and create a line with the number of cameras.
    auto nCameras = camerasCount();

    if (nCameras > 1) {
        ss << this->d->regAddLine(L"HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras",
                                  L"size",
                                  int(nCameras - 1),
                                  wow)
           << std::endl;

        ss << this->d->regDeleteLine(L"HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                                     + std::to_wstring(camera + 1),
                                     wow)
           << std::endl;

        for (DWORD i = DWORD(camera + 1); i < nCameras; i++) {
            ss << this->d->regMoveLine(L"HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                                       + std::to_wstring(i + 1),
                                       L"HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                                       + std::to_wstring(i),
                                       wow)
               << std::endl;
        }

        for (auto path: installPaths)
            ss << L"regsvr32 /s \"" << path << L"\"" << std::endl;
    } else {
        for (auto path: installPaths)
            ss << L"regsvr32 /s /u \"" << path << L"\"" << std::endl;

        for (auto path: assistantInstallPaths)
            ss << L"\"" << path << L"\" --uninstall" << std::endl;

        ss << this->d->regDeleteLine(L"HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras",
                                     wow)
           << std::endl;

        if (lstrcmpi(driverPath.c_str(), driverInstallPath.c_str()))
            ss << L"rmdir /s /q \"" << driverInstallPath << L"\"" << std::endl;
    }

    auto temp = tempPath();
    auto scriptPath = std::string(temp.begin(), temp.end())
                    + "\\device_destroy_"
                    + timeStamp()
                    + ".bat";
    std::wfstream script;
    script.imbue(std::locale(""));
    script.open(scriptPath, std::ios_base::out | std::ios_base::trunc);

    if (script.is_open()) {
        script << ss.str();
        script.close();
        this->d->sudo({"cmd", "/c", scriptPath});
        std::wstring wScriptPath(scriptPath.begin(), scriptPath.end());
        DeleteFile(wScriptPath.c_str());
    }

    return true;
}

bool AkVCam::IpcBridge::changeDescription(const std::string &deviceId,
                                          const std::wstring &description)
{
    AkIpcBridgeLogMethod();

    auto camera = cameraFromId(std::wstring(deviceId.begin(), deviceId.end()));

    if (camera < 0)
        return false;

    auto driverPath = this->d->locateDriverPath();

    if (driverPath.empty()) {
        this->d->m_error = L"Driver not found";

        return false;
    }

    std::wstringstream ss;
    ss << L"@echo off" << std::endl;
    ss << L"chcp " << GetACP() << std::endl;

    auto driverInstallPath =
            programFilesPath() + L"\\" DSHOW_PLUGIN_NAME_L L".plugin";
    std::vector<std::wstring> installPaths;

    for (auto path: this->d->findFiles(std::wstring(driverPath.begin(),
                                                    driverPath.end()),
                                       DSHOW_PLUGIN_NAME_L L".dll")) {
        auto installPath = replace(path, driverPath, driverInstallPath);
        installPaths.push_back(installPath);
    }

    ss << this->d->regAddLine(L"HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras\\"
                              + std::to_wstring(camera + 1),
                              L"description",
                              description,
                              isWow64())
       << std::endl;

    for (auto path: installPaths)
        ss << L"regsvr32 /s \"" << path << L"\"" << std::endl;

    auto temp = tempPath();
    auto scriptPath = std::string(temp.begin(), temp.end())
                    + "\\device_change_description_"
                    + timeStamp()
                    + ".bat";
    std::wfstream script;
    script.imbue(std::locale(""));
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

    if (driverPath.empty()) {
        this->d->m_error = L"Driver not found";

        return false;
    }

    std::wstringstream ss;
    ss << L"@echo off" << std::endl;
    ss << L"chcp " << GetACP() << std::endl;

    auto driverInstallPath =
            programFilesPath() + L"\\" DSHOW_PLUGIN_NAME_L L".plugin";

    for (auto path: this->d->findFiles(std::wstring(driverPath.begin(),
                                                    driverPath.end()),
                                       DSHOW_PLUGIN_NAME_L L".dll")) {
        auto installPath = replace(path, driverPath, driverInstallPath);
        ss << L"regsvr32 /s /u \"" << installPath << L"\"" << std::endl;
    }

    for (auto path: this->d->findFiles(std::wstring(driverPath.begin(),
                                                    driverPath.end()),
                                       DSHOW_PLUGIN_ASSISTANT_NAME_L L".exe")) {
        auto installPath = replace(path, driverPath, driverInstallPath);
        ss << L"\"" << installPath << L"\" --uninstall" << std::endl;
    }

    ss << this->d->regDeleteLine(L"HKLM\\SOFTWARE\\Webcamoid\\VirtualCamera\\Cameras",
                                 isWow64())
       << std::endl;

    if (lstrcmpi(driverPath.c_str(), driverInstallPath.c_str()))
        ss << "rmdir /s /q \"" << driverInstallPath << L"\"" << std::endl;

    auto temp = tempPath();
    auto scriptPath = std::string(temp.begin(), temp.end())
                    + "\\device_remove_all_"
                    + timeStamp()
                    + ".bat";
    std::wfstream script;
    script.imbue(std::locale(""));
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

bool AkVCam::IpcBridge::deviceStart(const std::string &deviceId,
                                    const VideoFormat &format)
{
    UNUSED(format)
    AkIpcBridgeLogMethod();
    auto it = std::find(this->d->m_broadcasting.begin(),
                        this->d->m_broadcasting.end(),
                        deviceId);

    if (it != this->d->m_broadcasting.end())
        return false;

    std::wstring portName(this->d->m_portName.begin(),
                          this->d->m_portName.end());
    this->d->m_sharedMemory.setName(L"Local\\" + portName + L".data");
    this->d->m_globalMutex = Mutex(portName + L".mutex");

    if (!this->d->m_sharedMemory.open(maxBufferSize,
                                      SharedMemory::OpenModeWrite))
        return false;

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING;
    message.dataSize = sizeof(MsgBroadcasting);
    auto data = messageData<MsgBroadcasting>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));
    memcpy(data->broadcaster,
           this->d->m_portName.c_str(),
           (std::min<size_t>)(this->d->m_portName.size(), MAX_STRING));

    if (!this->d->m_mainServer.sendMessage(&message)) {
        this->d->m_sharedMemory.close();

        return false;
    }

    if (!data->status)
        return false;

    this->d->m_broadcasting.push_back(deviceId);

    return true;
}

void AkVCam::IpcBridge::deviceStop(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();
    auto it = std::find(this->d->m_broadcasting.begin(),
                        this->d->m_broadcasting.end(),
                        deviceId);

    if (it == this->d->m_broadcasting.end())
        return;

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING;
    message.dataSize = sizeof(MsgBroadcasting);
    auto data = messageData<MsgBroadcasting>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));

    this->d->m_mainServer.sendMessage(&message);
    this->d->m_sharedMemory.close();
    this->d->m_broadcasting.erase(it);
}

bool AkVCam::IpcBridge::write(const std::string &deviceId,
                              const VideoFrame &frame)
{
    AkIpcBridgeLogMethod();

    if (frame.format().size() < 1)
        return false;

    auto buffer =
            reinterpret_cast<Frame *>(this->d->m_sharedMemory.lock(&this->d->m_globalMutex));

    if (!buffer)
        return false;

    if (size_t(frame.format().width() * frame.format().height()) > maxFrameSize) {
        auto scaledFrame = frame.scaled(maxFrameSize);
        buffer->format = scaledFrame.format().fourcc();
        buffer->width = scaledFrame.format().width();
        buffer->height = scaledFrame.format().height();
        buffer->size = uint32_t(frame.data().size());
        memcpy(buffer->data,
               scaledFrame.data().data(),
               scaledFrame.data().size());
    } else {
        buffer->format = frame.format().fourcc();
        buffer->width = frame.format().width();
        buffer->height = frame.format().height();
        buffer->size = uint32_t(frame.data().size());
        memcpy(buffer->data,
               frame.data().data(),
               frame.data().size());
    }

    this->d->m_sharedMemory.unlock(&this->d->m_globalMutex);

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_FRAME_READY;
    message.dataSize = sizeof(MsgFrameReady);
    auto data = messageData<MsgFrameReady>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));
    memcpy(data->port,
           this->d->m_portName.c_str(),
           (std::min<size_t>)(this->d->m_portName.size(), MAX_STRING));

    return this->d->m_mainServer.sendMessage(&message) == TRUE;
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
    this->d->m_mainServer.sendMessage(&message);
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
    this->d->m_mainServer.sendMessage(&message);
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
    this->d->m_mainServer.sendMessage(&message);
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
    this->d->m_mainServer.sendMessage(&message);
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
           this->d->m_portName.c_str(),
           (std::min<size_t>)(this->d->m_portName.size(), MAX_STRING));

    if (!this->d->m_mainServer.sendMessage(&message))
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
           this->d->m_portName.c_str(),
           (std::min<size_t>)(this->d->m_portName.size(), MAX_STRING));

    if (!this->d->m_mainServer.sendMessage(&message))
        return false;

    return data->status;
}

AkVCam::IpcBridgePrivate::IpcBridgePrivate(IpcBridge *self):
    self(self),
    m_asClient(false)
{
    this->m_mainServer.setPipeName(L"\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME_L);
    this->m_mainServer.setMode(MessageServer::ServerModeSend);
    this->m_mainServer.connectPipeStateChanged(this,
                                               &IpcBridgePrivate::pipeStateChanged);
    this->updateDeviceSharedProperties();

    this->m_messageHandlers = std::map<uint32_t, MessageHandler> {
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

AkVCam::IpcBridgePrivate::~IpcBridgePrivate()
{
    this->m_mainServer.stop(true);
}

std::vector<std::wstring> *AkVCam::IpcBridgePrivate::driverPaths()
{
    static std::vector<std::wstring> paths;

    return &paths;
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
            cameras.push_back(MonikerPtr(moniker, [](IMoniker *moniker) {
                moniker->Release();
            }));

        enumMoniker->Release();
    }

    deviceEnumerator->Release();

    return cameras;
}

AkVCam::BaseFilterPtr AkVCam::IpcBridgePrivate::filter(IMoniker *moniker) const
{
    if (!moniker)
        return {};

    IBaseFilter *baseFilter = nullptr;

    if (FAILED(moniker->BindToObject(nullptr,
                                     nullptr,
                                     IID_IBaseFilter,
                                     reinterpret_cast<void **>(&baseFilter)))) {
        return {};
    }

    return BaseFilterPtr(baseFilter, [] (IBaseFilter *baseFilter) {
        baseFilter->Release();
    });
}

AkVCam::PropertyBagPtr AkVCam::IpcBridgePrivate::propertyBag(IMoniker *moniker) const
{
    if (!moniker)
        return {};

    IPropertyBag *propertyBag = nullptr;

    if (FAILED(moniker->BindToStorage(nullptr,
                                      nullptr,
                                      IID_IPropertyBag,
                                      reinterpret_cast<void **>(&propertyBag)))) {
        return {};
    }

    return PropertyBagPtr(propertyBag, [] (IPropertyBag *propertyBag) {
        propertyBag->Release();
    });
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
    std::string devicePath(wstr.begin(), wstr.end());
    VariantClear(&var);

    return devicePath;
}

std::wstring AkVCam::IpcBridgePrivate::cameraDescription(const MonikerPtr &moniker) const
{
    auto propertyBag = this->propertyBag(moniker.get());

    return this->cameraDescription(propertyBag.get());
}

std::wstring AkVCam::IpcBridgePrivate::cameraDescription(IPropertyBag *propertyBag) const
{
    VARIANT var;
    VariantInit(&var);

    if (FAILED(propertyBag->Read(L"Description", &var, nullptr)))
        if (FAILED(propertyBag->Read(L"FriendlyName", &var, nullptr)))
            return {};

    std::wstring wstr(var.bstrVal);
    VariantClear(&var);

    return wstr;
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
                pins.push_back(PinPtr(pin, [] (IPin *pin) {
                    pin->Release();
                }));

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

        if (format.size() > 0)
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
    auto wfiles =
            this->findFiles(std::wstring(path.begin(), path.end()),
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

std::wstring AkVCam::IpcBridgePrivate::regAddLine(const std::wstring &key,
                                                  const std::wstring &value,
                                                  const std::wstring &data,
                                                  BOOL wow) const
{
    std::wstringstream ss;
    ss << L"reg add \""
       << key
       << L"\" /v "
       << value
       << L" /d \""
       << data
       << L"\" /f";

    if (wow)
        ss << L" /reg:64";

    return ss.str();
}

std::wstring AkVCam::IpcBridgePrivate::regAddLine(const std::wstring &key,
                                                  const std::wstring &value,
                                                  int data,
                                                  BOOL wow) const
{
    std::wstringstream ss;
    ss << L"reg add \""
       << key
       << L"\" /v "
       << value
       << L" /t REG_DWORD"
       << L" /d "
       << data
       << L" /f";

    if (wow)
        ss << L" /reg:64";

    return ss.str();
}

std::wstring AkVCam::IpcBridgePrivate::regDeleteLine(const std::wstring &key,
                                                     BOOL wow) const
{
    std::wstringstream ss;
    ss << L"reg delete \"" + key + L"\" /f";

    if (wow)
        ss << L" /reg:64";

    return ss.str();
}

std::wstring AkVCam::IpcBridgePrivate::regDeleteLine(const std::wstring &key,
                                                     const std::wstring &value,
                                                     BOOL wow) const
{
    std::wstringstream ss;
    ss << L"reg delete \"" + key + L"\" /v \"" + value + L"\" /f";

    if (wow)
        ss << L" /reg:64";

    return ss.str();
}

std::wstring AkVCam::IpcBridgePrivate::regMoveLine(const std::wstring &fromKey,
                                                   const std::wstring &toKey,
                                                   BOOL wow) const
{
    std::wstringstream ss;
    ss << L"reg copy \"" << fromKey << L"\" \"" << toKey << L"\" /s /f";

    if (wow)
        ss << L" /reg:64";

    ss << std::endl
       << regDeleteLine(fromKey, wow);

    return ss.str();
}

std::wstring AkVCam::IpcBridgePrivate::dirname(const std::wstring &path) const
{
    return path.substr(0, path.rfind(L"\\"));
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
        this->m_mainServer.sendMessage(&message);
        this->updateDeviceSharedProperties(deviceId,
                                           std::string(data->broadcaster));
    }
}

void AkVCam::IpcBridgePrivate::updateDeviceSharedProperties(const std::string &deviceId,
                                                            const std::string &owner)
{
    if (owner.empty()) {
        this->m_devices[deviceId] = {SharedMemory(), Mutex()};
    } else {
        Mutex mutex(std::wstring(owner.begin(), owner.end()) + L".mutex");
        SharedMemory sharedMemory;
        sharedMemory.setName(L"Local\\"
                             + std::wstring(owner.begin(), owner.end())
                             + L".data");

        if (sharedMemory.open())
            this->m_devices[deviceId] = {sharedMemory, mutex};
    }
}

std::wstring AkVCam::IpcBridgePrivate::locateDriverPath() const
{
    std::wstring driverPath;

    for (auto it = this->driverPaths()->rbegin();
         it != this->driverPaths()->rend();
         it++) {
        auto path = *it;
        path = replace(path, L"/", L"\\");

        if (path.back() != L'\\')
            path += L'\\';

        path += DSHOW_PLUGIN_NAME_L L".plugin";

        if (this->findFiles(path, DSHOW_PLUGIN_NAME_L L".dll").empty())
            continue;

        if (this->findFiles(path, DSHOW_PLUGIN_ASSISTANT_NAME_L L".exe").empty())
            continue;

        driverPath = path;

        break;
    }

    return driverPath;
}

void AkVCam::IpcBridgePrivate::pipeStateChanged(void *userData,
                                                MessageServer::PipeState state)
{
    AkIpcBridgePrivateLogMethod();
    auto self = reinterpret_cast<IpcBridgePrivate *>(userData);

    switch (state) {
    case MessageServer::PipeStateAvailable:
        AkLoggerLog("Server Available");

        if (self->self->registerPeer(self->m_asClient)) {
            AKVCAM_EMIT(self->self,
                        ServerStateChanged,
                        IpcBridge::ServerStateAvailable)
        }

        break;

    case MessageServer::PipeStateGone:
        AkLoggerLog("Server Gone");
        AKVCAM_EMIT(self->self,
                    ServerStateChanged,
                    IpcBridge::ServerStateGone)
        self->self->unregisterPeer();

        break;
    }
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

    if (this->m_devices.count(deviceId) < 1) {
        this->updateDeviceSharedProperties(deviceId, std::string(data->port));

        return;
    }

    auto frame =
            reinterpret_cast<Frame *>(this->m_devices[deviceId]
                                      .sharedMemory
                                      .lock(&this->m_devices[deviceId].mutex));

    if (!frame)
        return;

    VideoFormat videoFormat(frame->format, frame->width, frame->height);
    VideoFrame videoFrame(videoFormat);
    memcpy(videoFrame.data().data(), frame->data, frame->size);
    this->m_devices[deviceId].sharedMemory.unlock(&this->m_devices[deviceId].mutex);
    AKVCAM_EMIT(this->self, FrameReady, deviceId, videoFrame)
}

void AkVCam::IpcBridgePrivate::setBroadcasting(Message *message)
{
    auto data = messageData<MsgBroadcasting>(message);
    std::string deviceId(data->device);
    std::string broadcaster(data->broadcaster);
    this->updateDeviceSharedProperties(deviceId, broadcaster);
    AKVCAM_EMIT(this->self, BroadcastingChanged, deviceId, broadcaster)
}

void AkVCam::IpcBridgePrivate::setMirror(Message *message)
{
    auto data = messageData<MsgMirroring>(message);
    std::string deviceId(data->device);
    AKVCAM_EMIT(this->self,
                MirrorChanged,
                deviceId,
                data->hmirror,
                data->vmirror)
}

void AkVCam::IpcBridgePrivate::setScaling(Message *message)
{
    auto data = messageData<MsgScaling>(message);
    std::string deviceId(data->device);
    AKVCAM_EMIT(this->self, ScalingChanged, deviceId, data->scaling)
}

void AkVCam::IpcBridgePrivate::setAspectRatio(Message *message)
{
    auto data = messageData<MsgAspectRatio>(message);
    std::string deviceId(data->device);
    AKVCAM_EMIT(this->self, AspectRatioChanged, deviceId, data->aspect)
}

void AkVCam::IpcBridgePrivate::setSwapRgb(Message *message)
{
    auto data = messageData<MsgSwapRgb>(message);
    std::string deviceId(data->device);
    AKVCAM_EMIT(this->self, SwapRgbChanged, deviceId, data->swap)
}

void AkVCam::IpcBridgePrivate::listenerAdd(Message *message)
{
    auto data = messageData<MsgListeners>(message);
    std::string deviceId(data->device);
    AKVCAM_EMIT(this->self,
                ListenerAdded,
                deviceId,
                std::string(data->listener))
}

void AkVCam::IpcBridgePrivate::listenerRemove(Message *message)
{
    auto data = messageData<MsgListeners>(message);
    std::string deviceId(data->device);
    AKVCAM_EMIT(this->self,
                ListenerRemoved,
                deviceId,
                std::string(data->listener))
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

    if (!execInfo.hProcess) {
        this->m_error = L"Failed executing script";

        return E_FAIL;
    }

    WaitForSingleObject(execInfo.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(execInfo.hProcess, &exitCode);
    CloseHandle(execInfo.hProcess);

    if (FAILED(exitCode))
        this->m_error = L"Script failed with code " + std::to_wstring(exitCode);

    return int(exitCode);
}
