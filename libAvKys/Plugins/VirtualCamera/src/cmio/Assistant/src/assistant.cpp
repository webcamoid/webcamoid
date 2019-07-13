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

#include <map>
#include <sstream>
#include <CoreFoundation/CoreFoundation.h>
#include <xpc/xpc.h>
#include <xpc/connection.h>

#include "assistant.h"
#include "assistantglobals.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/image/videoframe.h"
#include "VCamUtils/src/logger/logger.h"

#define AkAssistantLogMethod() \
    AkLoggerLog("Assistant::", __FUNCTION__, "()")

#define AkAssistantPrivateLogMethod() \
    AkLoggerLog("ServicePrivate::", __FUNCTION__, "()")

#define AKVCAM_BIND_FUNC(member) \
    std::bind(&member, this, std::placeholders::_1, std::placeholders::_2)

#define PREFERENCES_ID CFSTR(AKVCAM_ASSISTANT_NAME)

namespace AkVCam
{
    struct AssistantDevice
    {
        std::wstring description;
        std::vector<VideoFormat> formats;
        std::string broadcaster;
        std::vector<std::string> listeners;
        bool horizontalMirror {false};
        bool verticalMirror {false};
        Scaling scaling {ScalingFast};
        AspectRatio aspectRatio {AspectRatioIgnore};
        bool swapRgb {false};
    };

    using AssistantPeers = std::map<std::string, xpc_connection_t>;
    using DeviceConfigs = std::map<std::string, AssistantDevice>;

    class AssistantPrivate
    {
        public:
            AssistantPeers m_servers;
            AssistantPeers m_clients;
            DeviceConfigs m_deviceConfigs;
            std::map<int64_t, XpcMessage> m_messageHandlers;
            CFRunLoopTimerRef m_timer {nullptr};
            double m_timeout {0.0};

            AssistantPrivate();
            ~AssistantPrivate();
            inline static uint64_t id();
            bool startTimer();
            void stopTimer();
            static void timerTimeout(CFRunLoopTimerRef timer, void *info);
            std::shared_ptr<CFTypeRef> cfTypeFromStd(const std::string &str) const;
            std::shared_ptr<CFTypeRef> cfTypeFromStd(const std::wstring &str) const;
            std::shared_ptr<CFTypeRef> cfTypeFromStd(int num) const;
            std::shared_ptr<CFTypeRef> cfTypeFromStd(double num) const;
            std::string stringFromCFType(CFTypeRef cfType) const;
            std::wstring wstringFromCFType(CFTypeRef cfType) const;
            std::vector<std::string> preferencesKeys() const;
            void preferencesWrite(const std::string &key,
                                  const std::shared_ptr<CFTypeRef> &value) const;
            void preferencesWrite(const std::string &key,
                                  const std::string &value) const;
            void preferencesWrite(const std::string &key,
                                  const std::wstring &value) const;
            void preferencesWrite(const std::string &key, int value) const;
            void preferencesWrite(const std::string &key, double value) const;
            std::shared_ptr<CFTypeRef> preferencesRead(const std::string &key) const;
            std::string preferencesReadString(const std::string &key) const;
            std::wstring preferencesReadWString(const std::string &key) const;
            int preferencesReadInt(const std::string &key) const;
            double preferencesReadDouble(const std::string &key) const;
            void preferencesDelete(const std::string &key) const;
            void preferencesDeleteAll(const std::string &key) const;
            void preferencesMove(const std::string &keyFrom,
                                 const std::string &keyTo) const;
            void preferencesMoveAll(const std::string &keyFrom,
                                    const std::string &keyTo) const;
            void preferencesSync() const;
            std::string preferencesAddCamera(const std::wstring &description,
                                             const std::vector<VideoFormat> &formats);
            std::string preferencesAddCamera(const std::string &path,
                                             const std::wstring &description,
                                             const std::vector<VideoFormat> &formats);
            void preferencesRemoveCamera(const std::string &path);
            size_t camerasCount() const;
            std::string createDevicePath() const;
            int cameraFromPath(const std::string &path) const;
            bool cameraExists(const std::string &path) const;
            std::wstring cameraDescription(size_t cameraIndex) const;
            std::string cameraPath(size_t cameraIndex) const;
            size_t formatsCount(size_t cameraIndex) const;
            VideoFormat cameraFormat(size_t cameraIndex, size_t formatIndex) const;
            std::vector<VideoFormat> cameraFormats(size_t cameraIndex) const;
            void loadCameras();
            void releaseDevicesFromPeer(const std::string &portName);
            void peerDied();
            void requestPort(xpc_connection_t client, xpc_object_t event);
            void addPort(xpc_connection_t client, xpc_object_t event);
            void removePortByName(const std::string &portName);
            void removePort(xpc_connection_t client, xpc_object_t event);
            void deviceCreate(xpc_connection_t client, xpc_object_t event);
            void deviceDestroyById(const std::string &deviceId);
            void deviceDestroy(xpc_connection_t client, xpc_object_t event);
            void setBroadcasting(xpc_connection_t client, xpc_object_t event);
            void setMirroring(xpc_connection_t client, xpc_object_t event);
            void setScaling(xpc_connection_t client, xpc_object_t event);
            void setAspectRatio(xpc_connection_t client, xpc_object_t event);
            void setSwapRgb(xpc_connection_t client, xpc_object_t event);
            void frameReady(xpc_connection_t client, xpc_object_t event);
            void listeners(xpc_connection_t client, xpc_object_t event);
            void listener(xpc_connection_t client, xpc_object_t event);
            void devices(xpc_connection_t client, xpc_object_t event);
            void description(xpc_connection_t client, xpc_object_t event);
            void formats(xpc_connection_t client, xpc_object_t event);
            void broadcasting(xpc_connection_t client, xpc_object_t event);
            void mirroring(xpc_connection_t client, xpc_object_t event);
            void scaling(xpc_connection_t client, xpc_object_t event);
            void aspectRatio(xpc_connection_t client, xpc_object_t event);
            void swapRgb(xpc_connection_t client, xpc_object_t event);
            void listenerAdd(xpc_connection_t client, xpc_object_t event);
            void listenerRemove(xpc_connection_t client, xpc_object_t event);
    };
}

AkVCam::Assistant::Assistant()
{
    this->d = new AssistantPrivate;
}

AkVCam::Assistant::~Assistant()
{
    delete this->d;
}

void AkVCam::Assistant::setTimeout(double timeout)
{
    this->d->m_timeout = timeout;
}

void AkVCam::Assistant::messageReceived(xpc_connection_t client,
                                        xpc_object_t event)
{
    AkAssistantLogMethod();
    auto type = xpc_get_type(event);

    if (type == XPC_TYPE_ERROR) {
        if (event == XPC_ERROR_CONNECTION_INVALID) {
            this->d->peerDied();
        } else {
            auto description = xpc_copy_description(event);
            AkLoggerLog("ERROR: ", description);
            free(description);
        }
    } else if (type == XPC_TYPE_DICTIONARY) {
        int64_t message = xpc_dictionary_get_int64(event, "message");

        if (this->d->m_messageHandlers.count(message))
            this->d->m_messageHandlers[message](client, event);
    }
}

AkVCam::AssistantPrivate::AssistantPrivate()
{
    this->m_messageHandlers = {
        {AKVCAM_ASSISTANT_MSG_FRAME_READY           , AKVCAM_BIND_FUNC(AssistantPrivate::frameReady)     },
        {AKVCAM_ASSISTANT_MSG_REQUEST_PORT          , AKVCAM_BIND_FUNC(AssistantPrivate::requestPort)    },
        {AKVCAM_ASSISTANT_MSG_ADD_PORT              , AKVCAM_BIND_FUNC(AssistantPrivate::addPort)        },
        {AKVCAM_ASSISTANT_MSG_REMOVE_PORT           , AKVCAM_BIND_FUNC(AssistantPrivate::removePort)     },
        {AKVCAM_ASSISTANT_MSG_DEVICE_CREATE         , AKVCAM_BIND_FUNC(AssistantPrivate::deviceCreate)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_DESTROY        , AKVCAM_BIND_FUNC(AssistantPrivate::deviceDestroy)  },
        {AKVCAM_ASSISTANT_MSG_DEVICES               , AKVCAM_BIND_FUNC(AssistantPrivate::devices)        },
        {AKVCAM_ASSISTANT_MSG_DEVICE_DESCRIPTION    , AKVCAM_BIND_FUNC(AssistantPrivate::description)    },
        {AKVCAM_ASSISTANT_MSG_DEVICE_FORMATS        , AKVCAM_BIND_FUNC(AssistantPrivate::formats)        },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_ADD   , AKVCAM_BIND_FUNC(AssistantPrivate::listenerAdd)    },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_REMOVE, AKVCAM_BIND_FUNC(AssistantPrivate::listenerRemove) },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENERS      , AKVCAM_BIND_FUNC(AssistantPrivate::listeners)      },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER       , AKVCAM_BIND_FUNC(AssistantPrivate::listener)       },
        {AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING   , AKVCAM_BIND_FUNC(AssistantPrivate::broadcasting)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING, AKVCAM_BIND_FUNC(AssistantPrivate::setBroadcasting)},
        {AKVCAM_ASSISTANT_MSG_DEVICE_MIRRORING      , AKVCAM_BIND_FUNC(AssistantPrivate::mirroring)      },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETMIRRORING   , AKVCAM_BIND_FUNC(AssistantPrivate::setMirroring)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SCALING        , AKVCAM_BIND_FUNC(AssistantPrivate::scaling)        },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETSCALING     , AKVCAM_BIND_FUNC(AssistantPrivate::setScaling)     },
        {AKVCAM_ASSISTANT_MSG_DEVICE_ASPECTRATIO    , AKVCAM_BIND_FUNC(AssistantPrivate::aspectRatio)    },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETASPECTRATIO , AKVCAM_BIND_FUNC(AssistantPrivate::setAspectRatio) },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SWAPRGB        , AKVCAM_BIND_FUNC(AssistantPrivate::swapRgb)        },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETSWAPRGB     , AKVCAM_BIND_FUNC(AssistantPrivate::setSwapRgb)     },
    };

    this->loadCameras();
    this->startTimer();
}

AkVCam::AssistantPrivate::~AssistantPrivate()
{
    std::vector<AssistantPeers *> allPeers {
        &this->m_clients,
        &this->m_servers
    };

    for (auto &device: this->m_deviceConfigs) {
        auto notification = xpc_dictionary_create(NULL, NULL, 0);
        xpc_dictionary_set_int64(notification, "message", AKVCAM_ASSISTANT_MSG_DEVICE_DESTROY);
        xpc_dictionary_set_string(notification, "device", device.first.c_str());

        for (auto peers: allPeers)
            for (auto &peer: *peers)
                xpc_connection_send_message(peer.second, notification);

        xpc_release(notification);
    }
}

uint64_t AkVCam::AssistantPrivate::id()
{
    static uint64_t id = 0;

    return id++;
}

bool AkVCam::AssistantPrivate::startTimer()
{
    AkAssistantPrivateLogMethod();

    if (this->m_timer || this->m_timeout <= 0.0)
        return false;

    // If no peer has been connected for 5 minutes shutdown the assistant.
    CFRunLoopTimerContext context {0, this, nullptr, nullptr, nullptr};
    this->m_timer =
            CFRunLoopTimerCreate(kCFAllocatorDefault,
                                 CFAbsoluteTimeGetCurrent() + this->m_timeout,
                                 0,
                                 0,
                                 0,
                                 AssistantPrivate::timerTimeout,
                                 &context);

    if (!this->m_timer)
        return false;

    CFRunLoopAddTimer(CFRunLoopGetMain(),
                      this->m_timer,
                      kCFRunLoopCommonModes);

    return true;
}

void AkVCam::AssistantPrivate::stopTimer()
{
    AkAssistantPrivateLogMethod();

    if (!this->m_timer)
        return;

    CFRunLoopTimerInvalidate(this->m_timer);
    CFRunLoopRemoveTimer(CFRunLoopGetMain(),
                         this->m_timer,
                         kCFRunLoopCommonModes);
    CFRelease(this->m_timer);
    this->m_timer = nullptr;
}

void AkVCam::AssistantPrivate::timerTimeout(CFRunLoopTimerRef timer, void *info)
{
    UNUSED(timer)
    UNUSED(info)
    AkAssistantPrivateLogMethod();

    CFRunLoopStop(CFRunLoopGetMain());
}

std::shared_ptr<CFTypeRef> AkVCam::AssistantPrivate::cfTypeFromStd(const std::string &str) const
{
    auto ref =
            new CFTypeRef(CFStringCreateWithCString(kCFAllocatorDefault,
                                                    str.c_str(),
                                                    kCFStringEncodingUTF8));

    return std::shared_ptr<CFTypeRef>(ref, [] (CFTypeRef *ptr) {
        CFRelease(*ptr);
        delete ptr;
    });
}

std::shared_ptr<CFTypeRef> AkVCam::AssistantPrivate::cfTypeFromStd(const std::wstring &str) const
{
    auto ref =
            new CFTypeRef(CFStringCreateWithBytes(kCFAllocatorDefault,
                                                  reinterpret_cast<const UInt8 *>(str.c_str()),
                                                  CFIndex(str.size() * sizeof(wchar_t)),
                                                  kCFStringEncodingUTF32LE,
                                                  false));

    return std::shared_ptr<CFTypeRef>(ref, [] (CFTypeRef *ptr) {
        CFRelease(*ptr);
        delete ptr;
    });
}

std::shared_ptr<CFTypeRef> AkVCam::AssistantPrivate::cfTypeFromStd(int num) const
{
    auto ref =
            new CFTypeRef(CFNumberCreate(kCFAllocatorDefault,
                                         kCFNumberIntType,
                                         &num));

    return std::shared_ptr<CFTypeRef>(ref, [] (CFTypeRef *ptr) {
        CFRelease(*ptr);
        delete ptr;
    });
}

std::shared_ptr<CFTypeRef> AkVCam::AssistantPrivate::cfTypeFromStd(double num) const
{
    auto ref =
            new CFTypeRef(CFNumberCreate(kCFAllocatorDefault,
                                         kCFNumberDoubleType,
                                         &num));

    return std::shared_ptr<CFTypeRef>(ref, [] (CFTypeRef *ptr) {
        CFRelease(*ptr);
        delete ptr;
    });
}

std::string AkVCam::AssistantPrivate::stringFromCFType(CFTypeRef cfType) const
{
    auto len = size_t(CFStringGetLength(CFStringRef(cfType)));
    auto data = CFStringGetCStringPtr(CFStringRef(cfType), kCFStringEncodingUTF8);

    if (data)
        return std::string(data, len);

    auto cstr = new char[len];
    CFStringGetCString(CFStringRef(cfType), cstr, CFIndex(len), kCFStringEncodingUTF8);
    std::string str(cstr, len);
    delete [] cstr;

    return str;
}

std::wstring AkVCam::AssistantPrivate::wstringFromCFType(CFTypeRef cfType) const
{
    auto len = CFStringGetLength(CFStringRef(cfType));
    auto range = CFRangeMake(0, len);
    CFIndex bufferLen = 0;
    auto converted = CFStringGetBytes(CFStringRef(cfType),
                                      range,
                                      kCFStringEncodingUTF32LE,
                                      0,
                                      false,
                                      nullptr,
                                      0,
                                      &bufferLen);

    if (converted < 1 || bufferLen < 1)
        return {};

    wchar_t cstr[bufferLen];

    converted = CFStringGetBytes(CFStringRef(cfType),
                                 range,
                                 kCFStringEncodingUTF32LE,
                                 0,
                                 false,
                                 reinterpret_cast<UInt8 *>(cstr),
                                 bufferLen,
                                 nullptr);

    if (converted < 1)
        return {};

    return std::wstring(cstr, size_t(len));
}

std::vector<std::string> AkVCam::AssistantPrivate::preferencesKeys() const
{
    AkAssistantPrivateLogMethod();
    std::vector<std::string> keys;

    auto cfKeys = CFPreferencesCopyKeyList(PREFERENCES_ID,
                                           kCFPreferencesCurrentUser,
                                           kCFPreferencesAnyHost);

    if (cfKeys) {
        auto size = CFArrayGetCount(cfKeys);

        for (CFIndex i = 0; i < size; i++) {
            auto key = CFStringRef(CFArrayGetValueAtIndex(cfKeys, i));
            keys.push_back(this->stringFromCFType(key));
        }

        CFRelease(cfKeys);
    }

#ifdef QT_DEBUG
    AkLoggerLog("Keys: ", keys.size());
    std::sort(keys.begin(), keys.end());

    for (auto &key: keys)
        AkLoggerLog("    ", key);
#endif

    return keys;
}

void AkVCam::AssistantPrivate::preferencesWrite(const std::string &key,
                                                const std::shared_ptr<CFTypeRef> &value) const
{
    AkAssistantPrivateLogMethod();
    AkLoggerLog("Writing: ", key, " = ", *value);
    auto cfKey = cfTypeFromStd(key);
    CFPreferencesSetAppValue(CFStringRef(*cfKey), *value, PREFERENCES_ID);
}

void AkVCam::AssistantPrivate::preferencesWrite(const std::string &key,
                                                const std::string &value) const
{
    AkAssistantPrivateLogMethod();
    AkLoggerLog("Writing: ", key, " = ", value);
    auto cfKey = cfTypeFromStd(key);
    auto cfValue = cfTypeFromStd(value);
    CFPreferencesSetAppValue(CFStringRef(*cfKey), *cfValue, PREFERENCES_ID);
}

void AkVCam::AssistantPrivate::preferencesWrite(const std::string &key,
                                                const std::wstring &value) const
{
    AkAssistantPrivateLogMethod();
    AkLoggerLog("Writing: ", key, " = ", std::string(value.begin(),
                                                     value.end()));
    auto cfKey = cfTypeFromStd(key);
    auto cfValue = cfTypeFromStd(value);
    CFPreferencesSetAppValue(CFStringRef(*cfKey), *cfValue, PREFERENCES_ID);
}

void AkVCam::AssistantPrivate::preferencesWrite(const std::string &key,
                                                int value) const
{
    AkAssistantPrivateLogMethod();
    AkLoggerLog("Writing: ", key, " = ", value);
    auto cfKey = cfTypeFromStd(key);
    auto cfValue = cfTypeFromStd(value);
    CFPreferencesSetAppValue(CFStringRef(*cfKey), *cfValue, PREFERENCES_ID);
}

void AkVCam::AssistantPrivate::preferencesWrite(const std::string &key,
                                                double value) const
{
    AkAssistantPrivateLogMethod();
    AkLoggerLog("Writing: ", key, " = ", value);
    auto cfKey = cfTypeFromStd(key);
    auto cfValue = cfTypeFromStd(value);
    CFPreferencesSetAppValue(CFStringRef(*cfKey), *cfValue, PREFERENCES_ID);
}

std::shared_ptr<CFTypeRef> AkVCam::AssistantPrivate::preferencesRead(const std::string &key) const
{
    AkAssistantPrivateLogMethod();
    auto cfKey = cfTypeFromStd(key);
    auto cfValue = CFTypeRef(CFPreferencesCopyAppValue(CFStringRef(*cfKey),
                                                       PREFERENCES_ID));

    if (!cfValue)
        return {};

    return std::shared_ptr<CFTypeRef>(new CFTypeRef(cfValue),
                                      [] (CFTypeRef *ptr) {
                                          CFRelease(*ptr);
                                          delete ptr;
                                      });
}

std::string AkVCam::AssistantPrivate::preferencesReadString(const std::string &key) const
{
    AkAssistantPrivateLogMethod();
    auto cfKey = cfTypeFromStd(key);
    auto cfValue =
            CFStringRef(CFPreferencesCopyAppValue(CFStringRef(*cfKey),
                                                  PREFERENCES_ID));
    std::string value;

    if (cfValue) {
        value = this->stringFromCFType(cfValue);
        CFRelease(cfValue);
    }

    return value;
}

std::wstring AkVCam::AssistantPrivate::preferencesReadWString(const std::string &key) const
{
    AkAssistantPrivateLogMethod();
    auto cfKey = cfTypeFromStd(key);
    auto cfValue =
            CFStringRef(CFPreferencesCopyAppValue(CFStringRef(*cfKey),
                                                  PREFERENCES_ID));
    std::wstring value;

    if (cfValue) {
        value = this->wstringFromCFType(cfValue);
        CFRelease(cfValue);
    }

    return value;
}

int AkVCam::AssistantPrivate::preferencesReadInt(const std::string &key) const
{
    AkAssistantPrivateLogMethod();
    auto cfKey = cfTypeFromStd(key);
    auto cfValue =
            CFNumberRef(CFPreferencesCopyAppValue(CFStringRef(*cfKey),
                                                  PREFERENCES_ID));
    int value = 0;

    if (cfValue) {
        CFNumberGetValue(cfValue, kCFNumberIntType, &value);
        CFRelease(cfValue);
    }

    return value;
}

double AkVCam::AssistantPrivate::preferencesReadDouble(const std::string &key) const
{
    AkAssistantPrivateLogMethod();
    auto cfKey = cfTypeFromStd(key);
    auto cfValue =
            CFNumberRef(CFPreferencesCopyAppValue(CFStringRef(*cfKey),
                                                  PREFERENCES_ID));
    double value = 0;

    if (cfValue) {
        CFNumberGetValue(cfValue, kCFNumberDoubleType, &value);
        CFRelease(cfValue);
    }

    return value;
}

void AkVCam::AssistantPrivate::preferencesDelete(const std::string &key) const
{
    AkAssistantPrivateLogMethod();
    AkLoggerLog("Deleting ", key);
    auto cfKey = cfTypeFromStd(key);
    CFPreferencesSetAppValue(CFStringRef(*cfKey), nullptr, PREFERENCES_ID);
}

void AkVCam::AssistantPrivate::preferencesDeleteAll(const std::string &key) const
{
    AkAssistantPrivateLogMethod();
    AkLoggerLog("Key: ", key);

    for (auto &key_: this->preferencesKeys())
        if (key_.size() >= key.size() && key_.substr(0, key.size()) == key)
            this->preferencesDelete(key_);
}

void AkVCam::AssistantPrivate::preferencesMove(const std::string &keyFrom,
                                               const std::string &keyTo) const
{
    AkAssistantPrivateLogMethod();
    AkLoggerLog("From: ", keyFrom);
    AkLoggerLog("To: ", keyTo);
    auto value = this->preferencesRead(keyFrom);

    if (!value)
        return;

    this->preferencesWrite(keyTo, value);
    this->preferencesDelete(keyFrom);
}

void AkVCam::AssistantPrivate::preferencesMoveAll(const std::string &keyFrom,
                                                  const std::string &keyTo) const
{
    AkAssistantPrivateLogMethod();
    AkLoggerLog("From: ", keyFrom);
    AkLoggerLog("to: ", keyTo);

    for (auto &key: this->preferencesKeys())
        if (key.size() >= keyFrom.size()
            && key.substr(0, keyFrom.size()) == keyFrom) {
            if (key.size() == keyFrom.size())
                this->preferencesMove(key, keyTo);
            else
                this->preferencesMove(key, keyTo + key.substr(keyFrom.size()));
        }
}

void AkVCam::AssistantPrivate::preferencesSync() const
{
    AkAssistantPrivateLogMethod();
    CFPreferencesAppSynchronize(PREFERENCES_ID);
}

std::string AkVCam::AssistantPrivate::preferencesAddCamera(const std::wstring &description,
                                                           const std::vector<VideoFormat> &formats)
{
    return this->preferencesAddCamera("", description, formats);
}

std::string AkVCam::AssistantPrivate::preferencesAddCamera(const std::string &path,
                                                           const std::wstring &description,
                                                           const std::vector<VideoFormat> &formats)
{
    AkAssistantPrivateLogMethod();

    if (!path.empty() && this->cameraExists(path))
        return {};

    auto path_ = path.empty()? this->createDevicePath(): path;
    int cameraIndex = this->preferencesReadInt("cameras");
    this->preferencesWrite("cameras", cameraIndex + 1);

    this->preferencesWrite("cameras."
                           + std::to_string(cameraIndex)
                           + ".description",
                           description);
    this->preferencesWrite("cameras."
                           + std::to_string(cameraIndex)
                           + ".path",
                           path_);
    this->preferencesWrite("cameras."
                           + std::to_string(cameraIndex)
                           + ".formats",
                           int(formats.size()));

    for (size_t i = 0; i < formats.size(); i++) {
        auto &format = formats[i];
        auto prefix = "cameras."
                    + std::to_string(cameraIndex)
                    + ".formats."
                    + std::to_string(i);
        auto formatStr = VideoFormat::stringFromFourcc(format.fourcc());
        this->preferencesWrite(prefix + ".format", formatStr);
        this->preferencesWrite(prefix + ".width", format.width());
        this->preferencesWrite(prefix + ".height", format.height());
        this->preferencesWrite(prefix + ".fps", format.minimumFrameRate().toString());
    }

    this->preferencesSync();

    return path_;
}

void AkVCam::AssistantPrivate::preferencesRemoveCamera(const std::string &path)
{
    AkAssistantPrivateLogMethod();
    AkLoggerLog("Device: ", path);
    int cameraIndex = this->cameraFromPath(path);

    if (cameraIndex < 0)
        return;

    auto nCameras = this->camerasCount();
    this->preferencesDeleteAll("cameras." + std::to_string(cameraIndex));

    for (auto i = size_t(cameraIndex + 1); i < nCameras; i++)
        this->preferencesMoveAll("cameras." + std::to_string(i),
                                 "cameras." + std::to_string(i - 1));

    if (nCameras > 1)
        this->preferencesWrite("cameras", int(nCameras - 1));
    else
        this->preferencesDelete("cameras");

    this->preferencesSync();
}

size_t AkVCam::AssistantPrivate::camerasCount() const
{
    AkAssistantPrivateLogMethod();
    int nCameras = this->preferencesReadInt("cameras");
    AkLoggerLog("Cameras: ", nCameras);

    return size_t(nCameras);
}

std::string AkVCam::AssistantPrivate::createDevicePath() const
{
    AkAssistantPrivateLogMethod();

    // List device paths in use.
    std::vector<std::string> cameraPaths;

    for (size_t i = 0; i < this->camerasCount(); i++)
        cameraPaths.push_back(this->cameraPath(i));

    const int maxId = 64;

    for (int i = 0; i < maxId; i++) {
        /* There are no rules for device paths in Windows. Just append an
         * incremental index to a common prefix.
         */
        auto path = CMIO_PLUGIN_DEVICE_PREFIX + std::to_string(i);

        // Check if the path is being used, if not return it.
        if (std::find(cameraPaths.begin(),
                      cameraPaths.end(),
                      path) == cameraPaths.end())
            return path;
    }

    return {};
}

int AkVCam::AssistantPrivate::cameraFromPath(const std::string &path) const
{
    for (size_t i = 0; i < this->camerasCount(); i++)
        if (this->cameraPath(i) == path && !this->cameraFormats(i).empty())
            return int(i);

    return -1;
}

bool AkVCam::AssistantPrivate::cameraExists(const std::string &path) const
{
    for (size_t i = 0; i < this->camerasCount(); i++)
        if (this->cameraPath(i) == path)
            return true;

    return false;
}

std::wstring AkVCam::AssistantPrivate::cameraDescription(size_t cameraIndex) const
{
    return this->preferencesReadWString("cameras."
                                        + std::to_string(cameraIndex)
                                        + ".description");
}

std::string AkVCam::AssistantPrivate::cameraPath(size_t cameraIndex) const
{
    return this->preferencesReadString("cameras."
                                       + std::to_string(cameraIndex)
                                       + ".path");
}

size_t AkVCam::AssistantPrivate::formatsCount(size_t cameraIndex) const
{
    return size_t(this->preferencesReadInt("cameras."
                                           + std::to_string(cameraIndex)
                                           + ".formats"));
}

AkVCam::VideoFormat AkVCam::AssistantPrivate::cameraFormat(size_t cameraIndex,
                                                           size_t formatIndex) const
{
    AkAssistantPrivateLogMethod();
    auto prefix = "cameras."
                + std::to_string(cameraIndex)
                + ".formats."
                + std::to_string(formatIndex);
    auto format = this->preferencesReadString(prefix + ".format");
    auto fourcc = VideoFormat::fourccFromString(format);
    int width = this->preferencesReadInt(prefix + ".width");
    int height = this->preferencesReadInt(prefix + ".height");
    auto fps = Fraction(this->preferencesReadString(prefix + ".fps"));

    return VideoFormat(fourcc, width, height, {fps});
}

std::vector<AkVCam::VideoFormat> AkVCam::AssistantPrivate::cameraFormats(size_t cameraIndex) const
{
    AkAssistantPrivateLogMethod();
    std::vector<AkVCam::VideoFormat> formats;

    for (size_t i = 0; i < this->formatsCount(cameraIndex); i++) {
        auto videoFormat = this->cameraFormat(cameraIndex, i);

        if (videoFormat)
            formats.push_back(videoFormat);
    }

    return formats;
}

void AkVCam::AssistantPrivate::loadCameras()
{
    AkAssistantPrivateLogMethod();

    for (size_t i = 0; i < this->camerasCount(); i++) {
        this->m_deviceConfigs[this->cameraPath(i)] = {};
        this->m_deviceConfigs[this->cameraPath(i)].description = this->cameraDescription(i);
        this->m_deviceConfigs[this->cameraPath(i)].formats = this->cameraFormats(i);
    }
}

void AkVCam::AssistantPrivate::releaseDevicesFromPeer(const std::string &portName)
{
    AkAssistantPrivateLogMethod();

    for (auto &config: this->m_deviceConfigs)
        if (config.second.broadcaster == portName) {
            config.second.broadcaster.clear();

            auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
            xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING);
            xpc_dictionary_set_string(dictionary, "device", config.first.c_str());
            xpc_dictionary_set_string(dictionary, "broadcaster", "");

            for (auto &client: this->m_clients) {
                auto reply =
                        xpc_connection_send_message_with_reply_sync(client.second,
                                                                    dictionary);
                xpc_release(reply);
            }

            xpc_release(dictionary);
        } else {
            auto it = std::find(config.second.listeners.begin(),
                                config.second.listeners.end(),
                                portName);

            if (it != config.second.listeners.end())
                config.second.listeners.erase(it);
        }
}

void AkVCam::AssistantPrivate::peerDied()
{
    AkAssistantPrivateLogMethod();

    std::vector<AssistantPeers *> allPeers {
        &this->m_clients,
        &this->m_servers
    };

    for (auto peers: allPeers) {
        for (auto &peer: *peers) {
            auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
            xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_ISALIVE);
            auto reply = xpc_connection_send_message_with_reply_sync(peer.second,
                                                                     dictionary);
            xpc_release(dictionary);
            auto replyType = xpc_get_type(reply);
            bool alive = false;

            if (replyType == XPC_TYPE_DICTIONARY)
                alive = xpc_dictionary_get_bool(reply, "alive");

            xpc_release(reply);

            if (!alive)
                this->removePortByName(peer.first);
        }
    }
}

void AkVCam::AssistantPrivate::requestPort(xpc_connection_t client,
                                           xpc_object_t event)
{
    AkAssistantPrivateLogMethod();

    bool asClient = xpc_dictionary_get_bool(event, "client");
    std::string portName = asClient?
                AKVCAM_ASSISTANT_CLIENT_NAME:
                AKVCAM_ASSISTANT_SERVER_NAME;
    portName += std::to_string(this->id());

    AkLoggerLog("Returning Port: ", portName);

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_string(reply, "port", portName.c_str());
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::addPort(xpc_connection_t client,
                                       xpc_object_t event)
{
    AkAssistantPrivateLogMethod();

    std::string portName = xpc_dictionary_get_string(event, "port");
    auto endpoint = xpc_dictionary_get_value(event, "connection");
    auto connection = xpc_connection_create_from_endpoint(reinterpret_cast<xpc_endpoint_t>(endpoint));
    xpc_connection_set_event_handler(connection, ^(xpc_object_t) {});
    xpc_connection_resume(connection);
    bool ok = true;
    AssistantPeers *peers;

    if (portName.find(AKVCAM_ASSISTANT_CLIENT_NAME) != std::string::npos)
        peers = &this->m_clients;
    else
        peers = &this->m_servers;

    for (auto &peer: *peers)
        if (peer.first == portName) {
            ok = false;

            break;
        }

    if (ok) {
        AkLoggerLog("Adding Peer: ", portName);
        (*peers)[portName] = connection;
        this->stopTimer();
    }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::removePortByName(const std::string &portName)
{
    AkAssistantPrivateLogMethod();
    AkLoggerLog("Port: ", portName);

    std::vector<AssistantPeers *> allPeers {
        &this->m_clients,
        &this->m_servers
    };

    bool breakLoop = false;

    for (auto peers: allPeers) {
        for (auto &peer: *peers)
            if (peer.first == portName) {
                xpc_release(peer.second);
                peers->erase(portName);
                breakLoop = true;

                break;
            }

        if (breakLoop)
            break;
    }

    bool peersEmpty = this->m_servers.empty() && this->m_clients.empty();

    if (peersEmpty)
        this->startTimer();

    this->releaseDevicesFromPeer(portName);
}

void AkVCam::AssistantPrivate::removePort(xpc_connection_t client,
                                          xpc_object_t event)
{
    UNUSED(client)
    AkAssistantPrivateLogMethod();

    this->removePortByName(xpc_dictionary_get_string(event, "port"));
}

void AkVCam::AssistantPrivate::deviceCreate(xpc_connection_t client,
                                            xpc_object_t event)
{
    AkAssistantPrivateLogMethod();
    std::string portName = xpc_dictionary_get_string(event, "port");
    AkLoggerLog("Port Name: ", portName);
    size_t len = 0;
    auto data = reinterpret_cast<const wchar_t *>(xpc_dictionary_get_data(event,
                                                                          "description",
                                                                          &len));
    std::wstring description(data, len / sizeof(wchar_t));
    auto formatsArray = xpc_dictionary_get_array(event, "formats");
    std::vector<VideoFormat> formats;

    for (size_t i = 0; i < xpc_array_get_count(formatsArray); i++) {
        auto format = xpc_array_get_dictionary(formatsArray, i);
        auto fourcc = FourCC(xpc_dictionary_get_uint64(format, "fourcc"));
        auto width = int(xpc_dictionary_get_int64(format, "width"));
        auto height = int(xpc_dictionary_get_int64(format, "height"));
        auto frameRate = Fraction(xpc_dictionary_get_string(format, "fps"));
        formats.push_back(VideoFormat {fourcc, width, height, {frameRate}});
    }

    auto deviceId = this->preferencesAddCamera(description, formats);
    this->m_deviceConfigs[deviceId] = {};
    this->m_deviceConfigs[deviceId].description = description;
    this->m_deviceConfigs[deviceId].formats = formats;

    auto notification = xpc_copy(event);
    xpc_dictionary_set_string(notification, "device", deviceId.c_str());

    for (auto &client: this->m_clients)
        xpc_connection_send_message(client.second, notification);

    xpc_release(notification);
    AkLoggerLog("Device created: ", deviceId);

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_string(reply, "device", deviceId.c_str());
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::deviceDestroyById(const std::string &deviceId)
{
    AkAssistantPrivateLogMethod();
    auto it = this->m_deviceConfigs.find(deviceId);

    if (it != this->m_deviceConfigs.end()) {
        this->m_deviceConfigs.erase(it);

        auto notification = xpc_dictionary_create(nullptr, nullptr, 0);
        xpc_dictionary_set_int64(notification, "message", AKVCAM_ASSISTANT_MSG_DEVICE_DESTROY);
        xpc_dictionary_set_string(notification, "device", deviceId.c_str());

        for (auto &client: this->m_clients)
            xpc_connection_send_message(client.second, notification);

        xpc_release(notification);
    }
}

void AkVCam::AssistantPrivate::deviceDestroy(xpc_connection_t client,
                                             xpc_object_t event)
{
    UNUSED(client)
    AkAssistantPrivateLogMethod();

    std::string deviceId = xpc_dictionary_get_string(event, "device");
    this->deviceDestroyById(deviceId);
    this->preferencesRemoveCamera(deviceId);
}

void AkVCam::AssistantPrivate::setBroadcasting(xpc_connection_t client,
                                               xpc_object_t event)
{
    AkAssistantPrivateLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    std::string broadcaster = xpc_dictionary_get_string(event, "broadcaster");
    bool ok = false;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        if (this->m_deviceConfigs[deviceId].broadcaster != broadcaster) {
            AkLoggerLog("Device: ", deviceId);
            AkLoggerLog("Broadcaster: ", broadcaster);
            this->m_deviceConfigs[deviceId].broadcaster = broadcaster;
            auto notification = xpc_copy(event);

            for (auto &client: this->m_clients)
                xpc_connection_send_message(client.second, notification);

            xpc_release(notification);
            ok = true;
        }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::setMirroring(xpc_connection_t client,
                                            xpc_object_t event)
{
    AkAssistantPrivateLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    bool horizontalMirror = xpc_dictionary_get_bool(event, "hmirror");
    bool verticalMirror = xpc_dictionary_get_bool(event, "vmirror");
    bool ok = false;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        if (this->m_deviceConfigs[deviceId].horizontalMirror != horizontalMirror
            || this->m_deviceConfigs[deviceId].verticalMirror != verticalMirror) {
            this->m_deviceConfigs[deviceId].horizontalMirror = horizontalMirror;
            this->m_deviceConfigs[deviceId].verticalMirror = verticalMirror;
            auto notification = xpc_copy(event);

            for (auto &client: this->m_clients)
                xpc_connection_send_message(client.second, notification);

            xpc_release(notification);
            ok = true;
        }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::setScaling(xpc_connection_t client,
                                          xpc_object_t event)
{
    AkAssistantPrivateLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    auto scaling = Scaling(xpc_dictionary_get_int64(event, "scaling"));
    bool ok = false;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        if (this->m_deviceConfigs[deviceId].scaling != scaling) {
            this->m_deviceConfigs[deviceId].scaling = scaling;
            auto notification = xpc_copy(event);

            for (auto &client: this->m_clients)
                xpc_connection_send_message(client.second, notification);

            xpc_release(notification);
            ok = true;
        }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::setAspectRatio(xpc_connection_t client,
                                              xpc_object_t event)
{
    AkAssistantPrivateLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    auto aspectRatio = AspectRatio(xpc_dictionary_get_int64(event, "aspect"));
    bool ok = false;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        if (this->m_deviceConfigs[deviceId].aspectRatio != aspectRatio) {
            this->m_deviceConfigs[deviceId].aspectRatio = aspectRatio;
            auto notification = xpc_copy(event);

            for (auto &client: this->m_clients)
                xpc_connection_send_message(client.second, notification);

            xpc_release(notification);
            ok = true;
        }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::setSwapRgb(xpc_connection_t client,
                                          xpc_object_t event)
{
    AkAssistantPrivateLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    auto swapRgb = xpc_dictionary_get_bool(event, "swap");
    bool ok = false;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        if (this->m_deviceConfigs[deviceId].swapRgb != swapRgb) {
            this->m_deviceConfigs[deviceId].swapRgb = swapRgb;
            auto notification = xpc_copy(event);

            for (auto &client: this->m_clients)
                xpc_connection_send_message(client.second, notification);

            xpc_release(notification);
            ok = true;
        }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::frameReady(xpc_connection_t client,
                                          xpc_object_t event)
{
    UNUSED(client)
    AkAssistantPrivateLogMethod();

    for (auto &client: this->m_clients)
        xpc_connection_send_message(client.second, event);
}

void AkVCam::AssistantPrivate::listeners(xpc_connection_t client,
                                         xpc_object_t event)
{
    AkAssistantPrivateLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    auto listeners = xpc_array_create(nullptr, 0);

    if (this->m_deviceConfigs.count(deviceId) > 0)
        for (auto &listener: this->m_deviceConfigs[deviceId].listeners) {
            auto listenerObj = xpc_string_create(listener.c_str());
            xpc_array_append_value(listeners, listenerObj);
        }

    AkLoggerLog("Device: ", deviceId);
    AkLoggerLog("Listeners: ", xpc_array_get_count(listeners));
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_value(reply, "listeners", listeners);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::listener(xpc_connection_t client,
                                        xpc_object_t event)
{
    AkAssistantPrivateLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    auto index = xpc_dictionary_get_uint64(event, "index");
    std::string listener;
    bool ok = false;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        if (index < this->m_deviceConfigs[deviceId].listeners.size()) {
            listener = this->m_deviceConfigs[deviceId].listeners[index];
            ok = true;
        }

    AkLoggerLog("Device: ", deviceId);
    AkLoggerLog("Listener: ", listener);
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_string(reply, "listener", listener.c_str());
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::devices(xpc_connection_t client,
                                       xpc_object_t event)
{
    AkAssistantPrivateLogMethod();
    auto devices = xpc_array_create(nullptr, 0);

    for (auto &device: this->m_deviceConfigs) {
        auto deviceObj = xpc_string_create(device.first.c_str());
        xpc_array_append_value(devices, deviceObj);
    }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_value(reply, "devices", devices);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::description(xpc_connection_t client,
                                           xpc_object_t event)
{
    AkAssistantPrivateLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    std::wstring description;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        description = this->m_deviceConfigs[deviceId].description;

    AkLoggerLog("Description for device ", deviceId, ": ",
                std::string(description.begin(), description.end()));
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_data(reply,
                            "description",
                            description.c_str(),
                            description.size() * sizeof(wchar_t));
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::formats(xpc_connection_t client,
                                       xpc_object_t event)
{
    AkAssistantPrivateLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    auto formats = xpc_array_create(nullptr, 0);

    if (this->m_deviceConfigs.count(deviceId) > 0)
        for (auto &format: this->m_deviceConfigs[deviceId].formats) {
            auto dictFormat = xpc_dictionary_create(nullptr, nullptr, 0);
            xpc_dictionary_set_uint64(dictFormat, "fourcc", format.fourcc());
            xpc_dictionary_set_int64(dictFormat, "width", format.width());
            xpc_dictionary_set_int64(dictFormat, "height", format.height());
            xpc_dictionary_set_string(dictFormat, "fps", format.minimumFrameRate().toString().c_str());
            xpc_array_append_value(formats, dictFormat);
        }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_value(reply, "formats", formats);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::broadcasting(xpc_connection_t client,
                                            xpc_object_t event)
{
    AkAssistantPrivateLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    std::string broadcaster;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        broadcaster = this->m_deviceConfigs[deviceId].broadcaster;

    AkLoggerLog("Device: ", deviceId);
    AkLoggerLog("Broadcaster: ", broadcaster);
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_string(reply, "broadcaster", broadcaster.c_str());
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::mirroring(xpc_connection_t client,
                                         xpc_object_t event)
{
    AkAssistantPrivateLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    bool horizontalMirror = false;
    bool verticalMirror = false;

    if (this->m_deviceConfigs.count(deviceId) > 0) {
        horizontalMirror = this->m_deviceConfigs[deviceId].horizontalMirror;
        verticalMirror = this->m_deviceConfigs[deviceId].verticalMirror;
    }

    AkLoggerLog("Device: ", deviceId);
    AkLoggerLog("Horizontal mirror: ", horizontalMirror);
    AkLoggerLog("Vertical mirror: ", verticalMirror);
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "hmirror", horizontalMirror);
    xpc_dictionary_set_bool(reply, "vmirror", verticalMirror);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::scaling(xpc_connection_t client, xpc_object_t event)
{
    AkAssistantPrivateLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    Scaling scaling = ScalingFast;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        scaling = this->m_deviceConfigs[deviceId].scaling;

    AkLoggerLog("Device: ", deviceId);
    AkLoggerLog("Scaling: ", scaling);
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_int64(reply, "scaling", scaling);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::aspectRatio(xpc_connection_t client,
                                           xpc_object_t event)
{
    AkAssistantPrivateLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    AspectRatio aspectRatio = AspectRatioIgnore;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        aspectRatio = this->m_deviceConfigs[deviceId].aspectRatio;

    AkLoggerLog("Device: ", deviceId);
    AkLoggerLog("Aspect ratio: ", aspectRatio);
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_int64(reply, "aspect", aspectRatio);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::swapRgb(xpc_connection_t client,
                                       xpc_object_t event)
{
    AkAssistantPrivateLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    bool swapRgb = false;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        swapRgb = this->m_deviceConfigs[deviceId].swapRgb;

    AkLoggerLog("Device: ", deviceId);
    AkLoggerLog("Swap RGB: ", swapRgb);
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "swap", swapRgb);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::listenerAdd(xpc_connection_t client,
                                           xpc_object_t event)
{
    AkAssistantPrivateLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    std::string listener = xpc_dictionary_get_string(event, "listener");
    bool ok = false;

    if (this->m_deviceConfigs.count(deviceId) > 0) {
        auto &listeners = this->m_deviceConfigs[deviceId].listeners;
        auto it = std::find(listeners.begin(), listeners.end(), listener);

        if (it == listeners.end()) {
            listeners.push_back(listener);
            auto notification = xpc_copy(event);

            for (auto &client: this->m_clients)
                xpc_connection_send_message(client.second, notification);

            xpc_release(notification);
            ok = true;
        }
    }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::listenerRemove(xpc_connection_t client,
                                              xpc_object_t event)
{
    AkAssistantPrivateLogMethod();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    std::string listener = xpc_dictionary_get_string(event, "listener");
    bool ok = false;

    if (this->m_deviceConfigs.count(deviceId) > 0) {
        auto &listeners = this->m_deviceConfigs[deviceId].listeners;
        auto it = std::find(listeners.begin(), listeners.end(), listener);

        if (it != listeners.end()) {
            listeners.erase(it);
            auto notification = xpc_copy(event);

            for (auto &client: this->m_clients)
                xpc_connection_send_message(client.second, notification);

            xpc_release(notification);
            ok = true;
        }
    }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}
