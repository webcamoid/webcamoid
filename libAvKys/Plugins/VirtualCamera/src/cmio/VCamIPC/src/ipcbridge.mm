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
#import <CoreMediaIO/CMIOHardwarePlugIn.h>

#include "ipcbridge.h"
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
            QStringList m_devices;
            QMap<QString, QString> m_descriptions;
            QMap<QString, FormatsList> m_devicesFormats;
            std::vector<std::string> m_broadcasting;
            std::map<std::string, std::string> m_options;
            std::wstring m_error;

            IpcBridgePrivate(IpcBridge *self=nullptr);
            ~IpcBridgePrivate();

            static inline const PixelFormatToFourCCMap &formatToFourCCMap();
            QStringList listDrivers();
            QString plugin() const;
            QString manager() const;
            QVector<CMIODeviceID> devices() const;
            QString deviceUID(CMIODeviceID deviceID) const;
            QString objectName(CMIOObjectID objectID) const;
            QVector<CMIOStreamID> deviceStreams(CMIODeviceID deviceID) const;
            QVector<Float64> streamFrameRates(CMIOStreamID streamID,
                                              CMVideoFormatDescriptionRef formatDescription=nullptr) const;
            VideoFormat formatFromDescription(CMVideoFormatDescriptionRef formatDescription) const;
            StreamDirection streamDirection(CMIOStreamID streamID) const;
            CFArrayRef formatDescriptions(CMIOStreamID streamID) const;
            QStringList connectedDevices(const QString &deviceId) const;
            void updateDevices();

            // Utility methods
            QString locateDriverPath() const;

            // Execute commands with elevated privileges.
            int sudo(const QStringList &parameters);
    };

    Q_GLOBAL_STATIC(QStringList, driverPaths)
}

AkVCam::IpcBridge::IpcBridge()
{
    this->d = new IpcBridgePrivate(this);
    this->d->updateDevices();
}

AkVCam::IpcBridge::~IpcBridge()
{
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

    for (auto &path: *AkVCam::driverPaths)
        paths.push_back(path.toStdWString());

    return paths;
}

void AkVCam::IpcBridge::setDriverPaths(const std::vector<std::wstring> &driverPaths)
{
    QStringList paths;

    for (auto &path: driverPaths)
        paths << QString::fromStdWString(path);

    *AkVCam::driverPaths = paths;
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
    QProcess manager;
    manager.start(this->d->manager(),
                  {"-p", "broadcasters", QString::fromStdString(deviceId)});
    manager.waitForFinished();

    if (manager.exitCode() != 0)
        return {};

    QStringList broadcasters;

    for (auto &line: manager.readAllStandardOutput().split('\n'))
        broadcasters << line.trimmed();

    return broadcasters.first().toStdString();
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
{/*
    if (!this->d->m_serverMessagePort)
        return {};

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_LISTENERS);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return {};
    }

    auto listenersList = xpc_dictionary_get_array(reply, "listeners");*/
    std::vector<std::string> listeners;
/*
    for (size_t i = 0; i < xpc_array_get_count(listenersList); i++)
        listeners.push_back(xpc_array_get_string(listenersList, i));

    xpc_release(reply);
*/
    return listeners;
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
    return operation == OperationDestroyAll
            || ((operation == OperationDestroy || operation == OperationEdit)
                && this->listDevices().size() == 1);
}

bool AkVCam::IpcBridge::canApply(AkVCam::IpcBridge::Operation operation) const
{
    return this->clientsPids().empty() && !this->needsRestart(operation);
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
    ts << "inDevice=$('"
       << manager
       << "' add-device -i '"
       << QString::fromStdWString(description)
       << " (in)')"
       << Qt::endl;
    ts << "[ $? != 0 ] && exit -1" << Qt::endl;
    ts << "outDevice=$('"
       << manager
       << "' add-device -o '"
       << QString::fromStdWString(description)
       << "')" << Qt::endl;
    ts << "[ $? != 0 ] && exit -1" << Qt::endl;

    std::vector<VideoFormat> inputformats;
    std::vector<VideoFormat> outputformats;

    for (auto &format: formats) {
        auto width = format.width();
        auto height = format.height();
        auto fps = format.minimumFrameRate();
        VideoFormat inputFormat(PixelFormatRGB24,
                                width,
                                height,
                                {fps});
        auto it = std::find(inputformats.begin(),
                            inputformats.end(),
                            inputFormat);

        if (it == inputformats.end()) {
            ts << "'"
               << manager
               << "' add-format \"${inDevice}\" "
               << "RGB24"
               << " "
               << width
               << " "
               << height
               << " "
               << fps.toString().c_str()
               << Qt::endl;
            ts << "[ $? != 0 ] && exit -1" << Qt::endl;
            inputformats.push_back(inputFormat);
        }

        auto ot = std::find(outputformats.begin(),
                            outputformats.end(),
                            format);

        if (ot == outputformats.end()) {
            ts << "'"
               << manager
               << "' add-format \"${outDevice}\" "
               << VideoFormat::stringFromFourcc(format.fourcc()).c_str()
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

    ts << "'" << manager << "' connect \"${inDevice}\" \"${outDevice}\"" << Qt::endl;
    ts << "[ $? != 0 ] && exit -1" << Qt::endl;
    ts << "'" << manager << "' update" << Qt::endl;
    ts << "[ $? != 0 ] && exit -1" << Qt::endl;
    ts << "echo ${outDevice}";
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
    auto inputDevices =
        this->d->connectedDevices(QString::fromStdString(deviceId));

    auto manager = this->d->manager();
    QTextStream ts(&cmds);

    if (!inputDevices.isEmpty()) {
        ts << "'"
           << manager
           << "' set-description '"
           << inputDevices.first()
           << "' '"
           << QString::fromStdWString(description)
           << " (in)'"
           << Qt::endl;
        ts << "[ $? != 0 ] && exit -1" << Qt::endl;
    }

    ts << "'"
       << manager
       << "' set-description '"
       << QString::fromStdString(deviceId)
       << "' '"
       << QString::fromStdWString(description)
       << "'" << Qt::endl;
    ts << "[ $? != 0 ] && exit -1" << Qt::endl;

    // Clear formats
    if (!inputDevices.isEmpty()) {
        ts << "'"
           << manager
           << "' remove-formats '"
           << inputDevices.first()
           << "'"
           << Qt::endl;
        ts << "[ $? != 0 ] && exit -1" << Qt::endl;
    }

    ts << "'"
       << manager
       << "' remove-formats '"
       << QString::fromStdString(deviceId)
       << "'"
       << Qt::endl;
    ts << "[ $? != 0 ] && exit -1" << Qt::endl;

    std::vector<VideoFormat> inputformats;
    std::vector<VideoFormat> outputformats;

    for (auto &format: formats) {
        auto width = format.width();
        auto height = format.height();
        auto fps = format.minimumFrameRate();

        if (!inputDevices.isEmpty()) {
            VideoFormat inputFormat(PixelFormatRGB24,
                                    width,
                                    height,
                                    {fps});
            auto it = std::find(inputformats.begin(),
                                inputformats.end(),
                                inputFormat);

            if (it == inputformats.end()) {
                ts << "'"
                   << manager
                   << "' add-format '"
                   << inputDevices.first()
                   << "' "
                   << "RGB24"
                   << " "
                   << width
                   << " "
                   << height
                   << " "
                   << fps.toString().c_str()
                   << Qt::endl;
                ts << "[ $? != 0 ] && exit -1" << Qt::endl;
                inputformats.push_back(inputFormat);
            }
        }

        auto ot = std::find(outputformats.begin(),
                            outputformats.end(),
                            format);

        if (ot == outputformats.end()) {
            ts << "'"
               << manager
               << "' add-format '"
               << QString::fromStdString(deviceId)
               << "' "
               << VideoFormat::stringFromFourcc(format.fourcc()).c_str()
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
    auto inputDevices =
        this->d->connectedDevices(QString::fromStdString(deviceId));

    auto manager = this->d->manager();
    QTextStream ts(&cmds);

    if (!inputDevices.isEmpty()) {
        ts << "'"
           << manager
           << "' set-description '"
           << inputDevices.first()
           << "' '"
           << QString::fromStdWString(description)
           << " (in)'"
           << Qt::endl;
        ts << "[ $? != 0 ] && exit -1" << Qt::endl;
    }

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
    auto inputDevices =
        this->d->connectedDevices(QString::fromStdString(deviceId));

    auto manager = this->d->manager();
    QTextStream ts(&cmds);

    if (!inputDevices.isEmpty()) {
        ts << "'"
           << manager
           << "' remove-device '"
           << inputDevices.first()
           << "'"
           << Qt::endl;
        ts << "[ $? != 0 ] && exit -1" << Qt::endl;
    }

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
{/*
    Q_UNUSED(format);

    auto it = std::find(this->d->m_broadcasting.begin(),
                        this->d->m_broadcasting.end(),
                        deviceId);

    if (it != this->d->m_broadcasting.end())
        return false;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_string(dictionary, "broadcaster", this->d->m_portName.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return false;
    }

    bool status = xpc_dictionary_get_bool(reply, "status");
    xpc_release(reply);
    this->d->m_broadcasting.push_back(deviceId);

    return status;
    */
    return false;
}

void AkVCam::IpcBridge::deviceStop(const std::string &deviceId)
{/*
    auto it = std::find(this->d->m_broadcasting.begin(),
                        this->d->m_broadcasting.end(),
                        deviceId);

    if (it == this->d->m_broadcasting.end())
        return;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_string(dictionary, "broadcaster", "");
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    xpc_release(reply);
    this->d->m_broadcasting.erase(it);*/
}

bool AkVCam::IpcBridge::write(const std::string &deviceId,
                              const VideoFrame &frame)
{/*
    auto it = std::find(this->d->m_broadcasting.begin(),
                        this->d->m_broadcasting.end(),
                        deviceId);

    if (it == this->d->m_broadcasting.end())
        return false;

    QVector<CFStringRef> keys {
        kIOSurfacePixelFormat,
        kIOSurfaceWidth,
        kIOSurfaceHeight,
        kIOSurfaceAllocSize
    };

    auto fourcc = frame.format().fourcc();
    auto width = frame.format().width();
    auto height = frame.format().height();
    auto dataSize = int64_t(frame.data().size());

    QVector<CFNumberRef> values {
        CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &fourcc),
        CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &width),
        CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &height),
        CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &dataSize)
    };

    auto surfaceProperties =
            CFDictionaryCreate(kCFAllocatorDefault,
                               reinterpret_cast<const void **>(keys.data()),
                               reinterpret_cast<const void **>(values.data()),
                               CFIndex(values.size()),
                               nullptr,
                               nullptr);
    auto surface = IOSurfaceCreate(surfaceProperties);

    for (auto &value: values)
        CFRelease(value);

    CFRelease(surfaceProperties);

    if (!surface)
        return false;

    uint32_t surfaceSeed = 0;
    IOSurfaceLock(surface, 0, &surfaceSeed);
    auto data = IOSurfaceGetBaseAddress(surface);
    memcpy(data, frame.data().data(), frame.data().size());
    IOSurfaceUnlock(surface, 0, &surfaceSeed);
    auto surfaceObj = IOSurfaceCreateXPCObject(surface);

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_FRAME_READY);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_value(dictionary, "frame", surfaceObj);
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    xpc_release(reply);
    xpc_release(surfaceObj);
    CFRelease(surface);

    return true;
    */
    return false;
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

AkVCam::IpcBridgePrivate::IpcBridgePrivate(IpcBridge *self):
    self(self)
{
}

AkVCam::IpcBridgePrivate::~IpcBridgePrivate()
{

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
    for (auto it = AkVCam::driverPaths->rbegin();
         it != AkVCam::driverPaths->rend();
         it++) {
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
    for (auto it = AkVCam::driverPaths->rbegin();
         it != AkVCam::driverPaths->rend();
         it++) {
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

QVector<CMIODeviceID> AkVCam::IpcBridgePrivate::devices() const
{
    CMIOObjectPropertyAddress devicesProperty {
        kCMIOHardwarePropertyDevices,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 devicesSize = 0;
    auto status =
        CMIOObjectGetPropertyDataSize(kCMIOObjectSystemObject,
                                      &devicesProperty,
                                      0,
                                      nullptr,
                                      &devicesSize);

    if (status != kCMIOHardwareNoError)
        return {};

    QVector<CMIODeviceID> devices(devicesSize / sizeof(CMIODeviceID));
    status =
        CMIOObjectGetPropertyData(kCMIOObjectSystemObject,
                                  &devicesProperty,
                                  0,
                                  nullptr,
                                  devicesSize,
                                  &devicesSize,
                                  devices.data());

    return status == kCMIOHardwareNoError? devices: QVector<CMIODeviceID>();
}

QString AkVCam::IpcBridgePrivate::deviceUID(CMIODeviceID deviceID) const
{
    CMIOObjectPropertyAddress deviceUIDProperty {
        kCMIODevicePropertyDeviceUID,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 deviceUIDSize = sizeof(CFStringRef);
    CFStringRef deviceUID = nullptr;
    auto status =
        CMIOObjectGetPropertyData(deviceID,
                                  &deviceUIDProperty,
                                  0,
                                  nullptr,
                                  deviceUIDSize,
                                  &deviceUIDSize,
                                  &deviceUID);

    if (status != kCMIOHardwareNoError)
        return {};

    auto uid = QString::fromCFString(deviceUID);
    CFRelease(deviceUID);

    return uid;
}

QString AkVCam::IpcBridgePrivate::objectName(CMIOObjectID objectID) const
{
    CMIOObjectPropertyAddress objectNameProperty {
        kCMIOObjectPropertyName,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 objectNameSize = sizeof(CFStringRef);
    CFStringRef objectName = nullptr;
    auto status =
        CMIOObjectGetPropertyData(objectID,
                                  &objectNameProperty,
                                  0,
                                  nullptr,
                                  objectNameSize,
                                  &objectNameSize,
                                  &objectName);

    if (status != kCMIOHardwareNoError)
        return {};

    auto name = QString::fromCFString(objectName);
    CFRelease(objectName);

    return name;
}

QVector<CMIOStreamID> AkVCam::IpcBridgePrivate::deviceStreams(CMIODeviceID deviceID) const
{
    CMIOObjectPropertyAddress streamsProperty {
        kCMIODevicePropertyStreams,
        kCMIODevicePropertyScopeInput,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 deviceStreamsSize = 0;
    auto status =
        CMIOObjectGetPropertyDataSize(deviceID,
                                      &streamsProperty,
                                      0,
                                      nullptr,
                                      &deviceStreamsSize);

    if (status != kCMIOHardwareNoError)
        return {};

    QVector<CMIOStreamID> streams(deviceStreamsSize / sizeof(CMIOStreamID));
    status =
        CMIOObjectGetPropertyData(deviceID,
                                  &streamsProperty,
                                  0,
                                  nullptr,
                                  deviceStreamsSize,
                                  &deviceStreamsSize,
                                  streams.data());

    return status == kCMIOHardwareNoError? streams: QVector<CMIOStreamID>();
}

QVector<Float64> AkVCam::IpcBridgePrivate::streamFrameRates(CMIOStreamID streamID,
                                                            CMVideoFormatDescriptionRef formatDescription) const
{
    UInt32 formatDescriptionSize =
            formatDescription?
                sizeof(CMVideoFormatDescriptionRef):
                0;
    CMIOObjectPropertyAddress frameRatesProperty {
        kCMIOStreamPropertyFrameRates,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 frameRatesSize = 0;
    auto status =
        CMIOObjectGetPropertyDataSize(streamID,
                                      &frameRatesProperty,
                                      formatDescriptionSize,
                                      formatDescription?
                                          &formatDescription:
                                          nullptr,
                                      &frameRatesSize);

    if (status != kCMIOHardwareNoError)
        return {};

    QVector<Float64> frameRates(frameRatesSize / sizeof(Float64));
    status =
        CMIOObjectGetPropertyData(streamID,
                                  &frameRatesProperty,
                                  formatDescriptionSize,
                                  formatDescription?
                                      &formatDescription:
                                      nullptr,
                                  frameRatesSize,
                                  &frameRatesSize,
                                  frameRates.data());

    return status == kCMIOHardwareNoError? frameRates: QVector<Float64>();
}

AkVCam::VideoFormat AkVCam::IpcBridgePrivate::formatFromDescription(CMVideoFormatDescriptionRef formatDescription) const
{
    auto mediaType = CMFormatDescriptionGetMediaType(formatDescription);

    if (mediaType != kCMMediaType_Video)
        return {};

    auto fourCC = CMFormatDescriptionGetMediaSubType(formatDescription);
    auto size = CMVideoFormatDescriptionGetDimensions(formatDescription);
    auto &map = formatToFourCCMap();

    return {map.key(fourCC, 0), size.width, size.height, {{30, 1}}};
}

AkVCam::StreamDirection AkVCam::IpcBridgePrivate::streamDirection(CMIOStreamID streamID) const
{
    CMIOObjectPropertyAddress directionProperty {
        kCMIOStreamPropertyDirection,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 directionSize = sizeof(UInt32);
    UInt32 direction = 0;
    CMIOObjectGetPropertyData(streamID,
                              &directionProperty,
                              0,
                              nullptr,
                              directionSize,
                              &directionSize,
                              &direction);

    return direction? StreamDirectionInput: StreamDirectionOutput;
}

CFArrayRef AkVCam::IpcBridgePrivate::formatDescriptions(CMIOStreamID streamID) const
{
    CMIOObjectPropertyAddress formatDescriptionsProperty {
        kCMIOStreamPropertyFormatDescriptions,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 formatDescriptionsSize = sizeof(CFArrayRef);
    CFArrayRef formats = nullptr;
    CMIOObjectGetPropertyData(streamID,
                              &formatDescriptionsProperty,
                              0,
                              nullptr,
                              formatDescriptionsSize,
                              &formatDescriptionsSize,
                              &formats);

    return formats;
}

QStringList AkVCam::IpcBridgePrivate::connectedDevices(const QString &deviceId) const
{
    QProcess manager;
    manager.start(this->manager(), {"-p", "connections", deviceId});
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
    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesFormats) devicesFormats;
    QStringList virtualDevices;

    for (auto &id: this->devices()) {
        auto deviceUID = this->deviceUID(id);
        FormatsList formatsList;

        for (auto &stream: this->deviceStreams(id)) {
            if (this->streamDirection(stream) != StreamDirectionOutput)
                continue;

            auto formats = this->formatDescriptions(stream);

            for (CFIndex i = 0; i < CFArrayGetCount(formats); i++) {
                auto format =
                        reinterpret_cast<CMVideoFormatDescriptionRef>(CFArrayGetValueAtIndex(formats,
                                                                                             i));
                auto videoFormat = this->formatFromDescription(format);
                auto frameRates = this->streamFrameRates(stream, format);

                if (!videoFormat)
                    continue;

                for (auto &fpsRange: frameRates) {
                    videoFormat.frameRates() = {{qRound(1e3 * fpsRange), 1000}};
                    formatsList << videoFormat;
                }
            }

            CFRelease(formats);
        }

        if (!formatsList.isEmpty()
            && !this->connectedDevices(deviceUID).isEmpty()) {
            devices << deviceUID;
            descriptions[deviceUID] = this->objectName(id);
            devicesFormats[deviceUID] = formatsList;
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

    for (auto it = AkVCam::driverPaths->rbegin();
         it != AkVCam::driverPaths->rend();
         it++) {
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

int AkVCam::IpcBridgePrivate::sudo(const QStringList &parameters)
{
    QProcess su;
    QStringList params;

    for (auto &param: parameters) {
        if (param.contains(' '))
            params << "'" << param << "'";
        else
            params << param;
    }

    su.start("osascript",
             {"-e",
              "do shell script \""
              + params.join(' ')
              + "\" with administrator privileges"});
    su.waitForFinished(-1);

    if (su.exitCode()) {
        auto outMsg = su.readAllStandardOutput();
        this->m_error = {};

        if (!outMsg.isEmpty()) {
            qDebug() << outMsg.toStdString().c_str();
            this->m_error += QString(outMsg).toStdWString() + L" ";
        }

        auto errorMsg = su.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->m_error += QString(errorMsg).toStdWString();
        }
    }

    return su.exitCode();
}
