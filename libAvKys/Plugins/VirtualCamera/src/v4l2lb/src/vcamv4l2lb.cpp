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

#include <cerrno>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFileSystemWatcher>
#include <QMutex>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSettings>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QThread>
#include <fcntl.h>
#include <limits>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <libkmod.h>
#include <akelement.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>

#include "vcamv4l2lb.h"

#define MAX_CAMERAS 64

enum IoMethod
{
    IoMethodUnknown = -1,
    IoMethodReadWrite,
    IoMethodMemoryMap,
    IoMethodUserPointer
};

enum DeviceType
{
    DeviceTypeCapture,
    DeviceTypeOutput
};

struct CaptureBuffer
{
    char *start;
    size_t length;
};

using RwMode = __u32;

struct DeviceInfo
{
    int nr;
    QString path;
    QString description;
    QString driver;
    QString bus;
    AkVideoCapsList formats;
    QStringList connectedDevices;
    DeviceType type;
};

struct DeviceControl
{
    QString name;
    QString type;
    int minimum;
    int maximum;
    int step;
    int default_value;
    QStringList menu;
};

using DeviceControls = QVector<DeviceControl>;
using DeviceControlValues = QMap<QString, int>;

struct V4L2AkFormat
{
    uint32_t v4l2;
    AkVideoCaps::PixelFormat ak;
    QString str;
};

using V4L2AkFormatMap = QVector<V4L2AkFormat>;
using V4l2CtrlTypeMap = QMap<v4l2_ctrl_type, QString>;

class VCamV4L2LoopBackPrivate
{
    public:
        VCamV4L2LoopBack *self;
        QString m_device;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, AkVideoCapsList> m_devicesFormats;
        AkVideoCapsList m_defaultFormats;
        QVariantList m_globalControls;
        QVariantMap m_localControls;
        QFileSystemWatcher *m_fsWatcher;
        QVector<CaptureBuffer> m_buffers;
        QMap<QString, DeviceControlValues> m_deviceControlValues;
        QMutex m_controlsMutex;
        AkElementPtr m_flipFilter   {akPluginManager->create<AkElement>("VideoFilter/Flip")};
        AkElementPtr m_scaleFilter  {akPluginManager->create<AkElement>("VideoFilter/Scale")};
        AkElementPtr m_swapRBFilter {akPluginManager->create<AkElement>("VideoFilter/SwapRB")};
        QString m_error;
        AkVideoCaps m_currentCaps;
        AkVideoCaps m_outputCaps;
        QString m_rootMethod;
        IoMethod m_ioMethod {IoMethodUnknown};
        int m_fd {-1};
        int m_nBuffers {32};

        explicit VCamV4L2LoopBackPrivate(VCamV4L2LoopBack *self);
        VCamV4L2LoopBackPrivate(const VCamV4L2LoopBackPrivate &other) = delete;
        ~VCamV4L2LoopBackPrivate();

        inline int xioctl(int fd, ulong request, void *arg) const;
        bool sudo(const QString &script);
        QStringList availableRootMethods() const;
        QString whereBin(const QString &binary) const;
        QVariantList controls(int fd, quint32 controlClass) const;
        QVariantList controls(int fd) const;
        bool setControls(int fd,
                         quint32 controlClass,
                         const QVariantMap &controls) const;
        bool setControls(int fd,
                         const QVariantMap &controls) const;
        QVariantList queryControl(int handle,
                                  quint32 controlClass,
                                  v4l2_queryctrl *queryctrl) const;
        QMap<QString, quint32> findControls(int handle,
                                            quint32 controlClass) const;
        QVariantMap controlStatus(const QVariantList &controls) const;
        QVariantMap mapDiff(const QVariantMap &map1,
                            const QVariantMap &map2) const;
        inline const V4L2AkFormatMap &v4l2AkFormatMap() const;
        inline const V4L2AkFormat &formatByV4L2(uint32_t v4l2) const;
        inline const V4L2AkFormat &formatByAk(AkVideoCaps::PixelFormat ak) const;
        inline const V4L2AkFormat &formatByStr(const QString &str) const;
        inline const V4l2CtrlTypeMap &ctrlTypeToStr() const;
        inline const DeviceControls &deviceControls() const;
        AkVideoCapsList formatFps(int fd,
                                  const struct v4l2_fmtdesc &format,
                                  __u32 width,
                                  __u32 height) const;
        AkVideoCapsList formats(int fd) const;
        QList<QStringList> combineMatrix(const QList<QStringList> &matrix) const;
        void combineMatrixP(const QList<QStringList> &matrix,
                            size_t index,
                            QStringList &combined,
                            QList<QStringList> &combinations) const;
        QList<AkVideoCapsList> readFormats(QSettings &settings) const;
        QList<DeviceInfo> readDevicesConfigs() const;
        AkVideoCapsList formatsFromSettings(const QString &deviceId,
                                            const QList<DeviceInfo> &devicesInfo) const;
        void setFps(int fd, const v4l2_fract &fps);
        bool initReadWrite(quint32 bufferSize);
        bool initMemoryMap();
        bool initUserPointer(quint32 bufferSize);
        void initDefaultFormats();
        bool startOutput();
        void stopOutput();
        void updateDevices();
        QString cleanDescription(const QString &description) const;
        QVector<int> requestDeviceNR(size_t count) const;
        bool waitForDevice(const QString &deviceId) const;
        bool waitForDevices(const QStringList &devices) const;
        inline QStringList v4l2Devices() const;
        QList<DeviceInfo> devicesInfo() const;
        inline QString stringFromIoctl(ulong cmd) const;
        template<typename T>
        static inline T alignUp(const T &value, const T &align)
        {
            return (value + align - 1) & ~(align - 1);
        }
};

VCamV4L2LoopBack::VCamV4L2LoopBack(QObject *parent):
    VCam(parent)
{
    this->d = new VCamV4L2LoopBackPrivate(this);
    static const QStringList preferredRootMethod {
        "pkexec",
    };

    auto availableMethods = this->d->availableRootMethods();

    for (auto &method: preferredRootMethod)
        if (availableMethods.contains(method)) {
            this->d->m_rootMethod = method;

            break;
        }
}

VCamV4L2LoopBack::~VCamV4L2LoopBack()
{
    delete this->d;
}

QString VCamV4L2LoopBack::error() const
{
    return this->d->m_error;
}

bool VCamV4L2LoopBack::isInstalled() const
{
    auto modules = QString("/lib/modules/%1/modules.dep")
                   .arg(QSysInfo::kernelVersion());
    QFile file(modules);

    if (!file.open(QIODevice::ReadOnly))
        return {};

    forever {
        auto line = file.readLine();

        if (line.isEmpty())
            break;

        auto driver = QFileInfo(line.left(line.indexOf(':'))).baseName();

        if (driver == "v4l2loopback")
            return true;
    }

    return false;
}

QString VCamV4L2LoopBack::installedVersion() const
{
    QString version;
    static const char moduleName[] = "v4l2loopback";
    auto modulesDir = QString("/lib/modules/%1").arg(QSysInfo::kernelVersion());
    const char *config = NULL;
    auto ctx = kmod_new(modulesDir.toStdString().c_str(), &config);

    if (ctx) {
        struct kmod_module *module = NULL;
        int error = kmod_module_new_from_name(ctx,  moduleName, &module);

        if (error == 0 && module) {
            struct kmod_list *info = NULL;
            error = kmod_module_get_info(module, &info);

            if (error >= 0 && info) {
                for (auto entry = info;
                     entry;
                     entry = kmod_list_next(info, entry)) {
                    auto key = kmod_module_info_get_key(entry);

                    if (strncmp(key, "version", 7) == 0) {
                        version = QString::fromLatin1(kmod_module_info_get_value(entry));

                        break;
                    }
                }

                kmod_module_info_free_list(info);
            }

            kmod_module_unref(module);
        }

        kmod_unref(ctx);
    }

    return version;
}

QStringList VCamV4L2LoopBack::webcams() const
{
    return this->d->m_devices;
}

QString VCamV4L2LoopBack::device() const
{
    return this->d->m_device;
}

QString VCamV4L2LoopBack::description(const QString &deviceId) const
{
    return this->d->m_descriptions.value(deviceId);
}

QList<AkVideoCaps::PixelFormat> VCamV4L2LoopBack::supportedOutputPixelFormats() const
{
    return {
        AkVideoCaps::Format_rgb24,
        AkVideoCaps::Format_rgb565le,
        AkVideoCaps::Format_rgb555le,
        AkVideoCaps::Format_0bgr,
        AkVideoCaps::Format_bgr24,
        AkVideoCaps::Format_uyvy422,
        AkVideoCaps::Format_yuyv422,
    };
}

AkVideoCaps::PixelFormat VCamV4L2LoopBack::defaultOutputPixelFormat() const
{
    return AkVideoCaps::Format_yuyv422;
}

AkVideoCapsList VCamV4L2LoopBack::caps(const QString &deviceId) const
{
    if (!this->d->m_devicesFormats.contains(deviceId))
        return {};

    return this->d->m_devicesFormats[deviceId];
}

AkVideoCaps VCamV4L2LoopBack::currentCaps() const
{
    return this->d->m_currentCaps;
}

QVariantList VCamV4L2LoopBack::controls() const
{
    return this->d->m_globalControls;
}

bool VCamV4L2LoopBack::setControls(const QVariantMap &controls)
{
    this->d->m_controlsMutex.lock();
    auto globalControls = this->d->m_globalControls;
    this->d->m_controlsMutex.unlock();

    for (int i = 0; i < globalControls.count(); i++) {
        auto control = globalControls[i].toList();
        auto controlName = control[0].toString();

        if (controls.contains(controlName)) {
            control[6] = controls[controlName];
            globalControls[i] = control;
        }
    }

    this->d->m_controlsMutex.lock();

    if (this->d->m_globalControls == globalControls) {
        this->d->m_controlsMutex.unlock();

        return false;
    }

    this->d->m_globalControls = globalControls;
    this->d->m_controlsMutex.unlock();

    if (this->d->m_fd < 0) {
        int fd = open(this->d->m_device.toStdString().c_str(), O_RDWR | O_NONBLOCK, 0);

            if (fd >= 0) {
                bool result = this->d->setControls(fd, controls);
                close(fd);

                return result;
        }
    }

    emit this->controlsChanged(controls);

    return true;
}

QList<quint64> VCamV4L2LoopBack::clientsPids() const
{
    auto devices = this->d->devicesInfo();
    QList<quint64> clientsPids;

    QDir procDir("/proc");
    auto pids = procDir.entryList(QStringList() << "[0-9]*",
                                  QDir::Dirs
                                  | QDir::Readable
                                  | QDir::NoSymLinks
                                  | QDir::NoDotAndDotDot,
                                  QDir::Name);

    for (auto &pidStr: pids) {
        bool ok = false;
        auto pid = pidStr.toULongLong(&ok);

        if (!ok)
            continue;

        QStringList videoDevices;
        QDir fdDir(QString("/proc/%1/fd").arg(pid));
        auto fds = fdDir.entryList(QStringList() << "[0-9]*",
                                   QDir::Files
                                   | QDir::Readable
                                   | QDir::NoDotAndDotDot,
                                   QDir::Name);

        for (auto &fd: fds) {
            QFileInfo fdInfo(fdDir.absoluteFilePath(fd));
            QString target = fdInfo.isSymLink()? fdInfo.symLinkTarget(): "";
            static const QRegularExpression re("^/dev/video[0-9]+$");

            if (re.match(target).hasMatch())
                videoDevices << target;
        }

        for (auto &device: devices)
            if (videoDevices.contains(device.path)) {
                clientsPids << pid;

                break;
            }
    }

    std::sort(clientsPids.begin(), clientsPids.end());

    return clientsPids;
}

QString VCamV4L2LoopBack::clientExe(quint64 pid) const
{
    return QFileInfo(QString("/proc/%1/exe").arg(pid)).symLinkTarget();
}

QString VCamV4L2LoopBack::rootMethod() const
{
    return this->d->m_rootMethod;
}

QStringList VCamV4L2LoopBack::availableRootMethods() const
{
    return this->d->availableRootMethods();
}

QString VCamV4L2LoopBack::deviceCreate(const QString &description,
                                       const AkVideoCapsList &formats)
{
    this->d->m_error = "";

    if (!this->clientsPids().isEmpty()) {
        this->d->m_error = "The driver is in use";

        return {};
    }

    auto deviceNR = this->d->requestDeviceNR(2);

    if (deviceNR.count() < 2) {
        this->d->m_error = "No available devices to create a virtual camera";

        return {};
    }

    // Fill device info.
    auto deviceId = QString("/dev/video%1").arg(deviceNR.front());
    auto devices = this->d->devicesInfo();
    devices << DeviceInfo {deviceNR.front(),
                           deviceId,
                           this->d->cleanDescription(description),
                           "",
                           "",
                           {},
                           {},
                           DeviceTypeCapture};

    // Read description.
    QString videoNR;
    QString cardLabel;

    for (auto &device: devices) {
        if (!videoNR.isEmpty())
            videoNR += ',';

        videoNR += QString("%1").arg(device.nr);

        if (!cardLabel.isEmpty())
            cardLabel += ',';

        cardLabel += device.description;
    }

    // Write the script file.

    QString script;
    QTextStream ts(&script);
    ts << "rmmod v4l2loopback 2>/dev/null" << Qt::endl;
    ts << "sed -i '/v4l2loopback/d' /etc/modules 2>/dev/null" << Qt::endl;
    ts << "sed -i '/v4l2loopback/d' /etc/modules-load.d/*.conf 2>/dev/null" << Qt::endl;
    ts << "sed -i '/v4l2loopback/d' /etc/modprobe.d/*.conf 2>/dev/null" << Qt::endl;
    ts << "echo v4l2loopback > /etc/modules-load.d/v4l2loopback.conf" << Qt::endl;
    ts << "echo options v4l2loopback video_nr="
       << videoNR
       << " 'card_label=\""
       << cardLabel
       << "\"' > /etc/modprobe.d/v4l2loopback.conf" << Qt::endl;
    ts << "modprobe v4l2loopback video_nr=" << videoNR << " card_label=\"" << cardLabel << "\"" << Qt::endl;

    // Execute the script
    if (!this->d->sudo(script))
        return {};

    if (!this->d->waitForDevice(deviceId)) {
        this->d->m_error = "Time exceeded while waiting for the device";

        return {};
    }

    // Save virtual camera settings.
    auto devicesInfo = this->d->readDevicesConfigs();
    QSettings settings(QCoreApplication::organizationName(),
                       "VirtualCamera");
    int i = 0;
    int j = 0;

    for (auto &device: this->d->devicesInfo()) {
        if (device.path == deviceId) {
            device.formats.clear();

            for (auto &format: formats)
                device.formats << format;
        } else {
            device.formats =
                    this->d->formatsFromSettings(device.path, devicesInfo);
        }

        if (device.formats.empty())
            device.formats = this->d->m_defaultFormats;

        QStringList formatsIndex;

        for (int i = 0; i < device.formats.size(); i++)
            formatsIndex << QString("%1").arg(i + j + 1);

        settings.beginGroup("Cameras");
        settings.beginWriteArray("cameras");
        settings.setArrayIndex(i);
        settings.setValue("driver", device.driver);
        settings.setValue("bus", device.bus);
        settings.setValue("formats", formatsIndex);
        settings.endArray();
        settings.endGroup();

        settings.beginGroup("Formats");
        settings.beginWriteArray("formats");

        for (auto &format: device.formats) {
            settings.setArrayIndex(j);
            settings.setValue("format", this->d->formatByAk(format.format()).str);
            settings.setValue("width", format.width());
            settings.setValue("height", format.height());
            settings.setValue("fps", format.fps().toString());
            j++;
        }

        settings.endArray();
        settings.endGroup();

        i++;
    }

    if (!deviceId.isEmpty())
        this->d->updateDevices();

    return deviceId;
}

bool VCamV4L2LoopBack::deviceEdit(const QString &deviceId,
                                  const QString &description,
                                  const AkVideoCapsList &formats)
{
    this->d->m_error = "";

    if (!this->clientsPids().isEmpty()) {
        this->d->m_error = "The driver is in use";

        return false;
    }

    auto devices = this->d->devicesInfo();

    for (auto &device: devices)
        if (device.path == deviceId) {
            device.description = this->d->cleanDescription(description);

            break;
        }

    // Read description.
    QString videoNR;
    QString cardLabel;

    for (auto &device: devices) {
        if (!videoNR.isEmpty())
            videoNR += ',';

        videoNR += QString("%1").arg(device.nr);

        if (!cardLabel.isEmpty())
            cardLabel += ',';

        cardLabel += device.description;
    }

    // Write the script file.

    QString script;
    QTextStream ts(&script);
    ts << "rmmod v4l2loopback 2>/dev/null" << Qt::endl;
    ts << "sed -i '/v4l2loopback/d' /etc/modules 2>/dev/null" << Qt::endl;
    ts << "sed -i '/v4l2loopback/d' /etc/modules-load.d/*.conf 2>/dev/null" << Qt::endl;
    ts << "sed -i '/v4l2loopback/d' /etc/modprobe.d/*.conf 2>/dev/null" << Qt::endl;
    ts << "echo v4l2loopback > /etc/modules-load.d/v4l2loopback.conf" << Qt::endl;
    ts << "echo options v4l2loopback video_nr="
       << videoNR
       << " 'card_label=\""
       << cardLabel
       << "\"' > /etc/modprobe.d/v4l2loopback.conf" << Qt::endl;
    ts << "modprobe v4l2loopback video_nr=" << videoNR << " card_label=\"" << cardLabel << "\"" << Qt::endl;

    // Execute the script
    if (!this->d->sudo(script))
        return false;

    if (!this->d->waitForDevice(deviceId)) {
        this->d->m_error = "Time exceeded while waiting for the device";

        return false;
    }

    // Save virtual camera settings.
    auto devicesInfo = this->d->readDevicesConfigs();
    QSettings settings(QCoreApplication::organizationName(),
                       "VirtualCamera");
    int i = 0;
    int j = 0;

    for (auto &device: this->d->devicesInfo()) {
        if (device.path == deviceId) {
            device.formats.clear();

            for (auto &format: formats)
                device.formats << format;
        } else {
            device.formats =
                    this->d->formatsFromSettings(device.path, devicesInfo);
        }

        if (device.formats.empty())
            device.formats = this->d->m_defaultFormats;

        QStringList formatsIndex;

        for (int i = 0; i < device.formats.size(); i++)
            formatsIndex << QString("%1").arg(i + j + 1);

        settings.beginGroup("Cameras");
        settings.beginWriteArray("cameras");
        settings.setArrayIndex(i);
        settings.setValue("driver", device.driver);
        settings.setValue("bus", device.bus);
        settings.setValue("formats", formatsIndex);
        settings.endArray();
        settings.endGroup();

        settings.beginGroup("Formats");
        settings.beginWriteArray("formats");

        for (auto &format: device.formats) {
            settings.setArrayIndex(j);
            settings.setValue("format", this->d->formatByAk(format.format()).str);
            settings.setValue("width", format.width());
            settings.setValue("height", format.height());
            settings.setValue("fps", format.fps().toString());
            j++;
        }

        settings.endArray();
        settings.endGroup();

        i++;
    }

    this->d->updateDevices();

    return true;
}

bool VCamV4L2LoopBack::changeDescription(const QString &deviceId,
                                         const QString &description)
{
    this->d->m_error = "";

    if (!this->clientsPids().isEmpty()) {
        this->d->m_error = "The driver is in use";

        return false;
    }

    auto devices = this->d->devicesInfo();
    QString videoNR;
    QString cardLabel;

    for (auto &device: devices) {
        if (!videoNR.isEmpty())
            videoNR += ',';

        videoNR += QString("%1").arg(device.nr);

        if (!cardLabel.isEmpty())
            cardLabel += ',';

        if (device.path == deviceId)
            cardLabel += this->d->cleanDescription(description);
        else
            cardLabel += device.description;
    }

    // Write the script file.

    QString script;
    QTextStream ts(&script);
    ts << "rmmod v4l2loopback 2>/dev/null" << Qt::endl;
    ts << "sed -i '/v4l2loopback/d' /etc/modules 2>/dev/null" << Qt::endl;
    ts << "sed -i '/v4l2loopback/d' /etc/modules-load.d/*.conf 2>/dev/null" << Qt::endl;
    ts << "sed -i '/v4l2loopback/d' /etc/modprobe.d/*.conf 2>/dev/null" << Qt::endl;
    ts << "echo v4l2loopback > /etc/modules-load.d/v4l2loopback.conf" << Qt::endl;
    ts << "echo options v4l2loopback video_nr="
       << videoNR
       << " 'card_label=\""
       << cardLabel
       << "\"' > /etc/modprobe.d/v4l2loopback.conf" << Qt::endl;
    ts << "modprobe v4l2loopback video_nr=" << videoNR << " card_label=\"" << cardLabel << "\"" << Qt::endl;

    if (!this->d->sudo(script))
        return false;

    auto result = this->d->waitForDevice(deviceId);
    this->d->updateDevices();

    return result;
}

bool VCamV4L2LoopBack::deviceDestroy(const QString &deviceId)
{
    this->d->m_error = "";

    if (!this->clientsPids().isEmpty()) {
        this->d->m_error = "The driver is in use";

        return false;
    }

    // Delete the devices
    auto devices = this->d->devicesInfo();
    auto it = std::find_if(devices.begin(),
                           devices.end(),
                           [&deviceId] (const DeviceInfo &device) {
                               return device.path == deviceId;
                           });

    if (it == devices.end()) {
        this->d->m_error = "Device not found";

        return false;
    }

    devices.erase(it);

    // Create the final list of devices.
    QStringList devicesList;

    for (auto &device: this->d->devicesInfo())
        if (device.path != deviceId)
            devicesList << device.path;

    // Read device description.
    QString videoNR;
    QString cardLabel;

    for (auto &device: devices) {
        if (!videoNR.isEmpty())
            videoNR += ',';

        videoNR += QString("%1").arg(device.nr);

        if (!cardLabel.isEmpty())
            cardLabel += ',';

        cardLabel += device.description;
    }

    // Write the script file.

    QString script;
    QTextStream ts(&script);
    ts << "rmmod v4l2loopback 2>/dev/null" << Qt::endl;
    ts << "sed -i '/v4l2loopback/d' /etc/modules 2>/dev/null" << Qt::endl;
    ts << "sed -i '/v4l2loopback/d' /etc/modules-load.d/*.conf 2>/dev/null" << Qt::endl;
    ts << "sed -i '/v4l2loopback/d' /etc/modprobe.d/*.conf 2>/dev/null" << Qt::endl;

    if (devices.empty()) {
        ts << "rm -f /etc/modules-load.d/v4l2loopback.conf" << Qt::endl;
        ts << "rm -f /etc/modprobe.d/v4l2loopback.conf" << Qt::endl;
    } else {
        ts << "echo v4l2loopback > /etc/modules-load.d/v4l2loopback.conf" << Qt::endl;
        ts << "echo options v4l2loopback video_nr="
           << videoNR
           << " 'card_label=\""
           << cardLabel
           << "\"' > /etc/modprobe.d/v4l2loopback.conf" << Qt::endl;
        ts << "modprobe v4l2loopback video_nr=" << videoNR << " card_label=\"" << cardLabel << "\"" << Qt::endl;
    }

    if (!this->d->sudo(script))
        return false;

    if (!this->d->waitForDevices(devicesList)) {
        this->d->m_error = "Time exceeded while waiting for the device";

        return false;
    }

    this->d->updateDevices();

    return true;
}

bool VCamV4L2LoopBack::destroyAllDevices()
{
    this->d->m_error = "";

    if (!this->clientsPids().isEmpty()) {
        this->d->m_error = "The driver is in use";

        return false;
    }

    // Write the script file.

    QString script;
    QTextStream ts(&script);
    ts << "rmmod v4l2loopback 2>/dev/null" << Qt::endl;
    ts << "sed -i '/v4l2loopback/d' /etc/modules 2>/dev/null" << Qt::endl;
    ts << "sed -i '/v4l2loopback/d' /etc/modules-load.d/*.conf 2>/dev/null" << Qt::endl;
    ts << "sed -i '/v4l2loopback/d' /etc/modprobe.d/*.conf 2>/dev/null" << Qt::endl;
    ts << "rm -f /etc/modules-load.d/v4l2loopback.conf" << Qt::endl;
    ts << "rm -f /etc/modprobe.d/v4l2loopback.conf" << Qt::endl;

    if (!this->d->sudo(script))
        return false;

    this->d->updateDevices();

    return true;
}

bool VCamV4L2LoopBack::init()
{
    this->d->m_localControls.clear();

    // Frames read must be blocking so we does not waste CPU time.
    this->d->m_fd = open(this->d->m_device.toStdString().c_str(),
                         O_RDWR | O_NONBLOCK,
                         0);

    if (this->d->m_fd < 0)
        return false;

    v4l2_capability capabilities;
    memset(&capabilities, 0, sizeof(v4l2_capability));

    if (this->d->xioctl(this->d->m_fd, VIDIOC_QUERYCAP, &capabilities) < 0) {
        qDebug() << "VirtualCamera: Can't query capabilities.";
        close(this->d->m_fd);
        this->d->m_fd = -1;

        return false;
    }

    v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    this->d->xioctl(this->d->m_fd, VIDIOC_G_FMT, &fmt);

    auto outputFormats = this->caps(this->d->m_device);

    if (outputFormats.empty()) {
        qDebug() << "VirtualCamera: Can't find a similar format:"
                 << this->d->m_currentCaps;
        close(this->d->m_fd);
        this->d->m_fd = -1;

        return false;
    }

    this->d->m_outputCaps = this->d->m_currentCaps.nearest(outputFormats);
    fmt.fmt.pix.pixelformat =
            this->d->formatByAk(this->d->m_outputCaps.format()).v4l2;
    fmt.fmt.pix.width = __u32(this->d->m_outputCaps.width());
    fmt.fmt.pix.height = __u32(this->d->m_outputCaps.height());

    if (this->d->xioctl(this->d->m_fd, VIDIOC_S_FMT, &fmt) < 0) {
        qDebug() << "VirtualCamera: Can't set format:"
                 << this->d->m_currentCaps;
        close(this->d->m_fd);
        this->d->m_fd = -1;

        return false;
    }

    v4l2_fract fps = {__u32(this->d->m_outputCaps.fps().num()),
                      __u32(this->d->m_outputCaps.fps().den())};
    this->d->setFps(this->d->m_fd, fps);

    if (this->d->m_ioMethod == IoMethodReadWrite
        && capabilities.capabilities & V4L2_CAP_READWRITE
        && this->d->initReadWrite(fmt.fmt.pix.sizeimage)) {
    } else if (this->d->m_ioMethod == IoMethodMemoryMap
             && capabilities.capabilities & V4L2_CAP_STREAMING
             && this->d->initMemoryMap()) {
    } else if (this->d->m_ioMethod == IoMethodUserPointer
             && capabilities.capabilities & V4L2_CAP_STREAMING
             && this->d->initUserPointer(fmt.fmt.pix.sizeimage)) {
    } else
        this->d->m_ioMethod = IoMethodUnknown;

    if (this->d->m_ioMethod != IoMethodUnknown)
        return this->d->startOutput();

    if (capabilities.capabilities & V4L2_CAP_STREAMING) {
        if (this->d->initMemoryMap())
            this->d->m_ioMethod = IoMethodMemoryMap;
        else if (this->d->initUserPointer(fmt.fmt.pix.sizeimage))
            this->d->m_ioMethod = IoMethodUserPointer;
    }

    if (this->d->m_ioMethod == IoMethodUnknown) {
        if (capabilities.capabilities & V4L2_CAP_READWRITE
            && this->d->initReadWrite(fmt.fmt.pix.sizeimage))
            this->d->m_ioMethod = IoMethodReadWrite;
        else
            return false;
    }

    return this->d->startOutput();
}

void VCamV4L2LoopBack::uninit()
{
    this->d->stopOutput();

    if (!this->d->m_buffers.isEmpty()) {
        if (this->d->m_ioMethod == IoMethodReadWrite)
            delete [] this->d->m_buffers[0].start;
        else if (this->d->m_ioMethod == IoMethodMemoryMap)
            for (auto &buffer: this->d->m_buffers)
                munmap(buffer.start, buffer.length);
        else if (this->d->m_ioMethod == IoMethodUserPointer)
            for (auto &buffer: this->d->m_buffers)
                delete [] buffer.start;
    }

    close(this->d->m_fd);
    this->d->m_fd = -1;
    this->d->m_buffers.clear();
}

void VCamV4L2LoopBack::setDevice(const QString &device)
{
    if (this->d->m_device == device)
        return;

    this->d->m_device = device;

    if (device.isEmpty()) {
        this->d->m_controlsMutex.lock();
        this->d->m_globalControls.clear();
        this->d->m_controlsMutex.unlock();
    } else {
        this->d->m_controlsMutex.lock();
        int fd = open(device.toStdString().c_str(), O_RDWR | O_NONBLOCK, 0);

        if (fd >= 0) {
            this->d->m_globalControls = this->d->controls(fd);
            close(fd);

            for (auto &control: this->d->deviceControls()) {
                int value = control.default_value;

                if (this->d->m_deviceControlValues.contains(this->d->m_device)
                    && this->d->m_deviceControlValues[this->d->m_device].contains(control.name))
                    value = this->d->m_deviceControlValues[this->d->m_device][control.name];

                QVariantList controlVar {
                    control.name,
                    control.type,
                    control.minimum,
                    control.maximum,
                    control.step,
                    control.default_value,
                    value,
                    control.menu
                };
                this->d->m_globalControls << QVariant(controlVar);
            }
        }

        this->d->m_controlsMutex.unlock();
    }

    this->d->m_controlsMutex.lock();
    auto status = this->d->controlStatus(this->d->m_globalControls);
    this->d->m_controlsMutex.unlock();

    emit this->deviceChanged(device);
    emit this->controlsChanged(status);
}

void VCamV4L2LoopBack::setCurrentCaps(const AkVideoCaps &currentCaps)
{
    if (this->d->m_currentCaps == currentCaps)
        return;

    this->d->m_currentCaps = currentCaps;
    emit this->currentCapsChanged(this->d->m_currentCaps);
}

void VCamV4L2LoopBack::setRootMethod(const QString &rootMethod)
{
    if (this->d->m_rootMethod == rootMethod)
        return;

    this->d->m_rootMethod = rootMethod;
    emit this->rootMethodChanged(this->d->m_rootMethod);
}

bool VCamV4L2LoopBack::write(const AkVideoPacket &packet)
{
    if (this->d->m_buffers.isEmpty())
        return false;

    if (this->d->m_fd < 0)
        return false;

    this->d->m_controlsMutex.lock();

    auto curControls = this->d->controlStatus(this->d->m_globalControls);

    this->d->m_controlsMutex.unlock();

    if (this->d->m_localControls != curControls) {
        auto controls = this->d->mapDiff(this->d->m_localControls,
                                         curControls);
        this->d->setControls(this->d->m_fd, controls);
        this->d->m_localControls = curControls;

        QVariantMap ctrls;

        for (auto &control: this->d->deviceControls())
            if (controls.contains(control.name))
                ctrls[control.name] = controls[control.name];

        if (!ctrls.isEmpty()) {
            if (!this->d->m_deviceControlValues.contains(this->d->m_device))
                this->d->m_deviceControlValues[this->d->m_device] = {};

            for (auto it = ctrls.begin(); it != ctrls.end(); it++)
                this->d->m_deviceControlValues[this->d->m_device][it.key()] =
                    it.value().toInt();
        }
    }

    auto values = this->d->m_deviceControlValues[this->d->m_device];
    this->d->m_flipFilter->setProperty("horizontalFlip", values.value("Horizontal Flip", false));
    this->d->m_flipFilter->setProperty("verticalFlip", values.value("Vertical Flip", false));
    auto packet_ = this->d->m_flipFilter->iStream(packet);

    if (values.value("Swap Read and Blue", false))
        packet_ = this->d->m_swapRBFilter->iStream(packet_);

    this->d->m_scaleFilter->setProperty("width", this->d->m_outputCaps.width());
    this->d->m_scaleFilter->setProperty("height", this->d->m_outputCaps.height());
    this->d->m_scaleFilter->setProperty("scaling", values.value("Scaling Mode", 0));
    this->d->m_scaleFilter->setProperty("aspectRatio", values.value("Aspect Ratio Mode", 0));
    packet_ = this->d->m_scaleFilter->iStream(packet_);
    packet_ = AkVideoPacket(packet_).convert(this->d->m_outputCaps.format());

    if (!packet_)
        return false;

    if (this->d->m_ioMethod == IoMethodReadWrite) {
        memcpy(this->d->m_buffers[0].start,
               packet_.buffer().data(),
               qMin<size_t>(this->d->m_buffers[0].length,
                            packet_.buffer().size()));

        return ::write(this->d->m_fd,
                       this->d->m_buffers[0].start,
                       this->d->m_buffers[0].length) >= 0;
    }

    if (this->d->m_ioMethod == IoMethodMemoryMap
        || this->d->m_ioMethod == IoMethodUserPointer) {
        v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(v4l2_buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buffer.memory = (this->d->m_ioMethod == IoMethodMemoryMap)?
                            V4L2_MEMORY_MMAP:
                            V4L2_MEMORY_USERPTR;

        if (this->d->xioctl(this->d->m_fd, VIDIOC_DQBUF, &buffer) < 0)
            return false;

        if (buffer.index >= quint32(this->d->m_buffers.size()))
            return false;

        memcpy(this->d->m_buffers[int(buffer.index)].start,
               packet_.buffer().data(),
               qMin<size_t>(buffer.bytesused,
                            packet_.buffer().size()));

        return this->d->xioctl(this->d->m_fd, VIDIOC_QBUF, &buffer) >= 0;
    }

    return false;
}

VCamV4L2LoopBackPrivate::VCamV4L2LoopBackPrivate(VCamV4L2LoopBack *self):
    self(self)
{
    this->m_fsWatcher = new QFileSystemWatcher({"/dev"}, self);
    QObject::connect(this->m_fsWatcher,
                     &QFileSystemWatcher::directoryChanged,
                     self,
                     [this] () {
        this->updateDevices();
    });
    this->initDefaultFormats();
    this->updateDevices();
}

VCamV4L2LoopBackPrivate::~VCamV4L2LoopBackPrivate()
{
    delete this->m_fsWatcher;
}

int VCamV4L2LoopBackPrivate::xioctl(int fd, ulong request, void *arg) const
{
    int r = -1;

    for (;;) {
        r = ioctl(fd, request, arg);

        if (r != -1 || errno != EINTR)
            break;
    }

#if 0
    if (r < 0)
        qDebug() << this->stringFromIoctl(request).toStdString().c_str()
                 << ":"
                 << strerror(errno)
                 << QString("(%1)").arg(errno).toStdString().c_str();
#endif

    return r;
}

bool VCamV4L2LoopBackPrivate::sudo(const QString &script)
{
    if (this->m_rootMethod.isEmpty()) {
        static const QString msg = "Root method not set";
        qDebug() << msg;
        this->m_error += msg + " ";

        return false;
    }

    auto sudoBin = this->whereBin(this->m_rootMethod);

    if (sudoBin.isEmpty()) {
        static const QString msg = "Can't find " + this->m_rootMethod;
        qDebug() << msg;
        this->m_error += msg + " ";

        return false;
    }

    QProcess su;
    su.start(sudoBin, QStringList {});

    if (su.waitForStarted()) {
       su.write(script.toUtf8());
       su.closeWriteChannel();
    }

    su.waitForFinished(-1);

    if (su.exitCode()) {
        auto outMsg = su.readAllStandardOutput();
        this->m_error.clear();

        if (!outMsg.isEmpty()) {
            qDebug() << outMsg.toStdString().c_str();
            this->m_error += QString(outMsg) + " ";
        }

        auto errorMsg = su.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->m_error += QString(outMsg);
        }

        return false;
    }

    return true;
}

QStringList VCamV4L2LoopBackPrivate::availableRootMethods() const
{
    static const QStringList sus {
        "pkexec",
    };

    QStringList methods;

    for (auto &su: sus)
        if (!this->whereBin(su).isEmpty())
            methods << su;

    return methods;
}

QString VCamV4L2LoopBackPrivate::whereBin(const QString &binary) const
{
    // Limit search paths to trusted directories only.
    static const QStringList paths {
        "/usr/bin",       // GNU/Linux
        "/bin",           // NetBSD
        "/usr/local/bin", // FreeBSD

        // Additionally, search it in a developer provided extra directory.

#ifdef EXTRA_SUDOER_TOOL_DIR
        EXTRA_SUDOER_TOOL_DIR,
#endif
    };

    for (auto &path: paths)
        if (QDir(path).exists(binary))
            return QDir(path).filePath(binary);

    return {};
}

QVariantList VCamV4L2LoopBackPrivate::controls(int fd, quint32 controlClass) const
{
    QVariantList controls;

    if (fd < 0)
        return controls;

    v4l2_queryctrl queryctrl;
    memset(&queryctrl, 0, sizeof(v4l2_queryctrl));
    queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;

    while (this->xioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == 0) {
        auto control = this->queryControl(fd, controlClass, &queryctrl);

        if (!control.isEmpty())
            controls << QVariant(control);

        queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }

    if (queryctrl.id != V4L2_CTRL_FLAG_NEXT_CTRL)
        return controls;

    for (__u32 id = V4L2_CID_USER_BASE; id < V4L2_CID_LASTP1; id++) {
        queryctrl.id = id;

        if (this->xioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == 0) {
            auto control = this->queryControl(fd, controlClass, &queryctrl);

            if (!control.isEmpty())
                controls << QVariant(control);
        }
    }

    for (queryctrl.id = V4L2_CID_PRIVATE_BASE;
         this->xioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == 0;
         queryctrl.id++) {
        auto control = this->queryControl(fd, controlClass, &queryctrl);

        if (!control.isEmpty())
            controls << QVariant(control);
    }

    return controls;
}

QVariantList VCamV4L2LoopBackPrivate::controls(int fd) const
{
    return this->controls(fd, V4L2_CTRL_CLASS_USER)
           + this->controls(fd, V4L2_CTRL_CLASS_CAMERA);
}

bool VCamV4L2LoopBackPrivate::setControls(int fd,
                                          quint32 controlClass,
                                          const QVariantMap &controls) const
{
    if (fd < 0)
        return false;

    auto ctrl2id = this->findControls(fd, controlClass);

    for (auto it = controls.cbegin(); it != controls.cend(); it++) {
        if (!ctrl2id.contains(it.key()))
            continue;

        v4l2_control ctrl;
        memset(&ctrl, 0, sizeof(v4l2_control));
        ctrl.id = ctrl2id[it.key()];
        ctrl.value = it.value().toInt();
        this->xioctl(fd, VIDIOC_S_CTRL, &ctrl);
    }

    return true;
}

bool VCamV4L2LoopBackPrivate::setControls(int fd,
                                          const QVariantMap &controls) const
{
    QVector<quint32> controlClasses {
        V4L2_CTRL_CLASS_USER,
        V4L2_CTRL_CLASS_CAMERA
    };

    for (auto &cls: controlClasses)
        this->setControls(fd, cls, controls);

    return true;
}

QVariantList VCamV4L2LoopBackPrivate::queryControl(int handle,
                                                   quint32 controlClass,
                                                   v4l2_queryctrl *queryctrl) const
{
    if (queryctrl->flags & V4L2_CTRL_FLAG_DISABLED)
        return {};

    if (V4L2_CTRL_ID2CLASS(queryctrl->id) != controlClass)
        return {};

    v4l2_ext_control ext_ctrl;
    memset(&ext_ctrl, 0, sizeof(v4l2_ext_control));
    ext_ctrl.id = queryctrl->id;

    v4l2_ext_controls ctrls;
    memset(&ctrls, 0, sizeof(v4l2_ext_controls));
    ctrls.ctrl_class = V4L2_CTRL_ID2CLASS(queryctrl->id);
    ctrls.count = 1;
    ctrls.controls = &ext_ctrl;

    if (V4L2_CTRL_ID2CLASS(queryctrl->id) != V4L2_CTRL_CLASS_USER &&
        queryctrl->id < V4L2_CID_PRIVATE_BASE) {
        if (this->xioctl(handle, VIDIOC_G_EXT_CTRLS, &ctrls))
            return {};
    } else {
        v4l2_control ctrl;
        memset(&ctrl, 0, sizeof(v4l2_control));
        ctrl.id = queryctrl->id;

        if (this->xioctl(handle, VIDIOC_G_CTRL, &ctrl))
            return QVariantList();

        ext_ctrl.value = ctrl.value;
    }

    v4l2_querymenu qmenu;
    memset(&qmenu, 0, sizeof(v4l2_querymenu));
    qmenu.id = queryctrl->id;
    QStringList menu;

    if (queryctrl->type == V4L2_CTRL_TYPE_MENU)
        for (int i = 0; i < queryctrl->maximum + 1; i++) {
            qmenu.index = __u32(i);

            if (this->xioctl(handle, VIDIOC_QUERYMENU, &qmenu))
                continue;

            menu << QString(reinterpret_cast<const char *>(qmenu.name));
        }

    auto type = static_cast<v4l2_ctrl_type>(queryctrl->type);

    return QVariantList {
        QString(reinterpret_cast<const char *>(queryctrl->name)),
        this->ctrlTypeToStr().value(type),
        queryctrl->minimum,
        queryctrl->maximum,
        queryctrl->step,
        queryctrl->default_value,
        ext_ctrl.value,
        menu
    };
}

QMap<QString, quint32> VCamV4L2LoopBackPrivate::findControls(int handle,
                                                             quint32 controlClass) const
{
    v4l2_queryctrl qctrl;
    memset(&qctrl, 0, sizeof(v4l2_queryctrl));
    qctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    QMap<QString, quint32> controls;

    while (this->xioctl(handle, VIDIOC_QUERYCTRL, &qctrl) == 0) {
        if (!(qctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            && V4L2_CTRL_ID2CLASS(qctrl.id) == controlClass)
            controls[QString(reinterpret_cast<const char *>(qctrl.name))] = qctrl.id;

        qctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }

    if (qctrl.id != V4L2_CTRL_FLAG_NEXT_CTRL)
        return controls;

    for (__u32 id = V4L2_CID_USER_BASE; id < V4L2_CID_LASTP1; id++) {
        qctrl.id = id;

        if (this->xioctl(handle, VIDIOC_QUERYCTRL, &qctrl) == 0
            && !(qctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            && V4L2_CTRL_ID2CLASS(qctrl.id) == controlClass)
            controls[QString(reinterpret_cast<const char *>(qctrl.name))] = qctrl.id;
    }

    qctrl.id = V4L2_CID_PRIVATE_BASE;

    while (this->xioctl(handle, VIDIOC_QUERYCTRL, &qctrl) == 0) {
        if (!(qctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            && V4L2_CTRL_ID2CLASS(qctrl.id) == controlClass)
            controls[QString(reinterpret_cast<const char *>(qctrl.name))] = qctrl.id;

        qctrl.id++;
    }

    return controls;
}

QVariantMap VCamV4L2LoopBackPrivate::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    for (auto &control: controls) {
        auto params = control.toList();
        auto controlName = params[0].toString();
        controlStatus[controlName] = params[6];
    }

    return controlStatus;
}

QVariantMap VCamV4L2LoopBackPrivate::mapDiff(const QVariantMap &map1,
                                             const QVariantMap &map2) const
{
    QVariantMap map;

    for (auto it = map2.cbegin(); it != map2.cend(); it++)
        if (!map1.contains(it.key())
            || map1[it.key()] != it.value()) {
            map[it.key()] = it.value();
        }

    return map;
}

inline const V4L2AkFormatMap &VCamV4L2LoopBackPrivate::v4l2AkFormatMap() const
{
    static const V4L2AkFormatMap formatMap = {
        {0                  , AkVideoCaps::Format_none    , ""     },

        // RGB formats
        {V4L2_PIX_FMT_RGB32 , AkVideoCaps::Format_0rgb    , "RGB32"},
        {V4L2_PIX_FMT_RGB24 , AkVideoCaps::Format_rgb24   , "RGB24"},
        {V4L2_PIX_FMT_RGB565, AkVideoCaps::Format_rgb565le, "RGB16"},
        {V4L2_PIX_FMT_RGB555, AkVideoCaps::Format_rgb555le, "RGB15"},

        // BGR formats
        {V4L2_PIX_FMT_BGR32 , AkVideoCaps::Format_0bgr    , "BGR32"},
        {V4L2_PIX_FMT_BGR24 , AkVideoCaps::Format_bgr24   , "BGR24"},

        // Luminance+Chrominance formats
        {V4L2_PIX_FMT_UYVY  , AkVideoCaps::Format_uyvy422 , "UYVY" },
        {V4L2_PIX_FMT_YUYV  , AkVideoCaps::Format_yuyv422 , "YUY2" },
    };

    return formatMap;
}

const V4L2AkFormat &VCamV4L2LoopBackPrivate::formatByV4L2(uint32_t v4l2) const
{
    auto &formatMap = this->v4l2AkFormatMap();

    for (auto &format: formatMap)
        if (format.v4l2 == v4l2)
            return format;

    return formatMap.first();
}

const V4L2AkFormat &VCamV4L2LoopBackPrivate::formatByAk(AkVideoCaps::PixelFormat ak) const
{
    auto &formatMap = this->v4l2AkFormatMap();

    for (auto &format: formatMap)
        if (format.ak == ak)
            return format;

    return formatMap.first();
}

const V4L2AkFormat &VCamV4L2LoopBackPrivate::formatByStr(const QString &str) const
{
    auto &formatMap = this->v4l2AkFormatMap();

    for (auto &format: formatMap)
        if (format.str == str)
            return format;

    return formatMap.first();
}

const V4l2CtrlTypeMap &VCamV4L2LoopBackPrivate::ctrlTypeToStr() const
{
    static const V4l2CtrlTypeMap ctrlTypeToStr = {
        // V4L2 controls
        {V4L2_CTRL_TYPE_INTEGER     , "integer"    },
        {V4L2_CTRL_TYPE_BOOLEAN     , "boolean"    },
        {V4L2_CTRL_TYPE_MENU        , "menu"       },
        {V4L2_CTRL_TYPE_BUTTON      , "button"     },
        {V4L2_CTRL_TYPE_INTEGER64   , "integer64"  },
        {V4L2_CTRL_TYPE_CTRL_CLASS  , "ctrlClass"  },
#ifdef HAVE_EXTENDEDCONTROLS
        {V4L2_CTRL_TYPE_STRING      , "string"     },
        {V4L2_CTRL_TYPE_BITMASK     , "bitmask"    },
        {V4L2_CTRL_TYPE_INTEGER_MENU, "integerMenu"}
#endif
    };

    return ctrlTypeToStr;
}

const DeviceControls &VCamV4L2LoopBackPrivate::deviceControls() const
{
    static const DeviceControls deviceControls = {
        {"Horizontal Flip"   , "boolean", 0, 1, 1, 0, {}           },
        {"Vertical Flip"     , "boolean", 0, 1, 1, 0, {}           },
        {"Scaling Mode"      , "menu"   , 0, 0, 1, 0, {"Fast",
                                                       "Linear"}   },
        {"Aspect Ratio Mode" , "menu"   , 0, 0, 1, 0, {"Ignore",
                                                       "Keep",
                                                       "Expanding"}},
        {"Swap Read and Blue", "boolean", 0, 1, 1, 0, {}           },
    };

    return deviceControls;
}

AkVideoCapsList VCamV4L2LoopBackPrivate::formatFps(int fd,
                                                   const v4l2_fmtdesc &format,
                                                   __u32 width,
                                                   __u32 height) const
{
    AkVideoCapsList caps;

#ifdef VIDIOC_ENUM_FRAMEINTERVALS
    v4l2_frmivalenum frmival;
    memset(&frmival, 0, sizeof(v4l2_frmivalenum));
    frmival.pixel_format = format.pixelformat;
    frmival.width = width;
    frmival.height = height;

    for (frmival.index = 0;
         this->xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) >= 0;
         frmival.index++) {
        if (!frmival.discrete.numerator
            || !frmival.discrete.denominator)
            continue;

        AkFrac fps;

        if (frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE)
            fps = AkFrac(frmival.discrete.denominator,
                         frmival.discrete.numerator);
        else
            fps = AkFrac(frmival.stepwise.min.denominator,
                         frmival.stepwise.max.numerator);

        caps << AkVideoCaps(this->formatByV4L2(format.pixelformat).ak,
                            int(width),
                            int(height),
                            fps);
    }

    if (caps.isEmpty()) {
#endif
        v4l2_streamparm params;
        memset(&params, 0, sizeof(v4l2_streamparm));
        params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (this->xioctl(fd, VIDIOC_G_PARM, &params) >= 0) {
            AkFrac fps;

            if (params.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)
                fps = AkFrac(params.parm.capture.timeperframe.denominator,
                             params.parm.capture.timeperframe.numerator);
            else
                fps = AkFrac(30, 1);

            caps << AkVideoCaps(this->formatByV4L2(format.pixelformat).ak,
                                int(width),
                                int(height),
                                fps);
        }
#ifdef VIDIOC_ENUM_FRAMEINTERVALS
    }
#endif

    return caps;
}

AkVideoCapsList VCamV4L2LoopBackPrivate::formats(int fd) const
{
    __u32 type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_capability capability;
    memset(&capability, 0, sizeof(v4l2_capability));

    if (this->xioctl(fd, VIDIOC_QUERYCAP, &capability) >= 0
        && capability.capabilities & V4L2_CAP_VIDEO_OUTPUT) {
        type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    }

    AkVideoCapsList caps;

#ifndef VIDIOC_ENUM_FRAMESIZES
    v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = type;
    uint width = 0;
    uint height = 0;

    // Check if it has at least a default format.
    if (this->xioctl(fd, VIDIOC_G_FMT, &fmt) >= 0) {
        width = fmt.fmt.pix.width;
        height = fmt.fmt.pix.height;
    }

    if (width <= 0 || height <= 0)
        return {};
#endif

    // Enumerate all supported formats.
    v4l2_fmtdesc fmtdesc;
    memset(&fmtdesc, 0, sizeof(v4l2_fmtdesc));
    fmtdesc.type = type;

    for (fmtdesc.index = 0;
         this->xioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) >= 0;
         fmtdesc.index++) {
#ifdef VIDIOC_ENUM_FRAMESIZES
        v4l2_frmsizeenum frmsize;
        memset(&frmsize, 0, sizeof(v4l2_frmsizeenum));
        frmsize.pixel_format = fmtdesc.pixelformat;

        // Enumerate frame sizes.
        for (frmsize.index = 0;
             this->xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0;
             frmsize.index++) {
            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                caps << this->formatFps(fd,
                                        fmtdesc,
                                        frmsize.discrete.width,
                                        frmsize.discrete.height);
            } else {
#if 0
                for (uint height = frmsize.stepwise.min_height;
                     height < frmsize.stepwise.max_height;
                     height += frmsize.stepwise.step_height)
                    for (uint width = frmsize.stepwise.min_width;
                         width < frmsize.stepwise.max_width;
                         width += frmsize.stepwise.step_width) {
                        caps << this->formatFps(fd, fmtdesc, width, height);
                    }
#endif
            }
        }
#else
        caps << this->capsFps(fd, fmtdesc, width, height);
#endif
    }

    return caps;
}

QList<QStringList> VCamV4L2LoopBackPrivate::combineMatrix(const QList<QStringList> &matrix) const
{
    QStringList combined;
    QList<QStringList> combinations;
    this->combineMatrixP(matrix, 0, combined, combinations);

    return combinations;
}

/* A matrix is a list of lists where each element in the main list is a row,
 * and each element in a row is a column. We combine each element in a row with
 * each element in the next rows.
 */
void VCamV4L2LoopBackPrivate::combineMatrixP(const QList<QStringList> &matrix,
                                              size_t index,
                                              QStringList &combined,
                                              QList<QStringList> &combinations) const
{
    if (index >= size_t(matrix.size())) {
        combinations << combined;

        return;
    }

    for (auto &column: matrix[int(index)]) {
        auto combined_p1 = combined + QStringList {column};
        this->combineMatrixP(matrix, index + 1, combined_p1, combinations);
    }
}

QList<AkVideoCapsList> VCamV4L2LoopBackPrivate::readFormats(QSettings &settings) const
{
    QList<AkVideoCapsList> formatsMatrix;
    QList<QStringList> strFormatsMatrix;
    settings.beginGroup("Formats");
    auto nFormats = settings.beginReadArray("formats");

    for (int i = 0; i < nFormats; i++) {
        settings.setArrayIndex(i);
        auto pixFormats = settings.value("format").toString().split(',');
        auto widths = settings.value("width").toString().split(',');
        auto heights = settings.value("height").toString().split(',');
        auto frameRates = settings.value("fps").toString().split(',');

        auto trim = [] (const QString &str) {
            return str.trimmed();
        };

        std::transform(pixFormats.begin(),
                       pixFormats.end(),
                       pixFormats.begin(), trim);
        std::transform(widths.begin(),
                       widths.end(),
                       widths.begin(), trim);
        std::transform(heights.begin(),
                       heights.end(),
                       heights.begin(), trim);
        std::transform(frameRates.begin(),
                       frameRates.end(),
                       frameRates.begin(), trim);

        if (pixFormats.empty()
            || widths.empty()
            || heights.empty()
            || frameRates.empty())
            continue;

        strFormatsMatrix << pixFormats;
        strFormatsMatrix << widths;
        strFormatsMatrix << heights;
        strFormatsMatrix << frameRates;
        auto combinedFormats = this->combineMatrix(strFormatsMatrix);
        AkVideoCapsList formats;

        for (auto &formatList: combinedFormats) {
            auto pixFormat = this->formatByStr(formatList[0].trimmed()).ak;
            auto width = formatList[1].trimmed().toUInt();
            auto height = formatList[2].trimmed().toUInt();
            auto fps = AkFrac(formatList[3]);
            AkVideoCaps format(pixFormat,
                               int(width),
                               int(height),
                               fps);

            if (format)
                formats << format;
        }

        formatsMatrix << formats;
    }

    settings.endArray();
    settings.endGroup();

    return formatsMatrix;
}

QList<DeviceInfo> VCamV4L2LoopBackPrivate::readDevicesConfigs() const
{
    QSettings settings(QCoreApplication::organizationName(),
                       "VirtualCamera");
    auto availableFormats = this->readFormats(settings);
    QList<DeviceInfo> devices;

    settings.beginGroup("Cameras");
    auto nCameras = settings.beginReadArray("cameras");

    for (int i = 0; i < nCameras; i++) {
        settings.setArrayIndex(i);
        auto description = settings.value("description").toString();
        auto driver = settings.value("driver").toString();
        auto bus = settings.value("bus").toString();
        auto formats = settings.value("formats").toStringList();
        AkVideoCapsList formatsList;

        for (auto &format: formats) {
            auto index = format.trimmed().toInt() - 1;

            if (index < 0 || index >= availableFormats.size())
                continue;

            if (!availableFormats[index].empty())
                formatsList << availableFormats[index];
        }

        if (!formatsList.isEmpty())
            devices << DeviceInfo {0,
                                   "",
                                   description,
                                   driver,
                                   bus,
                                   formatsList,
                                   {},
                                   DeviceTypeCapture};
    }

    settings.endArray();
    settings.endGroup();

    return devices;
}

AkVideoCapsList VCamV4L2LoopBackPrivate::formatsFromSettings(const QString &deviceId,
                                                             const QList<DeviceInfo> &devicesInfo) const
{
    int fd = open(deviceId.toStdString().c_str(), O_RDWR | O_NONBLOCK, 0);

    if (fd < 0)
        return {};

    QString driver;
    QString description;
    QString bus;
    v4l2_capability capability;
    memset(&capability, 0, sizeof(v4l2_capability));

    if (this->xioctl(fd, VIDIOC_QUERYCAP, &capability) >= 0) {
        driver = reinterpret_cast<const char *>(capability.driver);
        description = reinterpret_cast<const char *>(capability.card);
        bus = reinterpret_cast<const char *>(capability.bus_info);
    }

    close(fd);

    for (auto &devInfo: devicesInfo) {
        if (devInfo.driver.isEmpty()
            && devInfo.description.isEmpty()
            && devInfo.bus.isEmpty())
            continue;

        if ((devInfo.driver.isEmpty() || devInfo.driver == driver)
            && (devInfo.description.isEmpty() || devInfo.description == description)
            && (devInfo.bus.isEmpty() || devInfo.bus == bus))
            return devInfo.formats;
    }

    return {};
}

void VCamV4L2LoopBackPrivate::setFps(int fd, const v4l2_fract &fps)
{
    v4l2_streamparm streamparm;
    memset(&streamparm, 0, sizeof(v4l2_streamparm));
    streamparm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    if (this->xioctl(fd, VIDIOC_G_PARM, &streamparm) >= 0)
        if (streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
            streamparm.parm.capture.timeperframe.numerator = fps.denominator;
            streamparm.parm.capture.timeperframe.denominator = fps.numerator;
            this->xioctl(fd, VIDIOC_S_PARM, &streamparm);
        }
}

bool VCamV4L2LoopBackPrivate::initReadWrite(quint32 bufferSize)
{
    this->m_buffers.resize(1);
    this->m_buffers[0].length = bufferSize;
    this->m_buffers[0].start = new char[bufferSize];

    if (!this->m_buffers[0].start) {
        this->m_buffers.clear();

        return false;
    }

    memset(this->m_buffers[0].start, 0, bufferSize);

    return true;
}

bool VCamV4L2LoopBackPrivate::initMemoryMap()
{
    v4l2_requestbuffers requestBuffers;
    memset(&requestBuffers, 0, sizeof(v4l2_requestbuffers));
    requestBuffers.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    requestBuffers.memory = V4L2_MEMORY_MMAP;
    requestBuffers.count = __u32(this->m_nBuffers);

    if (this->xioctl(this->m_fd, VIDIOC_REQBUFS, &requestBuffers) < 0)
        return false;

    if (requestBuffers.count < 1)
        return false;

    this->m_buffers.resize(int(requestBuffers.count));
    bool error = false;

    for (int i = 0; i < int(requestBuffers.count); i++) {
        v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(v4l2_buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = __u32(i);

        if (this->xioctl(this->m_fd, VIDIOC_QUERYBUF, &buffer) < 0) {
            error = true;

            break;
        }

        this->m_buffers[i].length = buffer.length;
        this->m_buffers[i].start =
                reinterpret_cast<char *>(mmap(nullptr,
                                              buffer.length,
                                              PROT_READ | PROT_WRITE,
                                              MAP_SHARED,
                                              this->m_fd,
                                              buffer.m.offset));

        if (this->m_buffers[i].start == MAP_FAILED) {
            error = true;

            break;
        }
    }

    if (error) {
        for (auto &buffer: this->m_buffers)
            munmap(buffer.start, buffer.length);

        this->m_buffers.clear();

        return false;
    }

    return true;
}

bool VCamV4L2LoopBackPrivate::initUserPointer(quint32 bufferSize)
{
    v4l2_requestbuffers requestBuffers;
    memset(&requestBuffers, 0, sizeof(v4l2_requestbuffers));
    requestBuffers.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    requestBuffers.memory = V4L2_MEMORY_USERPTR;
    requestBuffers.count = __u32(this->m_nBuffers);

    if (this->xioctl(this->m_fd, VIDIOC_REQBUFS, &requestBuffers) < 0)
        return false;

    this->m_buffers.resize(int(requestBuffers.count));
    bool error = false;

    for (int i = 0; i < int(requestBuffers.count); i++) {
        this->m_buffers[i].length = bufferSize;
        this->m_buffers[i].start = new char[bufferSize];

        if (!this->m_buffers[i].start) {
            error = true;

            break;
        }

        memset(this->m_buffers[i].start, 0, bufferSize);
    }

    if (error) {
        for (auto &buffer: this->m_buffers)
            delete [] buffer.start;

        this->m_buffers.clear();

        return false;
    }

    return true;
}

void VCamV4L2LoopBackPrivate::initDefaultFormats()
{
    QVector<AkVideoCaps::PixelFormat> pixelFormats = {
        AkVideoCaps::Format_yuyv422,
        AkVideoCaps::Format_uyvy422,
        AkVideoCaps::Format_0rgb,
        AkVideoCaps::Format_rgb24,
    };
    QVector<QPair<int , int>> resolutions = {
        { 640,  480},
        { 160,  120},
        { 320,  240},
        { 800,  600},
        {1280,  720},
        {1920, 1080},
    };

    for (auto &format: pixelFormats)
        for (auto &resolution: resolutions)
            this->m_defaultFormats << AkVideoCaps(format,
                                                  resolution.first,
                                                  resolution.second,
                                                  {30, 1});
}

bool VCamV4L2LoopBackPrivate::startOutput()
{
    bool error = false;

    if (this->m_ioMethod == IoMethodMemoryMap) {
        for (int i = 0; i < this->m_buffers.size(); i++) {
            v4l2_buffer buffer;
            memset(&buffer, 0, sizeof(v4l2_buffer));
            buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
            buffer.memory = V4L2_MEMORY_MMAP;
            buffer.index = __u32(i);

            if (this->xioctl(this->m_fd, VIDIOC_QBUF, &buffer) < 0)
                error = true;
        }

        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

        if (this->xioctl(this->m_fd, VIDIOC_STREAMON, &type) < 0)
            error = true;
    } else if (this->m_ioMethod == IoMethodUserPointer) {
        for (int i = 0; i < this->m_buffers.size(); i++) {
            v4l2_buffer buffer;
            memset(&buffer, 0, sizeof(v4l2_buffer));
            buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
            buffer.memory = V4L2_MEMORY_USERPTR;
            buffer.index = __u32(i);
            buffer.m.userptr = ulong(this->m_buffers[i].start);
            buffer.length = __u32(this->m_buffers[i].length);

            if (this->xioctl(this->m_fd, VIDIOC_QBUF, &buffer) < 0)
                error = true;
        }

        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

        if (this->xioctl(this->m_fd, VIDIOC_STREAMON, &type) < 0)
            error = true;
    }

    if (error)
        self->uninit();

    return !error;
}

void VCamV4L2LoopBackPrivate::stopOutput()
{
    if (this->m_ioMethod == IoMethodMemoryMap
        || this->m_ioMethod == IoMethodUserPointer) {
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        this->xioctl(this->m_fd, VIDIOC_STREAMOFF, &type);
    }
}

void VCamV4L2LoopBackPrivate::updateDevices()
{
    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesFormats) devicesFormats;
    QStringList virtualDevices;

    QDir devicesDir("/dev");
    auto devicesFiles = this->v4l2Devices();

    for (auto &devicePath: devicesFiles) {
        auto fileName = devicesDir.absoluteFilePath(devicePath);
        int fd = open(fileName.toStdString().c_str(), O_RDWR | O_NONBLOCK, 0);

        if (fd < 0)
            continue;

        v4l2_capability capability;
        memset(&capability, 0, sizeof(v4l2_capability));

        if (this->xioctl(fd, VIDIOC_QUERYCAP, &capability) >= 0
            && capability.capabilities & V4L2_CAP_VIDEO_OUTPUT) {
            virtualDevices << fileName;
        }

        close(fd);
    }

    auto devicesInfo = this->readDevicesConfigs();

    for (auto &device: virtualDevices) {
        int fd = open(device.toStdString().c_str(), O_RDWR | O_NONBLOCK, 0);

        if (fd < 0)
            continue;

        auto formats = this->formatsFromSettings(device, devicesInfo);

        if (formats.empty())
            formats = this->m_defaultFormats;

        if (!formats.empty()) {
            v4l2_capability capability;
            memset(&capability, 0, sizeof(v4l2_capability));
            QString description;

            if (this->xioctl(fd, VIDIOC_QUERYCAP, &capability) >= 0)
                description = reinterpret_cast<const char *>(capability.card);

            devices << device;
            descriptions[device] = description;
            devicesFormats[device] = formats;
        }

        close(fd);
    }

    this->m_descriptions = descriptions;
    this->m_devicesFormats = devicesFormats;

    if (this->m_devices != devices) {
        if (!this->m_devices.isEmpty())
            this->m_fsWatcher->removePaths(this->m_devices);

        this->m_devices = devices;
#ifndef Q_OS_BSD4
        if (!this->m_devices.isEmpty())
            this->m_fsWatcher->addPaths(this->m_devices);
#endif
    }
}

QString VCamV4L2LoopBackPrivate::cleanDescription(const QString &description) const
{
    QString desc;

    for (auto &c: description)
        if (c < ' ' || QString("'\"\\,$`").contains(c))
            desc += ' ';
        else
            desc += c;

    desc = desc.simplified();

    if (desc.isEmpty())
        desc = "Virtual Camera";

    return desc;
}

QVector<int> VCamV4L2LoopBackPrivate::requestDeviceNR(size_t count) const
{
    QVector<int> nrs;

    for (int i = 0; i < MAX_CAMERAS && count > 0; i++)
        if (!QFileInfo::exists(QString("/dev/video%1").arg(i))) {
            nrs << i;
            count--;
        }

    return nrs;
}

bool VCamV4L2LoopBackPrivate::waitForDevice(const QString &deviceId) const
{
    /* udev can take some time to give proper file permissions to the device,
     * so we wait until el character device become fully accesible.
     */
    QElapsedTimer etimer;
    int fd = -1;
    etimer.start();

    while (etimer.elapsed() < 5000) {
        fd = open(deviceId.toStdString().c_str(), O_RDWR | O_NONBLOCK, 0);

        if (fd != -EPERM)
            break;

        QThread::msleep(500);
    }

    if (fd < 0)
        return false;

    close(fd);

    return true;
}

bool VCamV4L2LoopBackPrivate::waitForDevices(const QStringList &devices) const
{
    QElapsedTimer etimer;
    bool result = false;

    etimer.start();

    while (etimer.elapsed() < 5000) {
        auto devicesFiles = this->v4l2Devices();

        if (devicesFiles.size() == devices.size()) {
            result = true;

            break;
        }

        QThread::msleep(500);
    }

    return result;
}

QStringList VCamV4L2LoopBackPrivate::v4l2Devices() const
{
    QDir devicesDir("/dev");

    return devicesDir.entryList(QStringList() << "video*",
                                QDir::System
                                | QDir::Readable
                                | QDir::Writable
                                | QDir::NoSymLinks
                                | QDir::NoDotAndDotDot
                                | QDir::CaseSensitive,
                                QDir::Name);
}

QList<DeviceInfo> VCamV4L2LoopBackPrivate::devicesInfo() const
{
    QList<DeviceInfo> devices;
    QDir devicesDir("/dev");
    auto devicesFiles = this->v4l2Devices();

    for (auto &devicePath: devicesFiles) {
        auto fileName = devicesDir.absoluteFilePath(devicePath);
        int fd = open(fileName.toStdString().c_str(), O_RDWR | O_NONBLOCK, 0);

        if (fd < 0)
            continue;

        v4l2_capability capability;
        memset(&capability, 0, sizeof(v4l2_capability));

        if (this->xioctl(fd, VIDIOC_QUERYCAP, &capability) >= 0) {
            QString driver = reinterpret_cast<const char *>(capability.driver);

            if (driver == "v4l2 loopback")
                devices << DeviceInfo {
                    QString(fileName).remove("/dev/video").toInt(),
                    fileName,
                    reinterpret_cast<const char *>(capability.card),
                    reinterpret_cast<const char *>(capability.driver),
                    reinterpret_cast<const char *>(capability.bus_info),
                    {},
                    {},
                    (capability.capabilities
                     & (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_CAPTURE_MPLANE))?
                        DeviceTypeCapture: DeviceTypeOutput
                };
        }

        close(fd);
    }

    return devices;
}

inline QString VCamV4L2LoopBackPrivate::stringFromIoctl(ulong cmd) const
{
    static const QMap<ulong, QString> ioctlStrings {
#ifdef UVCIOC_CTRL_MAP
        {UVCIOC_CTRL_MAP           , "UVCIOC_CTRL_MAP"           },
#endif
#ifdef UVCIOC_CTRL_QUERY
        {UVCIOC_CTRL_QUERY         , "UVCIOC_CTRL_QUERY"         },
#endif
        {VIDIOC_QUERYCAP           , "VIDIOC_QUERYCAP"           },
#ifdef VIDIOC_RESERVED
        {VIDIOC_RESERVED           , "VIDIOC_RESERVED"           },
#endif
        {VIDIOC_ENUM_FMT           , "VIDIOC_ENUM_FMT"           },
        {VIDIOC_G_FMT              , "VIDIOC_G_FMT"              },
        {VIDIOC_S_FMT              , "VIDIOC_S_FMT"              },
        {VIDIOC_REQBUFS            , "VIDIOC_REQBUFS"            },
        {VIDIOC_QUERYBUF           , "VIDIOC_QUERYBUF"           },
        {VIDIOC_G_FBUF             , "VIDIOC_G_FBUF"             },
        {VIDIOC_S_FBUF             , "VIDIOC_S_FBUF"             },
        {VIDIOC_OVERLAY            , "VIDIOC_OVERLAY"            },
        {VIDIOC_QBUF               , "VIDIOC_QBUF"               },
        {VIDIOC_EXPBUF             , "VIDIOC_EXPBUF"             },
        {VIDIOC_DQBUF              , "VIDIOC_DQBUF"              },
        {VIDIOC_STREAMON           , "VIDIOC_STREAMON"           },
        {VIDIOC_STREAMOFF          , "VIDIOC_STREAMOFF"          },
        {VIDIOC_G_PARM             , "VIDIOC_G_PARM"             },
        {VIDIOC_S_PARM             , "VIDIOC_S_PARM"             },
        {VIDIOC_G_STD              , "VIDIOC_G_STD"              },
        {VIDIOC_S_STD              , "VIDIOC_S_STD"              },
        {VIDIOC_ENUMSTD            , "VIDIOC_ENUMSTD"            },
        {VIDIOC_ENUMINPUT          , "VIDIOC_ENUMINPUT"          },
        {VIDIOC_G_CTRL             , "VIDIOC_G_CTRL"             },
        {VIDIOC_S_CTRL             , "VIDIOC_S_CTRL"             },
        {VIDIOC_G_TUNER            , "VIDIOC_G_TUNER"            },
        {VIDIOC_S_TUNER            , "VIDIOC_S_TUNER"            },
        {VIDIOC_G_AUDIO            , "VIDIOC_G_AUDIO"            },
        {VIDIOC_S_AUDIO            , "VIDIOC_S_AUDIO"            },
        {VIDIOC_QUERYCTRL          , "VIDIOC_QUERYCTRL"          },
        {VIDIOC_QUERYMENU          , "VIDIOC_QUERYMENU"          },
        {VIDIOC_G_INPUT            , "VIDIOC_G_INPUT"            },
        {VIDIOC_S_INPUT            , "VIDIOC_S_INPUT"            },
        {VIDIOC_G_EDID             , "VIDIOC_G_EDID"             },
        {VIDIOC_S_EDID             , "VIDIOC_S_EDID"             },
        {VIDIOC_G_OUTPUT           , "VIDIOC_G_OUTPUT"           },
        {VIDIOC_S_OUTPUT           , "VIDIOC_S_OUTPUT"           },
        {VIDIOC_ENUMOUTPUT         , "VIDIOC_ENUMOUTPUT"         },
        {VIDIOC_G_AUDOUT           , "VIDIOC_G_AUDOUT"           },
        {VIDIOC_S_AUDOUT           , "VIDIOC_S_AUDOUT"           },
        {VIDIOC_G_MODULATOR        , "VIDIOC_G_MODULATOR"        },
        {VIDIOC_S_MODULATOR        , "VIDIOC_S_MODULATOR"        },
        {VIDIOC_G_FREQUENCY        , "VIDIOC_G_FREQUENCY"        },
        {VIDIOC_S_FREQUENCY        , "VIDIOC_S_FREQUENCY"        },
        {VIDIOC_CROPCAP            , "VIDIOC_CROPCAP"            },
        {VIDIOC_G_CROP             , "VIDIOC_G_CROP"             },
        {VIDIOC_S_CROP             , "VIDIOC_S_CROP"             },
        {VIDIOC_G_JPEGCOMP         , "VIDIOC_G_JPEGCOMP"         },
        {VIDIOC_S_JPEGCOMP         , "VIDIOC_S_JPEGCOMP"         },
        {VIDIOC_QUERYSTD           , "VIDIOC_QUERYSTD"           },
        {VIDIOC_TRY_FMT            , "VIDIOC_TRY_FMT"            },
        {VIDIOC_ENUMAUDIO          , "VIDIOC_ENUMAUDIO"          },
        {VIDIOC_ENUMAUDOUT         , "VIDIOC_ENUMAUDOUT"         },
        {VIDIOC_G_PRIORITY         , "VIDIOC_G_PRIORITY"         },
        {VIDIOC_S_PRIORITY         , "VIDIOC_S_PRIORITY"         },
        {VIDIOC_G_SLICED_VBI_CAP   , "VIDIOC_G_SLICED_VBI_CAP"   },
        {VIDIOC_LOG_STATUS         , "VIDIOC_LOG_STATUS"         },
        {VIDIOC_G_EXT_CTRLS        , "VIDIOC_G_EXT_CTRLS"        },
        {VIDIOC_S_EXT_CTRLS        , "VIDIOC_S_EXT_CTRLS"        },
        {VIDIOC_TRY_EXT_CTRLS      , "VIDIOC_TRY_EXT_CTRLS"      },
        {VIDIOC_ENUM_FRAMESIZES    , "VIDIOC_ENUM_FRAMESIZES"    },
        {VIDIOC_ENUM_FRAMEINTERVALS, "VIDIOC_ENUM_FRAMEINTERVALS"},
        {VIDIOC_G_ENC_INDEX        , "VIDIOC_G_ENC_INDEX"        },
        {VIDIOC_ENCODER_CMD        , "VIDIOC_ENCODER_CMD"        },
        {VIDIOC_TRY_ENCODER_CMD    , "VIDIOC_TRY_ENCODER_CMD"    },
        {VIDIOC_DBG_S_REGISTER     , "VIDIOC_DBG_S_REGISTER"     },
        {VIDIOC_DBG_G_REGISTER     , "VIDIOC_DBG_G_REGISTER"     },
        {VIDIOC_S_HW_FREQ_SEEK     , "VIDIOC_S_HW_FREQ_SEEK"     },
        {VIDIOC_S_DV_TIMINGS       , "VIDIOC_S_DV_TIMINGS"       },
        {VIDIOC_G_DV_TIMINGS       , "VIDIOC_G_DV_TIMINGS"       },
        {VIDIOC_DQEVENT            , "VIDIOC_DQEVENT"            },
        {VIDIOC_SUBSCRIBE_EVENT    , "VIDIOC_SUBSCRIBE_EVENT"    },
        {VIDIOC_UNSUBSCRIBE_EVENT  , "VIDIOC_UNSUBSCRIBE_EVENT"  },
        {VIDIOC_CREATE_BUFS        , "VIDIOC_CREATE_BUFS"        },
        {VIDIOC_PREPARE_BUF        , "VIDIOC_PREPARE_BUF"        },
        {VIDIOC_G_SELECTION        , "VIDIOC_G_SELECTION"        },
        {VIDIOC_S_SELECTION        , "VIDIOC_S_SELECTION"        },
        {VIDIOC_DECODER_CMD        , "VIDIOC_DECODER_CMD"        },
        {VIDIOC_TRY_DECODER_CMD    , "VIDIOC_TRY_DECODER_CMD"    },
        {VIDIOC_ENUM_DV_TIMINGS    , "VIDIOC_ENUM_DV_TIMINGS"    },
        {VIDIOC_QUERY_DV_TIMINGS   , "VIDIOC_QUERY_DV_TIMINGS"   },
        {VIDIOC_DV_TIMINGS_CAP     , "VIDIOC_DV_TIMINGS_CAP"     },
        {VIDIOC_ENUM_FREQ_BANDS    , "VIDIOC_ENUM_FREQ_BANDS"    },
        {VIDIOC_DBG_G_CHIP_INFO    , "VIDIOC_DBG_G_CHIP_INFO"    },
#ifdef VIDIOC_QUERY_EXT_CTRL
        {VIDIOC_QUERY_EXT_CTRL     , "VIDIOC_QUERY_EXT_CTRL"     },
#endif
    };

    return ioctlStrings.value(cmd, QString("VIDIOC_UNKNOWN(%1)").arg(cmd));
}

#include "moc_vcamv4l2lb.cpp"
