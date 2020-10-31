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
#include <QMutex>
#include <QProcess>
#include <QSettings>
#include <QTemporaryDir>
#include <QVariant>
#include <QWaitCondition>
#include <sys/time.h>
#include <libproc.h>
#import <AVFoundation/AVFoundation.h>
#include <akfrac.h>

#include "vcamcmio.h"
#include "deviceobserver.h"

#define CMIO_PLUGINS_DAL_PATH "/Library/CoreMediaIO/Plug-Ins/DAL"
#define CMIO_PLUGIN_NAME "AkVirtualCamera"
#define CMIO_PLUGIN_MANAGER_NAME "AkVCamManager"

struct CMIOAkFormat
{
    FourCharCode fourcc;
    AkVideoCaps::PixelFormat ak;
    QString str;
};

using CMIOAkFormatMap = QVector<CMIOAkFormat>;

class VCamCMIOPrivate
{
    public:
        VCamCMIO *self;
        ::id m_deviceObserver {nil};
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, AkVideoCapsList> m_devicesFormats;
        FILE *m_managerProc {nullptr};
        AkVideoCaps m_curFormat;
        QString m_error;

        VCamCMIOPrivate(VCamCMIO *self=nullptr);
        ~VCamCMIOPrivate();

        static bool canUseCamera();
        inline const CMIOAkFormatMap &cmioAkFormatMap() const;
        inline const CMIOAkFormat &formatByFourcc(FourCharCode fourcc) const;
        inline const CMIOAkFormat &formatByAk(AkVideoCaps::PixelFormat ak) const;
        QString manager() const;
        QStringList devices() const;
        void updateDevices();
};

VCamCMIO::VCamCMIO(QObject *parent):
    VCam(parent)
{
    this->d = new VCamCMIOPrivate(this);
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

VCamCMIO::~VCamCMIO()
{
    [[NSNotificationCenter defaultCenter]
     removeObserver: this->d->m_deviceObserver];

    [this->d->m_deviceObserver disconnect];
    [this->d->m_deviceObserver release];

    delete this->d;
}

QString VCamCMIO::error() const
{
    return this->d->m_error;
}

bool VCamCMIO::isInstalled() const
{
    return !this->d->manager().isEmpty();
}

QStringList VCamCMIO::webcams() const
{
    return this->d->m_devices;
}

QString VCamCMIO::description(const QString &deviceId) const
{
    return this->d->m_descriptions.value(deviceId);
}

QList<AkVideoCaps::PixelFormat> VCamCMIO::supportedOutputPixelFormats() const
{
    return {
        AkVideoCaps::Format_0rgb,
        AkVideoCaps::Format_rgb24,
        AkVideoCaps::Format_uyvy422,
        AkVideoCaps::Format_yuyv422,
    };
}

AkVideoCaps::PixelFormat VCamCMIO::defaultOutputPixelFormat() const
{
    return AkVideoCaps::Format_yuyv422;
}

AkVideoCapsList VCamCMIO::caps(const QString &deviceId) const
{
    if (!this->d->m_devicesFormats.contains(deviceId))
        return {};

    return this->d->m_devicesFormats[deviceId];
}

QVariantList VCamCMIO::controls() const
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return {};

    QProcess proc;
    proc.start(manager,
               {"-p",
                "controls",
                this->m_device});
    proc.waitForFinished();

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg;
            this->d->m_error += QString(errorMsg);
        }

        return {};
    }

    auto availableControls =
            QString(proc.readAllStandardOutput()).trimmed().split(QRegExp("\\s+"));
    QVariantList controls;

    for (auto &control: availableControls) {
        proc.start(manager,
                   {"-p",
                    "get-control",
                    this->m_device,
                    control});
        proc.waitForFinished();

        if (proc.exitCode())
            continue;

        auto value = proc.readAllStandardOutput().trimmed().toInt();

        proc.start(manager,
                   {"-p",
                    "get-control",
                    "-c",
                    this->m_device,
                    control});
        proc.waitForFinished();

        if (proc.exitCode())
            continue;

        auto description = QString(proc.readAllStandardOutput().trimmed());

        proc.start(manager,
                   {"-p",
                    "get-control",
                    "-t",
                    this->m_device,
                    control});
        proc.waitForFinished();

        if (proc.exitCode())
            continue;

        auto type = QString(proc.readAllStandardOutput().trimmed().toLower());

        proc.start(manager,
                   {"-p",
                    "get-control",
                    "-m",
                    this->m_device,
                    control});
        proc.waitForFinished();

        if (proc.exitCode())
            continue;

        auto minimum = proc.readAllStandardOutput().trimmed().toInt();

        proc.start(manager,
                   {"-p",
                    "get-control",
                    "-M",
                    this->m_device,
                    control});
        proc.waitForFinished();

        if (proc.exitCode())
            continue;

        auto maximum = proc.readAllStandardOutput().trimmed().toInt();

        proc.start(manager,
                   {"-p",
                    "get-control",
                    "-s",
                    this->m_device,
                    control});
        proc.waitForFinished();

        if (proc.exitCode())
            continue;

        auto step = proc.readAllStandardOutput().trimmed().toInt();

        proc.start(manager,
                   {"-p",
                    "get-control",
                    "-d",
                    this->m_device,
                    control});
        proc.waitForFinished();

        if (proc.exitCode())
            continue;

        auto defaultValue = proc.readAllStandardOutput().trimmed().toInt();

        proc.start(manager,
                   {"-p",
                    "get-control",
                    "-l",
                    this->m_device,
                    control});
        proc.waitForFinished();

        if (proc.exitCode())
            continue;

        auto menu =
                QString(proc.readAllStandardOutput()).trimmed().split(QRegExp("\\s+"));

        QVariantList controlVar {
            description,
            type,
            minimum,
            maximum,
            step,
            defaultValue,
            value,
            menu
        };
        controls << QVariant(controlVar);
    }

    return controls;
}

bool VCamCMIO::setControls(const QVariantMap &controls)
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return false;

    QProcess proc;
    proc.start(manager, {"-p", "controls", this->m_device});
    proc.waitForFinished();

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg;
            this->d->m_error += QString(errorMsg);
        }

        return false;
    }

    auto availableControls =
            QString(proc.readAllStandardOutput()).trimmed().split(QRegExp("\\s+"));
    auto result = true;
    QStringList args;

    for (auto &control: availableControls) {
        proc.start(manager,
                   {"-p",
                    "get-control",
                    "-c",
                    this->m_device,
                    control});
        proc.waitForFinished();

        if (proc.exitCode()) {
            result = false;

            continue;
        }

        auto description = QString(proc.readAllStandardOutput().trimmed());

        if (!controls.contains(description)) {
            result = false;

            continue;
        }

        args << QString("%1=%2").arg(control).arg(controls[description].toInt());
    }

    proc.start(manager, QStringList {"set-controls", this->m_device} + args);
    proc.waitForFinished();

    if (proc.exitCode())
        result = false;

    return result;
}

QList<quint64> VCamCMIO::clientsPids() const
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return {};

    QProcess proc;
    proc.start(manager, {"-p", "clients"});
    proc.waitForFinished();

    if (proc.exitCode())
        return {};

    QList<quint64> pids;

    for (auto &line: proc.readAllStandardOutput().split('\n')) {
        auto pidExe = line.simplified().split(' ');
        auto pid = pidExe.value(0).toInt();

        if (pid != getpid())
            pids << pid;
    }

    return pids;
}

QString VCamCMIO::clientExe(uint64_t pid) const
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
            return pidExe.value(1);
    }

    return {};
}

QString VCamCMIO::deviceCreate(const QString &description,
                               const AkVideoCapsList &formats)
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return {};

    QProcess proc;
    proc.start(manager, {"-p", "add-device", description});
    proc.waitForFinished();
    auto result = proc.exitCode();
    QString deviceId;

    if (!result) {
        deviceId = QString::fromUtf8(proc.readAllStandardOutput());
        deviceId = deviceId.trimmed();
    }

    if (!result) {
        AkVideoCapsList outputformats;

        for (auto &format: formats) {
            auto width = format.width();
            auto height = format.height();
            auto fps = format.fps();
            auto ot = std::find(outputformats.begin(),
                                outputformats.end(),
                                format);
            auto pixFormat = this->d->formatByAk(format.format()).str;

            if (ot == outputformats.end() && !pixFormat.isEmpty()) {
                proc.start(manager,
                           {"add-format",
                            deviceId,
                            pixFormat,
                            QString::number(width),
                            QString::number(height),
                            fps.toString()});
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
            qDebug() << errorMsg;
            this->d->m_error += QString(errorMsg);
        }

        return {};
    }

    return deviceId;
}

bool VCamCMIO::deviceEdit(const QString &deviceId,
                          const QString &description,
                          const AkVideoCapsList &formats)
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return {};

    QProcess proc;
    proc.start(manager, {"set-description", deviceId, description});
    proc.waitForFinished();
    auto result = proc.exitCode();

    if (!result) {
        proc.start(manager, {"remove-formats", deviceId});
        proc.waitForFinished();
        result = proc.exitCode();
    }

    if (!result) {
        AkVideoCapsList outputformats;

        for (auto &format: formats) {
            auto width = format.width();
            auto height = format.height();
            auto fps = format.fps();
            auto ot = std::find(outputformats.begin(),
                                outputformats.end(),
                                format);
            auto pixFormat = this->d->formatByAk(format.format()).str;

            if (ot == outputformats.end() && !pixFormat.isEmpty()) {
                proc.start(manager,
                           {"add-format",
                            deviceId,
                            pixFormat,
                            QString::number(width),
                            QString::number(height),
                            fps.toString()});
                proc.waitForFinished();

                if (proc.exitCode()) {
                    result = proc.exitCode();

                    break;
                }

                outputformats << format;
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
            qDebug() << errorMsg;
            this->d->m_error += QString(errorMsg);
        }

        return false;
    }

    return true;
}

bool VCamCMIO::changeDescription(const QString &deviceId,
                                 const QString &description)
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return false;

    QProcess proc;
    proc.start(manager, {"set-description", deviceId, description});
    proc.waitForFinished();

    if (!proc.exitCode()) {
        proc.start(manager, {"update"});
        proc.waitForFinished();
    }

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg;
            this->d->m_error += QString(errorMsg);
        }

        return false;
    }

    return true;
}

bool VCamCMIO::deviceDestroy(const QString &deviceId)
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return false;

    QProcess proc;
    proc.start(manager, {"remove-device", deviceId});
    proc.waitForFinished();

    if (!proc.exitCode()) {
        proc.start(manager, {"update"});
        proc.waitForFinished();
    }

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg;
            this->d->m_error += QString(errorMsg);
        }

        return false;
    }

    return true;
}

bool VCamCMIO::destroyAllDevices()
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
            qDebug() << errorMsg;
            this->d->m_error += QString(errorMsg);
        }

        return false;
    }

    return true;
}

bool VCamCMIO::init()
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return false;

    QString params;
    QTextStream paramsStream(&params);
    paramsStream << manager
                 << " "
                 << "stream"
                 << " "
                 << this->m_device
                 << " "
                 << this->d->formatByAk(this->m_currentCaps.format()).str
                 << " "
                 << this->m_currentCaps.width()
                 << " "
                 << this->m_currentCaps.height();
    this->d->m_managerProc = popen(params.toStdString().c_str(), "w");

    return this->d->m_managerProc != nullptr;
}

void VCamCMIO::uninit()
{
    if (this->d->m_managerProc) {
        pclose(this->d->m_managerProc);
        this->d->m_managerProc = nullptr;
    }

    this->d->m_curFormat.clear();
}

bool VCamCMIO::write(const AkVideoPacket &frame)
{
    if (!this->d->m_managerProc)
        return false;

    auto scaled = frame.scaled(this->d->m_curFormat.width(),
                               this->d->m_curFormat.height())
                        .convert(this->d->m_curFormat.format());

    if (!scaled)
        return false;

    return fwrite(scaled.buffer().data(),
                  scaled.buffer().size(),
                  1,
                  this->d->m_managerProc) > 0;
}

void VCamCMIO::cameraConnected()
{
    this->d->updateDevices();
}

void VCamCMIO::cameraDisconnected()
{
    this->d->updateDevices();
}

VCamCMIOPrivate::VCamCMIOPrivate(VCamCMIO *self):
    self(self)
{
}

VCamCMIOPrivate::~VCamCMIOPrivate()
{

}

bool VCamCMIOPrivate::canUseCamera()
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

const CMIOAkFormatMap &VCamCMIOPrivate::cmioAkFormatMap() const
{
    static const CMIOAkFormatMap formatMap {
        {0                             , AkVideoCaps::Format_none    , ""     },

        // RGB formats
        {kCMPixelFormat_32ARGB         , AkVideoCaps::Format_0rgb    , "RGB32"},
        {kCMPixelFormat_24RGB          , AkVideoCaps::Format_rgb24   , "RGB24"},
        {kCMPixelFormat_16LE565        , AkVideoCaps::Format_rgb565le, "RGB16"},
        {kCMPixelFormat_16LE555        , AkVideoCaps::Format_rgb555le, "RGB15"},

        // YUV formats
        {kCMPixelFormat_422YpCbCr8     , AkVideoCaps::Format_uyvy422 , "UYVY"},
        {kCMPixelFormat_422YpCbCr8_yuvs, AkVideoCaps::Format_yuyv422 , "YUY2"}
    };

    return formatMap;
}

const CMIOAkFormat &VCamCMIOPrivate::formatByFourcc(FourCharCode fourcc) const
{
    auto &formatMap = this->cmioAkFormatMap();

    for (auto &format: formatMap)
        if (format.fourcc == fourcc)
            return format;

    return formatMap.first();
}

const CMIOAkFormat &VCamCMIOPrivate::formatByAk(AkVideoCaps::PixelFormat ak) const
{
    auto &formatMap = this->cmioAkFormatMap();

    for (auto &format: formatMap)
        if (format.ak == ak)
            return format;

    return formatMap.first();
}

QString VCamCMIOPrivate::manager() const
{
    QString pluginPath = CMIO_PLUGINS_DAL_PATH
                         "/"
                         CMIO_PLUGIN_NAME
                         ".plugin/Contents/Resources/"
                         CMIO_PLUGIN_MANAGER_NAME;

    if (!QFileInfo::exists(pluginPath))
        return {};

    return QFileInfo(pluginPath).canonicalFilePath();
}

QStringList VCamCMIOPrivate::devices() const
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

void VCamCMIOPrivate::updateDevices()
{
    if (!VCamCMIOPrivate::canUseCamera())
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

        AkVideoCapsList formatsList;

        // List supported frame formats.
        for (AVCaptureDeviceFormat *format in camera.formats) {
            auto fourCC = CMFormatDescriptionGetMediaSubType(format.formatDescription);
            CMVideoDimensions size =
                    CMVideoFormatDescriptionGetDimensions(format.formatDescription);
            AkVideoCaps videoFormat(this->formatByFourcc(fourCC).ak,
                                    size.width,
                                    size.height,
                                    {30, 1});

            // List all supported frame rates for the format.
            for (AVFrameRateRange *fpsRange in format.videoSupportedFrameRateRanges) {
                videoFormat.fps() = {qRound(1e3 * fpsRange.maxFrameRate), 1000};
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

#include "moc_vcamcmio.cpp"
