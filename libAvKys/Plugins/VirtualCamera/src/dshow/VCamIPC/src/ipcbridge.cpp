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

#include <memory>
#include <sstream>
#include <dshow.h>

#include "ipcbridge.h"
#include "utils.h"
#include "VCamUtils/src/utils.h"
#include "VCamUtils/src/image/videoformat.h"

#define AkIpcBridgeLogMethod() \
    AkLoggerLog("IpcBridge::", __FUNCTION__, "()")

namespace AkVCam
{
    typedef std::shared_ptr<IMoniker> MonikerPtr;
    typedef std::shared_ptr<IBaseFilter> BaseFilterPtr;
    typedef std::shared_ptr<IPropertyBag> PropertyBagPtr;
    typedef std::shared_ptr<IPin> PinPtr;
    typedef std::shared_ptr<AM_MEDIA_TYPE> MediaTypePtr;

    class IpcBridgePrivate
    {
        public:
            std::vector<std::string> devices;

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
    };
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

int AkVCam::IpcBridge::sudo(const std::vector<std::string> &parameters,
                            const std::map<std::string, std::string> &options)
{
    AkIpcBridgeLogMethod();

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

    std::wstring wdirectory;

    if (options.count("directory") > 0) {
        auto directory = options.at("directory");
        wdirectory = std::wstring(directory.begin(), directory.end());
    }

    bool show = false;

    if (options.count("show") > 0)
        show = true;

    SHELLEXECUTEINFO execInfo;
    memset(&execInfo, 0, sizeof(SHELLEXECUTEINFO));

    execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    execInfo.hwnd = nullptr;
    execInfo.lpVerb = L"runas";
    execInfo.lpFile = wcommand.data();
    execInfo.lpParameters = wparameters.data();
    execInfo.lpDirectory = wdirectory.data();
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

bool AkVCam::IpcBridge::registerPeer(bool asClient)
{
    AkIpcBridgeLogMethod();

    return false;
}

void AkVCam::IpcBridge::unregisterPeer()
{
    AkIpcBridgeLogMethod();
}

std::vector<std::string> AkVCam::IpcBridge::listDevices(bool all) const
{
    AkIpcBridgeLogMethod();

    if (!all)
        return this->d->devices;

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

bool AkVCam::IpcBridge::broadcasting(const std::string &deviceId) const
{
    AkIpcBridgeLogMethod();

    return false;
}

bool AkVCam::IpcBridge::isHorizontalMirrored(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    return false;
}

bool AkVCam::IpcBridge::isVerticalMirrored(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    return false;
}

AkVCam::Scaling AkVCam::IpcBridge::scalingMode(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    return AkVCam::Scaling(0);
}

AkVCam::AspectRatio AkVCam::IpcBridge::aspectRatioMode(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    return AkVCam::AspectRatio(0);
}

int AkVCam::IpcBridge::listeners(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    return 0;
}

std::string AkVCam::IpcBridge::deviceCreate(const std::string &description,
                                            const std::vector<VideoFormat> &formats)
{
    AkIpcBridgeLogMethod();

    return {};
}

void AkVCam::IpcBridge::deviceDestroy(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();
}

bool AkVCam::IpcBridge::deviceStart(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    return false;
}

void AkVCam::IpcBridge::deviceStop(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();
}

bool AkVCam::IpcBridge::write(const std::string &deviceId,
                              const VideoFrame &frame)
{
    AkIpcBridgeLogMethod();

    return false;
}

void AkVCam::IpcBridge::setMirroring(const std::string &deviceId,
                                     bool horizontalMirrored,
                                     bool verticalMirrored)
{
    AkIpcBridgeLogMethod();
}

void AkVCam::IpcBridge::setScaling(const std::string &deviceId,
                                   Scaling scaling)
{
    AkIpcBridgeLogMethod();
}

void AkVCam::IpcBridge::setAspectRatio(const std::string &deviceId,
                                       AspectRatio aspectRatio)
{
    AkIpcBridgeLogMethod();
}

void AkVCam::IpcBridge::setListenersChangedCallback(ListenersChangedCallback callback)
{
}

bool AkVCam::IpcBridge::addListener(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    return false;
}

bool AkVCam::IpcBridge::removeListener(const std::string &deviceId)
{
    AkIpcBridgeLogMethod();

    return true;
}

void AkVCam::IpcBridge::setFrameReadyCallback(FrameReadyCallback callback)
{
}

void AkVCam::IpcBridge::setDeviceAddedCallback(DeviceChangedCallback callback)
{
}

void AkVCam::IpcBridge::setDeviceRemovedCallback(DeviceChangedCallback callback)
{
}

void AkVCam::IpcBridge::setBroadcastingChangedCallback(BroadcastingChangedCallback callback)
{
}

void AkVCam::IpcBridge::setMirrorChangedCallback(AkVCam::IpcBridge::MirrorChangedCallback callback)
{
}

void AkVCam::IpcBridge::setScalingChangedCallback(AkVCam::IpcBridge::ScalingChangedCallback callback)
{
}

void AkVCam::IpcBridge::setAspectRatioChangedCallback(AkVCam::IpcBridge::AspectRatioChangedCallback callback)
{
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

    return this->isVirtualCamera(baseFilter.get());
}

bool AkVCam::IpcBridgePrivate::isVirtualCamera(IBaseFilter *baseFilter) const
{
    CLSID clsid;
    memset(&clsid, 0, sizeof(CLSID));
    baseFilter->GetClassID(&clsid);

    return !cameraFromClsid(clsid).empty();
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
