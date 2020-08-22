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

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QVariant>
#include <QWaitCondition>
#include <QMutex>
#include <QProcess>
#include <QSettings>
#include <QTemporaryDir>
#include <sys/time.h>
#include <libproc.h>
#import <AVFoundation/AVFoundation.h>

#include "ipcbridge.h"
#include "deviceobserver.h"
#include "VCamUtils/src/utils.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/image/videoframe.h"

namespace AkVCam
{
    using PixelFormatToFourCCMap = QMap<FourCC, FourCharCode>;
    using FormatsList = QList<VideoFormat>;

    enum StreamDirection
    {
        StreamDirectionOutput,
        StreamDirectionInput
    };

    class IpcBridgePrivate
    {
        public:
            IpcBridge *self;
            ::id m_deviceObserver {nil};
            QStringList m_driverPaths {CMIO_PLUGINS_DAL_PATH};
            QStringList m_devices;
            QMap<QString, QString> m_descriptions;
            QMap<QString, FormatsList> m_devicesFormats;
            std::vector<std::string> m_broadcasting;
            std::map<std::string, std::string> m_options;
            FILE *m_managerProc {nullptr};
            VideoFormat m_curFormat;
            std::wstring m_error;

            IpcBridgePrivate(IpcBridge *self=nullptr);
            ~IpcBridgePrivate();

            static bool canUseCamera();
            static inline const PixelFormatToFourCCMap &formatToFourCCMap();
            QStringList listDrivers();
            QString plugin() const;
            QString manager() const;
            QStringList devices() const;
            void updateDevices();

            // Utility methods
            QString locateDriverPath() const;
    };

    Q_GLOBAL_STATIC(QStringList, globalDriverPaths)
}

AkVCam::IpcBridge::IpcBridge()
{
    this->d = new IpcBridgePrivate(this);
    this->d->m_deviceObserver = [[DeviceObserverVCamCMIO alloc]
                                 initWithCaptureObject: this];

    [[NSNotificationCenter defaultCenter]
     addObserver: this->d->m_deviceObserver
     selector: @selector(cameraConnected:)
     name: AVCaptureDeviceWasConnectedNotification
     object: nil];

    [[NSNotificationCenter defaultCenter]
     addObserver: this->d->m_deviceObserver
     selector: @selector(cameraDisconnected:)
     name: AVCaptureDeviceWasDisconnectedNotification
     object: nil];

    this->d->updateDevices();
}

AkVCam::IpcBridge::~IpcBridge()
{
    [[NSNotificationCenter defaultCenter]
     removeObserver: this->d->m_deviceObserver];

    [this->d->m_deviceObserver disconnect];
    [this->d->m_deviceObserver release];

    delete this->d;
}

std::wstring AkVCam::IpcBridge::errorMessage() const
{
    return this->d->m_error;
}

void AkVCam::IpcBridge::setOption(const std::string &key,
                                  const std::string &value)
{
    if (value.empty())
        this->d->m_options.erase(key);
    else
        this->d->m_options[key] = value;
}

std::vector<std::wstring> AkVCam::IpcBridge::driverPaths() const
{
    std::vector<std::wstring> paths;

    for (auto &path: *AkVCam::globalDriverPaths)
        paths.push_back(path.toStdWString());

    return paths;
}

void AkVCam::IpcBridge::setDriverPaths(const std::vector<std::wstring> &driverPaths)
{
    QStringList paths;

    for (auto &path: driverPaths)
        paths << QString::fromStdWString(path);

    *AkVCam::globalDriverPaths = paths;
}

std::vector<std::string> AkVCam::IpcBridge::availableDrivers() const
{
    std::vector<std::string> drivers;

    for (auto &driver: this->d->listDrivers())
        drivers.push_back(driver.toStdString());

    return drivers;
}

std::string AkVCam::IpcBridge::driver() const
{
    auto drivers = this->availableDrivers();

    if (drivers.empty())
        return {};

    QSettings settings(QCoreApplication::organizationName(),
                       "VirtualCamera");
    auto driver =
            settings.value("driver", "AkVirtualCamera").toString().toStdString();

    if (std::find(drivers.begin(), drivers.end(), driver) == drivers.end())
        return drivers.front();

    return driver;
}

bool AkVCam::IpcBridge::setDriver(const std::string &driver)
{
    auto drivers = this->availableDrivers();

    if (std::find(drivers.begin(), drivers.end(), driver) == drivers.end())
        return false;

    QSettings settings(QCoreApplication::organizationName(), "VirtualCamera");
    settings.setValue("driver", QString::fromStdString(driver));

    return true;
}

std::vector<std::string> AkVCam::IpcBridge::availableRootMethods() const
{
    auto paths =
            QProcessEnvironment::systemEnvironment().value("PATH").split(':');

    static const QStringList sus {
        "osascript"
    };

    std::vector<std::string> methods;

    for (auto &su: sus)
        for (auto &path: paths)
            if (QDir(path).exists(su)) {
                methods.push_back(su.toStdString());

                break;
            }

    return methods;
}

std::string AkVCam::IpcBridge::rootMethod() const
{
    auto methods = this->availableRootMethods();

    if (methods.empty())
        return {};

    QSettings settings(QCoreApplication::organizationName(),
                       "VirtualCamera");
    auto method =
            settings.value("rootMethod", "osascript").toString().toStdString();

    if (std::find(methods.begin(), methods.end(), method) == methods.end())
        return methods.front();

    return method;
}

bool AkVCam::IpcBridge::setRootMethod(const std::string &rootMethod)
{
    auto methods = this->availableRootMethods();

    if (std::find(methods.begin(), methods.end(), rootMethod) == methods.end())
        return false;

    QSettings settings(QCoreApplication::organizationName(), "VirtualCamera");
    settings.setValue("rootMethod", QString::fromStdString(rootMethod));

    return true;
}

void AkVCam::IpcBridge::connectService(bool asClient)
{
    Q_UNUSED(asClient)
}

void AkVCam::IpcBridge::disconnectService()
{
}

bool AkVCam::IpcBridge::registerPeer(bool asClient)
{
    Q_UNUSED(asClient)

    return true;
}

void AkVCam::IpcBridge::unregisterPeer()
{
}

std::vector<std::string> AkVCam::IpcBridge::listDevices() const
{
    std::vector<std::string> devices;

    for (auto &device: this->d->m_devices)
        devices.push_back(device.toStdString());

    return devices;
}

std::wstring AkVCam::IpcBridge::description(const std::string &deviceId) const
{
    return this->d->m_descriptions.value(QString::fromStdString(deviceId)).toStdWString();
}

std::vector<AkVCam::PixelFormat> AkVCam::IpcBridge::supportedOutputPixelFormats() const
{
    return {
        PixelFormatRGB32,
        PixelFormatRGB24,
        PixelFormatUYVY,
        PixelFormatYUY2
    };
}

AkVCam::PixelFormat AkVCam::IpcBridge::defaultOutputPixelFormat() const
{
    return PixelFormatYUY2;
}

std::vector<AkVCam::VideoFormat> AkVCam::IpcBridge::formats(const std::string &deviceId) const
{
    auto device = QString::fromStdString(deviceId);

    if (!this->d->m_devicesFormats.contains(device))
        return {};

    std::vector<VideoFormat> formats;

    for (auto &format: this->d->m_devicesFormats[device])
        formats.push_back(format);

    return formats;
}

std::string AkVCam::IpcBridge::broadcaster(const std::string &deviceId) const
{
    Q_UNUSED(deviceId)

    return {};
}

bool AkVCam::IpcBridge::isHorizontalMirrored(const std::string &deviceId)
{/*
    if (!this->d->m_serverMessagePort)
        return false;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_MIRRORING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return false;
    }

    bool horizontalMirror = xpc_dictionary_get_bool(reply, "hmirror");
    xpc_release(reply);

    return horizontalMirror;
    */
    return false;
}

bool AkVCam::IpcBridge::isVerticalMirrored(const std::string &deviceId)
{/*
    if (!this->d->m_serverMessagePort)
        return false;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_MIRRORING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return false;
    }

    bool verticalMirror = xpc_dictionary_get_bool(reply, "vmirror");
    xpc_release(reply);

    return verticalMirror;
    */
    return false;
}

AkVCam::Scaling AkVCam::IpcBridge::scalingMode(const std::string &deviceId)
{/*
    if (!this->d->m_serverMessagePort)
        return ScalingFast;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SCALING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return ScalingFast;
    }

    auto scaling = Scaling(xpc_dictionary_get_int64(reply, "scaling"));
    xpc_release(reply);

    return scaling;
    */
    return ScalingFast;
}

AkVCam::AspectRatio AkVCam::IpcBridge::aspectRatioMode(const std::string &deviceId)
{/*
    if (!this->d->m_serverMessagePort)
        return AspectRatioIgnore;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_ASPECTRATIO);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return AspectRatioIgnore;
    }

    auto aspectRatio = AspectRatio(xpc_dictionary_get_int64(reply, "aspect"));
    xpc_release(reply);

    return aspectRatio;
    */
    return AspectRatioIgnore;
}

bool AkVCam::IpcBridge::swapRgb(const std::string &deviceId)
{/*
    if (!this->d->m_serverMessagePort)
        return false;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SWAPRGB);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return false;
    }

    auto swap = xpc_dictionary_get_bool(reply, "swap");
    xpc_release(reply);

    return swap;
    */
    return false;
}

std::vector<std::string> AkVCam::IpcBridge::listeners(const std::string &deviceId)
{
    Q_UNUSED(deviceId)

    return {};
}

std::vector<uint64_t> AkVCam::IpcBridge::clientsPids() const
{
    auto driverPath = this->d->locateDriverPath();

    if (driverPath.isEmpty())
        return {};

    auto plugin = QFileInfo(driverPath).fileName();
    QString pluginPath =
            CMIO_PLUGINS_DAL_PATH "/"
            + plugin
            + "/Contents/MacOS/" CMIO_PLUGIN_NAME;
    auto npids = proc_listpidspath(PROC_ALL_PIDS,
                                   0,
                                   pluginPath.toStdString().c_str(),
                                   0,
                                   nullptr,
                                   0);
    pid_t pidsvec[npids];
    memset(pidsvec, 0, npids * sizeof(pid_t));
    proc_listpidspath(PROC_ALL_PIDS,
                      0,
                      pluginPath.toStdString().c_str(),
                      0,
                      pidsvec,
                      npids * sizeof(pid_t));
    auto currentPid = getpid();
    std::vector<uint64_t> pids;

    for (int i = 0; i < npids; i++) {
        auto it = std::find(pids.begin(), pids.end(), pidsvec[i]);

        if (pidsvec[i] > 0 && it == pids.end() && pidsvec[i] != currentPid)
            pids.push_back(pidsvec[i]);
    }

    return pids;
}

std::string AkVCam::IpcBridge::clientExe(uint64_t pid) const
{
    char path[4096];
    memset(path, 0, 4096);
    proc_pidpath(pid, path, 4096);

    return {path};
}

bool AkVCam::IpcBridge::needsRestart(Operation operation) const
{
    Q_UNUSED(operation)

    return false;
}

bool AkVCam::IpcBridge::canApply(AkVCam::IpcBridge::Operation operation) const
{
    Q_UNUSED(operation)

    return true;
}

std::string AkVCam::IpcBridge::deviceCreate(const std::wstring &description,
                                            const std::vector<VideoFormat> &formats)
{
    // Write the script file.
    QTemporaryDir tempDir;
    QFile cmds(tempDir.path() + "/akvcam_exec.sh");

    if (!cmds.open(QIODevice::WriteOnly | QIODevice::Text)) {
        this->d->m_error = L"Can't create install script";

        return {};
    }

    cmds.setPermissions(QFileDevice::ReadOwner
                        | QFileDevice::WriteOwner
                        | QFileDevice::ExeOwner
                        | QFileDevice::ReadUser
                        | QFileDevice::WriteUser
                        | QFileDevice::ExeUser);
    auto manager = this->d->manager();
    QTextStream ts(&cmds);
    ts << "#!/bin/sh" << Qt::endl;
    ts << "device=$('"
       << manager
       << "' -p add-device '"
       << QString::fromStdWString(description)
       << "')" << Qt::endl;
    ts << "[ $? != 0 ] && exit -1" << Qt::endl;

    std::vector<VideoFormat> outputformats;

    for (auto &format: formats) {
        auto width = format.width();
        auto height = format.height();
        auto fps = format.minimumFrameRate();
        auto ot = std::find(outputformats.begin(),
                            outputformats.end(),
                            format);
        auto pixFormat = VideoFormat::stringFromFourcc(format.fourcc());

        if (ot == outputformats.end() && !pixFormat.empty()) {
            ts << "'"
               << manager
               << "' add-format \"${device}\" "
               << pixFormat.c_str()
               << " "
               << width
               << " "
               << height
               << " "
               << fps.toString().c_str()
               << Qt::endl;
            ts << "[ $? != 0 ] && exit -1" << Qt::endl;
            outputformats.push_back(format);
        }
    }

    ts << "'" << manager << "' update" << Qt::endl;
    ts << "[ $? != 0 ] && exit -1" << Qt::endl;
    ts << "echo ${device}";
    cmds.close();

    QProcess sh;
    sh.start("sh", {cmds.fileName()});
    sh.waitForFinished(-1);

    if (sh.exitCode()) {
        auto errorMsg = sh.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg).toStdWString();
        }

        return {};
    }

    QStringList devices;

    for (auto &line: sh.readAllStandardOutput().trimmed().split('\n'))
        devices << line.trimmed();

    return devices.last().toStdString();
}

bool AkVCam::IpcBridge::deviceEdit(const std::string &deviceId,
                                   const std::wstring &description,
                                   const std::vector<VideoFormat> &formats)
{
    // Write the script file.
    QTemporaryDir tempDir;
    QFile cmds(tempDir.path() + "/akvcam_exec.sh");

    if (!cmds.open(QIODevice::WriteOnly | QIODevice::Text)) {
        this->d->m_error = L"Can't create install script";

        return false;
    }

    cmds.setPermissions(QFileDevice::ReadOwner
                        | QFileDevice::WriteOwner
                        | QFileDevice::ExeOwner
                        | QFileDevice::ReadUser
                        | QFileDevice::WriteUser
                        | QFileDevice::ExeUser);

    auto manager = this->d->manager();
    QTextStream ts(&cmds);
    ts << "#!/bin/sh" << Qt::endl;
    ts << "'"
       << manager
       << "' set-description '"
       << QString::fromStdString(deviceId)
       << "' '"
       << QString::fromStdWString(description)
       << "'" << Qt::endl;
    ts << "[ $? != 0 ] && exit -1" << Qt::endl;

    // Clear formats
    ts << "'"
       << manager
       << "' remove-formats '"
       << QString::fromStdString(deviceId)
       << "'"
       << Qt::endl;
    ts << "[ $? != 0 ] && exit -1" << Qt::endl;

    std::vector<VideoFormat> outputformats;

    for (auto &format: formats) {
        auto width = format.width();
        auto height = format.height();
        auto fps = format.minimumFrameRate();
        auto ot = std::find(outputformats.begin(),
                            outputformats.end(),
                            format);
        auto pixFormat = VideoFormat::stringFromFourcc(format.fourcc());

        if (ot == outputformats.end() && !pixFormat.empty()) {
            ts << "'"
               << manager
               << "' add-format '"
               << QString::fromStdString(deviceId)
               << "' "
               << pixFormat.c_str()
               << " "
               << width
               << " "
               << height
               << " "
               << fps.toString().c_str()
               << Qt::endl;
            ts << "[ $? != 0 ] && exit -1" << Qt::endl;
            outputformats.push_back(format);
        }
    }

    ts << "'" << manager << "' update" << Qt::endl;
    cmds.close();

    QProcess sh;
    sh.start("sh", {cmds.fileName()});
    sh.waitForFinished(-1);

    if (sh.exitCode()) {
        auto errorMsg = sh.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg).toStdWString();
        }

        return false;
    }

    return true;
}

bool AkVCam::IpcBridge::changeDescription(const std::string &deviceId,
                                          const std::wstring &description)
{
    // Write the script file.
    QTemporaryDir tempDir;
    QFile cmds(tempDir.path() + "/akvcam_exec.sh");

    if (!cmds.open(QIODevice::WriteOnly | QIODevice::Text)) {
        this->d->m_error = L"Can't create install script";

        return false;
    }

    cmds.setPermissions(QFileDevice::ReadOwner
                        | QFileDevice::WriteOwner
                        | QFileDevice::ExeOwner
                        | QFileDevice::ReadUser
                        | QFileDevice::WriteUser
                        | QFileDevice::ExeUser);

    auto manager = this->d->manager();
    QTextStream ts(&cmds);
    ts << "#!/bin/sh" << Qt::endl;
    ts << "'"
       << manager
       << "' set-description '"
       << QString::fromStdString(deviceId)
       << "' '"
       << QString::fromStdWString(description)
       << "'" << Qt::endl;
    ts << "[ $? != 0 ] && exit -1" << Qt::endl;
    ts << "'" << manager << "' update" << Qt::endl;
    cmds.close();

    QProcess sh;
    sh.start("sh", {cmds.fileName()});
    sh.waitForFinished(-1);

    if (sh.exitCode()) {
        auto errorMsg = sh.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg).toStdWString();
        }

        return false;
    }

    return true;
}

bool AkVCam::IpcBridge::deviceDestroy(const std::string &deviceId)
{
    // Write the script file.
    QTemporaryDir tempDir;
    QFile cmds(tempDir.path() + "/akvcam_exec.sh");

    if (!cmds.open(QIODevice::WriteOnly | QIODevice::Text)) {
        this->d->m_error = L"Can't create install script";

        return false;
    }

    cmds.setPermissions(QFileDevice::ReadOwner
                        | QFileDevice::WriteOwner
                        | QFileDevice::ExeOwner
                        | QFileDevice::ReadUser
                        | QFileDevice::WriteUser
                        | QFileDevice::ExeUser);

    auto manager = this->d->manager();
    QTextStream ts(&cmds);
    ts << "#!/bin/sh" << Qt::endl;
    ts << "'"
       << manager
       << "' remove-device '"
       << QString::fromStdString(deviceId)
       << "'" << Qt::endl;
    ts << "[ $? != 0 ] && exit -1" << Qt::endl;

    ts << "'" << manager << "' update" << Qt::endl;
    cmds.close();

    QProcess sh;
    sh.start("sh", {cmds.fileName()});
    sh.waitForFinished(-1);

    if (sh.exitCode()) {
        auto errorMsg = sh.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg).toStdWString();
        }

        return false;
    }

    return true;
}

bool AkVCam::IpcBridge::destroyAllDevices()
{
    // Write the script file.
    QTemporaryDir tempDir;
    QFile cmds(tempDir.path() + "/akvcam_exec.sh");

    if (!cmds.open(QIODevice::WriteOnly | QIODevice::Text)) {
        this->d->m_error = L"Can't create install script";

        return false;
    }

    cmds.setPermissions(QFileDevice::ReadOwner
                        | QFileDevice::WriteOwner
                        | QFileDevice::ExeOwner
                        | QFileDevice::ReadUser
                        | QFileDevice::WriteUser
                        | QFileDevice::ExeUser);

    auto manager = this->d->manager();
    QTextStream ts(&cmds);
    ts << "#!/bin/sh" << Qt::endl;
    ts << "'"
       << manager
       << "' remove-devices" << Qt::endl;
    ts << "[ $? != 0 ] && exit -1" << Qt::endl;

    ts << "'" << manager << "' update" << Qt::endl;
    cmds.close();

    QProcess sh;
    sh.start("sh", {cmds.fileName()});
    sh.waitForFinished(-1);

    if (sh.exitCode()) {
        auto errorMsg = sh.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg).toStdWString();
        }

        return false;
    }

    return true;
}

bool AkVCam::IpcBridge::deviceStart(const std::string &deviceId,
                                    const VideoFormat &format)
{
    this->d->m_curFormat = format;
    auto pixFormat =
            QString::fromStdString(VideoFormat::stringFromFourcc(format.fourcc()));
    QString params;
    QTextStream paramsStream(&params);
    paramsStream << this->d->manager()
                 << " "
                 << "stream"
                 << " "
                 << QString::fromStdString(deviceId)
                 << " "
                 << pixFormat
                 << " "
                 << format.width()
                 << " "
                 << format.height();
    this->d->m_managerProc = popen(params.toStdString().c_str(), "w");

    return this->d->m_managerProc != nullptr;
}

void AkVCam::IpcBridge::deviceStop(const std::string &deviceId)
{
    Q_UNUSED(deviceId)

    if (this->d->m_managerProc) {
        pclose(this->d->m_managerProc);
        this->d->m_managerProc = nullptr;
    }

    this->d->m_curFormat.clear();
}

bool AkVCam::IpcBridge::write(const std::string &deviceId,
                              const VideoFrame &frame)
{
    Q_UNUSED(deviceId)

    if (!this->d->m_managerProc)
        return false;

    auto scaled = frame.scaled(this->d->m_curFormat.width(),
                               this->d->m_curFormat.height())
                        .convert(this->d->m_curFormat.fourcc());

    if (!scaled.format().isValid())
        return false;

    return fwrite(scaled.data().data(),
                  scaled.data().size(),
                  1,
                  this->d->m_managerProc) > 0;
}

void AkVCam::IpcBridge::setMirroring(const std::string &deviceId,
                                     bool horizontalMirrored,
                                     bool verticalMirrored)
{/*
    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETMIRRORING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_bool(dictionary, "hmirror", horizontalMirrored);
    xpc_dictionary_set_bool(dictionary, "vmirror", verticalMirrored);
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    xpc_release(reply);*/
}

void AkVCam::IpcBridge::setScaling(const std::string &deviceId,
                                   Scaling scaling)
{/*
    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETSCALING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_int64(dictionary, "scaling", scaling);
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    xpc_release(reply);*/
}

void AkVCam::IpcBridge::setAspectRatio(const std::string &deviceId,
                                       AspectRatio aspectRatio)
{/*
    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETASPECTRATIO);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_int64(dictionary, "aspect", aspectRatio);
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    xpc_release(reply);*/
}

void AkVCam::IpcBridge::setSwapRgb(const std::string &deviceId, bool swap)
{/*
    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETSWAPRGB);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_bool(dictionary, "swap", swap);
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    xpc_release(reply);*/
}

bool AkVCam::IpcBridge::addListener(const std::string &deviceId)
{
    Q_UNUSED(deviceId)

    return true;
}

bool AkVCam::IpcBridge::removeListener(const std::string &deviceId)
{
    Q_UNUSED(deviceId)

    return true;
}

void AkVCam::IpcBridge::cameraConnected()
{
    this->d->updateDevices();
}

void AkVCam::IpcBridge::cameraDisconnected()
{
    this->d->updateDevices();
}

AkVCam::IpcBridgePrivate::IpcBridgePrivate(IpcBridge *self):
    self(self)
{
}

AkVCam::IpcBridgePrivate::~IpcBridgePrivate()
{

}

bool AkVCam::IpcBridgePrivate::canUseCamera()
{
    if (@available(macOS 10.14, *)) {
        auto status = [AVCaptureDevice authorizationStatusForMediaType: AVMediaTypeVideo];

        if (status == AVAuthorizationStatusAuthorized)
            return true;

        static bool done;
        static bool result = false;
        done = false;

        [AVCaptureDevice
         requestAccessForMediaType: AVMediaTypeVideo
         completionHandler: ^(BOOL granted) {
            done = true;
            result = granted;
        }];

        while (!done)
            qApp->processEvents();

        return result;
    }

    return true;
}

const AkVCam::PixelFormatToFourCCMap &AkVCam::IpcBridgePrivate::formatToFourCCMap()
{
    static const PixelFormatToFourCCMap fourccToStrMap {
        {AkVCam::PixelFormatRGB32, kCMPixelFormat_32ARGB         },
        {AkVCam::PixelFormatRGB24, kCMPixelFormat_24RGB          },
        {AkVCam::PixelFormatRGB16, kCMPixelFormat_16LE565        },
        {AkVCam::PixelFormatRGB15, kCMPixelFormat_16LE555        },
        {AkVCam::PixelFormatUYVY , kCMPixelFormat_422YpCbCr8     },
        {AkVCam::PixelFormatYUY2 , kCMPixelFormat_422YpCbCr8_yuvs}
    };

    return fourccToStrMap;
}

QStringList AkVCam::IpcBridgePrivate::listDrivers()
{
    auto plugin = this->plugin();

    if (plugin.isEmpty())
        return {};

    return {QFileInfo(plugin).baseName()};
}

QString AkVCam::IpcBridgePrivate::plugin() const
{
    auto paths = this->m_driverPaths + *AkVCam::globalDriverPaths;

    for (auto it = paths.rbegin(); it != paths.rend(); it++) {
        auto path = *it;
        path = path.replace("\\", "/");

        if (path.back() != '/')
            path += '/';

        path += CMIO_PLUGIN_NAME ".plugin/Contents/MacOS/" CMIO_PLUGIN_NAME;

        if (QFileInfo::exists(path))
            return path;
    }

    return {};
}

QString AkVCam::IpcBridgePrivate::manager() const
{
    auto paths = this->m_driverPaths + *AkVCam::globalDriverPaths;

    for (auto it = paths.rbegin(); it != paths.rend(); it++) {
        auto path = *it;
        path = path.replace("\\", "/");

        if (path.back() != '/')
            path += '/';

        path += CMIO_PLUGIN_NAME ".plugin/Contents/Resources/" CMIO_PLUGIN_MANAGER_NAME;

        if (QFileInfo::exists(path))
            return path;
    }

    return {};
}

QStringList AkVCam::IpcBridgePrivate::devices() const
{
    QProcess manager;
    manager.start(this->manager(), {"-p", "devices"});
    manager.waitForFinished();

    if (manager.exitCode() != 0)
        return {};

    QStringList devices;

    for (auto &line: manager.readAllStandardOutput().split('\n'))
        devices << line.trimmed();

    return devices;
}

void AkVCam::IpcBridgePrivate::updateDevices()
{
    if (!IpcBridgePrivate::canUseCamera())
        return;

    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesFormats) devicesFormats;

    auto virtualDevices = this->devices();
    auto cameras = [AVCaptureDevice devicesWithMediaType: AVMediaTypeVideo];

    for (AVCaptureDevice *camera in cameras) {
        QString deviceId = camera.uniqueID.UTF8String;
        auto it = std::find(virtualDevices.begin(),
                            virtualDevices.end(),
                            deviceId);

        if (it == virtualDevices.end())
            continue;

        FormatsList formatsList;

        // List supported frame formats.
        for (AVCaptureDeviceFormat *format in camera.formats) {
            auto fourCC = CMFormatDescriptionGetMediaSubType(format.formatDescription);
            CMVideoDimensions size =
                    CMVideoFormatDescriptionGetDimensions(format.formatDescription);
            auto &map = formatToFourCCMap();
            VideoFormat videoFormat(map.key(fourCC, 0),
                                    size.width,
                                    size.height,
                                    {{30, 1}});

            // List all supported frame rates for the format.
            for (AVFrameRateRange *fpsRange in format.videoSupportedFrameRateRanges) {
                videoFormat.frameRates() = {{qRound(1e3 * fpsRange.maxFrameRate), 1000}};
                formatsList << videoFormat;
            }
        }

        if (!formatsList.isEmpty()) {
            devices << deviceId;
            descriptions[deviceId] = camera.localizedName.UTF8String;
            devicesFormats[deviceId] = formatsList;
        }
    }

    this->m_descriptions = descriptions;
    this->m_devicesFormats = devicesFormats;
    this->m_devices = devices;
}

QString AkVCam::IpcBridgePrivate::locateDriverPath() const
{
    QString driverPath;
    QStringList pluginFiles {
        "/Contents/MacOS/" CMIO_PLUGIN_NAME,
        "/Contents/Resources/" CMIO_PLUGIN_ASSISTANT_NAME,
        "/Contents/Resources/" CMIO_PLUGIN_MANAGER_NAME,
    };

    auto paths = this->m_driverPaths + *AkVCam::globalDriverPaths;

    for (auto it = paths.rbegin(); it != paths.rend(); it++) {
        auto path = *it;
        path = path.replace("\\", "/");

        if (path.back() != '/')
            path += '/';

        path += CMIO_PLUGIN_NAME ".plugin";
        bool filesFound = true;

        for (auto &file: pluginFiles)
            if (!QFileInfo::exists(path + file)) {
                filesFound = false;

                break;
            }

        if (filesFound) {
            driverPath = path;

            break;
        }
    }

    return driverPath;
}
