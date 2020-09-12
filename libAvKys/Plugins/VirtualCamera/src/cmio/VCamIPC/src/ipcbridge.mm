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
    return {};
}

void AkVCam::IpcBridge::setDriverPaths(const std::vector<std::wstring> &driverPaths)
{
    Q_UNUSED(driverPaths)
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
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return false;

    QProcess proc;
    proc.start(manager,
               {"-p",
                "get-control",
                QString::fromStdString(deviceId),
                "hflip"});
    proc.waitForFinished();

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg).toStdWString();
        }

        return false;
    }

    return proc.readAllStandardOutput().trimmed() != "0";
}

bool AkVCam::IpcBridge::isVerticalMirrored(const std::string &deviceId)
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return false;

    QProcess proc;
    proc.start(manager,
               {"-p",
                "get-control",
                QString::fromStdString(deviceId),
                "vflip"});
    proc.waitForFinished();

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg).toStdWString();
        }

        return false;
    }

    return proc.readAllStandardOutput().trimmed() != "0";
}

AkVCam::Scaling AkVCam::IpcBridge::scalingMode(const std::string &deviceId)
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return ScalingFast;

    QProcess proc;
    proc.start(manager,
               {"-p",
                "get-control",
                QString::fromStdString(deviceId),
                "scaling"});
    proc.waitForFinished();

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg).toStdWString();
        }

        return ScalingFast;
    }

    return Scaling(proc.readAllStandardOutput().trimmed().toInt());
}

AkVCam::AspectRatio AkVCam::IpcBridge::aspectRatioMode(const std::string &deviceId)
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return AspectRatioIgnore;

    QProcess proc;
    proc.start(manager,
               {"-p",
                "get-control",
                QString::fromStdString(deviceId),
                "aspect_ratio"});
    proc.waitForFinished();

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg).toStdWString();
        }

        return AspectRatioIgnore;
    }

    return AspectRatio(proc.readAllStandardOutput().trimmed().toInt());
}

bool AkVCam::IpcBridge::swapRgb(const std::string &deviceId)
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return false;

    QProcess proc;
    proc.start(manager,
               {"-p",
                "get-control",
                QString::fromStdString(deviceId),
                "swap_rgb"});
    proc.waitForFinished();

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg).toStdWString();
        }

        return false;
    }

    return proc.readAllStandardOutput().trimmed() != "0";
}

std::vector<std::string> AkVCam::IpcBridge::listeners(const std::string &deviceId)
{
    Q_UNUSED(deviceId)

    return {};
}

std::vector<uint64_t> AkVCam::IpcBridge::clientsPids() const
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return {};

    QProcess proc;
    proc.start(manager, {"-p", "clients"});
    proc.waitForFinished();

    if (proc.exitCode())
        return {};

    std::vector<uint64_t> pids;

    for (auto &line: proc.readAllStandardOutput().split('\n')) {
        auto pidExe = line.simplified().split(' ');
        auto pid = pidExe.value(0).toInt();

        if (pid != getpid())
            pids.push_back(pid);
    }

    return pids;
}

std::string AkVCam::IpcBridge::clientExe(uint64_t pid) const
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return {};

    QProcess proc;
    proc.start(manager, {"-p", "clients"});
    proc.waitForFinished();

    if (proc.exitCode())
        return {};

    for (auto &line: proc.readAllStandardOutput().split('\n')) {
        auto pidExe = line.simplified().split(' ');

        if (pidExe.value(0).toULongLong() == pid)
            return pidExe.value(1).toStdString();
    }

    return {};
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
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return {};

    QProcess proc;
    proc.start(manager,
               {"-p",
                "add-device",
                QString::fromStdWString(description)});
    proc.waitForFinished();
    auto result = proc.exitCode();
    QString deviceId;

    if (!result) {
        deviceId = QString::fromUtf8(proc.readAllStandardOutput());
        deviceId = deviceId.trimmed();
    }

    if (!result) {
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
                proc.start(manager,
                           {"add-format",
                            deviceId,
                            QString::fromStdString(pixFormat),
                            QString::number(width),
                            QString::number(height),
                            QString::fromStdString(fps.toString())});
                proc.waitForFinished();

                if (proc.exitCode()) {
                    result = proc.exitCode();

                    break;
                }

                outputformats.push_back(format);
            }
        }
    }

    if (!result) {
        proc.start(manager, {"update"});
        proc.waitForFinished();
        result = proc.exitCode();
    }

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg).toStdWString();
        }

        return {};
    }

    return deviceId.toStdString();
}

bool AkVCam::IpcBridge::deviceEdit(const std::string &deviceId,
                                   const std::wstring &description,
                                   const std::vector<VideoFormat> &formats)
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return {};

    QProcess proc;
    proc.start(manager,
               {"set-description",
                QString::fromStdString(deviceId),
                QString::fromStdWString(description)});
    proc.waitForFinished();
    auto result = proc.exitCode();

    if (!result) {
        proc.start(manager,
                   {"remove-formats",
                    QString::fromStdString(deviceId)});
        proc.waitForFinished();
        result = proc.exitCode();
    }

    if (!result) {
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
                proc.start(manager,
                           {"add-format",
                            QString::fromStdString(deviceId),
                            QString::fromStdString(pixFormat),
                            QString::number(width),
                            QString::number(height),
                            QString::fromStdString(fps.toString())});
                proc.waitForFinished();

                if (proc.exitCode()) {
                    result = proc.exitCode();

                    break;
                }

                outputformats.push_back(format);
            }
        }
    }

    if (!result) {
        proc.start(manager, {"update"});
        proc.waitForFinished();
        result = proc.exitCode();
    }

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

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
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return false;

    QProcess proc;
    proc.start(manager,
               {"set-description",
                QString::fromStdString(deviceId),
                QString::fromStdWString(description)});
    proc.waitForFinished();

    if (!proc.exitCode()) {
        proc.start(manager, {"update"});
        proc.waitForFinished();
    }

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

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
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return false;

    QProcess proc;
    proc.start(manager, {"remove-device", QString::fromStdString(deviceId)});
    proc.waitForFinished();

    if (!proc.exitCode()) {
        proc.start(manager, {"update"});
        proc.waitForFinished();
    }

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

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
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return false;

    QProcess proc;
    proc.start(manager, {"remove-devices"});
    proc.waitForFinished();

    if (!proc.exitCode()) {
        proc.start(manager, {"update"});
        proc.waitForFinished();
    }

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

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
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return false;

    this->d->m_curFormat = format;
    auto pixFormat =
            QString::fromStdString(VideoFormat::stringFromFourcc(format.fourcc()));
    QString params;
    QTextStream paramsStream(&params);
    paramsStream << manager
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
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return;

    QProcess proc;
    proc.start(manager,
               {"set-controls",
                QString::fromStdString(deviceId),
                QString("hflip=%1").arg(horizontalMirrored),
                QString("vflip=%1").arg(verticalMirrored)});
    proc.waitForFinished();

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg).toStdWString();
        }
    }
}

void AkVCam::IpcBridge::setScaling(const std::string &deviceId,
                                   Scaling scaling)
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return;

    QProcess proc;
    proc.start(manager,
               {"set-controls",
                QString::fromStdString(deviceId),
                QString("scaling=%1").arg(scaling)});
    proc.waitForFinished();

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg).toStdWString();
        }
    }
}

void AkVCam::IpcBridge::setAspectRatio(const std::string &deviceId,
                                       AspectRatio aspectRatio)
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return;

    QProcess proc;
    proc.start(manager,
               {"set-controls",
                QString::fromStdString(deviceId),
                QString("aspect_ratio=%1").arg(aspectRatio)});
    proc.waitForFinished();

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg).toStdWString();
        }
    }
}

void AkVCam::IpcBridge::setSwapRgb(const std::string &deviceId, bool swap)
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return;

    QProcess proc;
    proc.start(manager,
               {"set-controls",
                QString::fromStdString(deviceId),
                QString("swap_rgb=%1").arg(swap)});
    proc.waitForFinished();

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg).toStdWString();
        }
    }
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
    auto path = this->locateDriverPath();

    if (path.isEmpty())
        return {};

    return path + "/Contents/MacOS/" CMIO_PLUGIN_NAME;
}

QString AkVCam::IpcBridgePrivate::manager() const
{
    auto path = this->locateDriverPath();

    if (path.isEmpty())
        return {};

    return path + "/Contents/Resources/" CMIO_PLUGIN_MANAGER_NAME;
}

QStringList AkVCam::IpcBridgePrivate::devices() const
{
    auto manager = this->manager();

    if (manager.isEmpty())
        return {};

    QProcess proc;
    proc.start(manager, {"-p", "devices"});
    proc.waitForFinished();

    if (proc.exitCode() != 0)
        return {};

    QStringList devices;

    for (auto &line: proc.readAllStandardOutput().split('\n'))
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
    QStringList pluginFiles {
        "/Contents/MacOS/" CMIO_PLUGIN_NAME,
        "/Contents/Resources/" CMIO_PLUGIN_ASSISTANT_NAME,
        "/Contents/Resources/" CMIO_PLUGIN_MANAGER_NAME,
    };

    QString pluginPath = CMIO_PLUGINS_DAL_PATH "/" CMIO_PLUGIN_NAME ".plugin";

    for (auto &file: pluginFiles)
        if (!QFileInfo::exists(pluginPath + file))
            return {};

    return QDir(pluginPath).canonicalPath();
}
