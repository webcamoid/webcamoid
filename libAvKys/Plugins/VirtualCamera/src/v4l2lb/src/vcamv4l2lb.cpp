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
#include <QSettings>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTextCodec>
#include <QThread>
#include <fcntl.h>
#include <limits>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifdef HAVE_LIBKMOD
#include <libkmod.h>
#endif

#include <akelement.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <akvideoconverter.h>
#include <akvideoformatspec.h>

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
    char *start[VIDEO_MAX_PLANES];
    size_t length[VIDEO_MAX_PLANES];
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
        AkElementPtr m_swapRBFilter {akPluginManager->create<AkElement>("VideoFilter/SwapRB")};
        QString m_error;
        AkVideoCaps m_currentCaps;
        AkVideoConverter m_videoConverter;
        QString m_rootMethod;
        v4l2_format m_v4l2Format;
        IoMethod m_ioMethod {IoMethodUnknown};
        int m_fd {-1};
        int m_nBuffers {32};

        explicit VCamV4L2LoopBackPrivate(VCamV4L2LoopBack *self);
        VCamV4L2LoopBackPrivate(const VCamV4L2LoopBackPrivate &other) = delete;
        ~VCamV4L2LoopBackPrivate();

        inline int xioctl(int fd, ulong request, void *arg) const;
        inline int planesCount(const v4l2_format &format) const;
        bool isFlatpak() const;
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
        QList<QStringList> combineMatrix(const QList<QStringList> &matrix) const;
        void combineMatrixP(const QList<QStringList> &matrix,
                            size_t index,
                            QStringList &combined,
                            QList<QStringList> &combinations) const;
        QList<AkVideoCapsList> readFormats(QSettings &settings) const;
        QList<DeviceInfo> readDevicesConfigs() const;
        AkVideoCapsList formatsFromSettings(const QString &deviceId,
                                            const QList<DeviceInfo> &devicesInfo) const;
        void setFps(int fd, __u32 bufferType, const v4l2_fract &fps);
        bool initReadWrite(const v4l2_format &format);
        bool initMemoryMap(const v4l2_format &format);
        bool initUserPointer(const v4l2_format &format);
        void initDefaultFormats();
        bool startOutput(const v4l2_format &format);
        void stopOutput(const v4l2_format &format);
        void writeFrame(char * const *planeData,
                        const AkVideoPacket &videoPacket);
        void updateDevices();
        QString cleanDescription(const QString &description) const;
        QVector<int> requestDeviceNR(size_t count) const;
        bool waitForDevice(const QString &deviceId) const;
        bool waitForDevices(const QStringList &devices) const;
        inline QStringList v4l2Devices() const;
        QList<DeviceInfo> devicesInfo() const;
        inline QString stringFromIoctl(ulong cmd) const;
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
    static bool haveResult = false;
    static bool result = false;

    if (!haveResult) {
        static const char moduleName[] = "v4l2loopback";

        if (this->d->isFlatpak()) {
            QProcess modinfo;
            modinfo.start("flatpak-spawn",
                          QStringList {"--host",
                                       "modinfo",
                                       "-F",
                                       "version",
                                       moduleName});
            modinfo.waitForFinished(-1);
            result = modinfo.exitCode() == 0;
        } else {
            auto modules = QString("/lib/modules/%1/modules.dep")
                           .arg(QSysInfo::kernelVersion());
            QFile file(modules);

            if (file.open(QIODevice::ReadOnly)) {
                forever {
                    auto line = file.readLine();

                    if (line.isEmpty())
                        break;

                    auto driver = QFileInfo(line.left(line.indexOf(':'))).baseName();

                    if (driver == moduleName) {
                        result = true;

                        break;
                    }
                }
            }
        }

        haveResult = true;
    }

    return result;
}

QString VCamV4L2LoopBack::installedVersion() const
{
    static bool haveVersion = false;
    static QString version;

    if (!haveVersion) {
        static const char moduleName[] = "v4l2loopback";

        if (this->d->isFlatpak()) {
            QProcess modinfo;
            modinfo.start("flatpak-spawn",
                          QStringList {"--host",
                                       "modinfo",
                                       "-F",
                                       "version",
                                       moduleName});
            modinfo.waitForFinished(-1);

            if (modinfo.exitCode() == 0)
                version = QString::fromUtf8(modinfo.readAllStandardOutput().trimmed());
        } else {
#ifdef HAVE_LIBKMOD
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
#else
            auto modinfoBin = this->d->whereBin("modinfo");

            if (!modinfoBin.isEmpty()) {
                QProcess modinfo;
                modinfo.start(modinfoBin, QStringList {"-F", "version", moduleName});
                modinfo.waitForFinished(-1);

                if (modinfo.exitCode() == 0)
                    version = QString::fromUtf8(modinfo.readAllStandardOutput().trimmed());
            }
#endif
        }

        haveVersion = true;
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
    return this->d->m_devicesFormats.value(deviceId);
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

    if (this->d->isFlatpak()) {
        QProcess find;
        find.start("flatpak-spawn",
                   QStringList {"--host",
                                "find",
                                "/proc",
                                "-regex",
                                "/proc/[0-9]+/fd/[0-9]+"});
        find.waitForFinished(-1);
        auto fdsStr = find.readAll();
        QList<quint64> pids;

        for (auto &fd: fdsStr.trimmed().split('\n')) {
            auto pid = fd.split('/').value(2).toULongLong();
            pids << pid;
        }

        QProcess xargs;
        xargs.start("flatpak-spawn",
                    QStringList {"--host",
                                 "xargs",
                                 "realpath",
                                 "-m"});

        if (xargs.waitForStarted()) {
            xargs.write(fdsStr);
            xargs.closeWriteChannel();
        }

        xargs.waitForFinished(-1);
        QStringList  files;

        while (!xargs.atEnd())
            files << xargs.readLine().trimmed();

        for (auto &device: devices) {
            if (device.type == DeviceTypeOutput)
                continue;

            for (size_t i = 0; i < pids.size(); i++)
                if (files.value(i) == device.path) {
                    auto pid = pids[i];

                    if (pid != 0 && !clientsPids.contains(pid))
                        clientsPids << pid;
                }
        }
    } else {
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

                if (QRegExp("/dev/video[0-9]+").exactMatch(target))
                    videoDevices << target;
            }

            for (auto &device: devices)
                if (videoDevices.contains(device.path)) {
                    clientsPids << pid;

                    break;
                }
        }
    }

    std::sort(clientsPids.begin(), clientsPids.end());

    return clientsPids;
}

QString VCamV4L2LoopBack::clientExe(quint64 pid) const
{
    if (this->d->isFlatpak()) {
        QProcess realpath;
        realpath.start("flatpak-spawn",
                       QStringList {"--host",
                                    "realpath",
                                    QString("/proc/%1/exe").arg(pid)});
        realpath.waitForFinished(-1);

        if (realpath.exitCode() != 0)
            return {};

        return QString::fromUtf8(realpath.readAll().trimmed());
    }

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

    auto outputFormats = this->d->m_devicesFormats.value(this->d->m_device);

    if (outputFormats.empty()) {
        qDebug() << "VirtualCamera: Output formats were not configured";
        close(this->d->m_fd);
        this->d->m_fd = -1;

        return false;
    }

    AkVideoCaps outputCaps = this->d->m_currentCaps.nearest(outputFormats);

    if (!outputCaps) {
        qDebug() << "VirtualCamera: Can't find a similar format:"
                 << this->d->m_currentCaps;
        close(this->d->m_fd);
        this->d->m_fd = -1;

        return false;
    }

    auto v4l2PixelFormat = this->d->formatByAk(outputCaps.format()).v4l2;
    int width = outputCaps.width();
    int height = outputCaps.height();
    auto specs = AkVideoCaps::formatSpecs(outputCaps.format());

    v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = specs.planes() > 1?
                   V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
                   V4L2_BUF_TYPE_VIDEO_OUTPUT;
    this->d->xioctl(this->d->m_fd, VIDIOC_G_FMT, &fmt);

    if (fmt.type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
        fmt.fmt.pix.pixelformat = v4l2PixelFormat;
        fmt.fmt.pix.width = width;
        fmt.fmt.pix.height = height;
    } else {
        fmt.fmt.pix_mp.pixelformat = v4l2PixelFormat;
        fmt.fmt.pix_mp.width = width;
        fmt.fmt.pix_mp.height = height;
    }

    if (this->d->xioctl(this->d->m_fd, VIDIOC_S_FMT, &fmt) < 0) {
        qDebug() << "VirtualCamera: Can't set format:"
                 << outputCaps;
        close(this->d->m_fd);
        this->d->m_fd = -1;

        return false;
    }

    memcpy(&this->d->m_v4l2Format, &fmt, sizeof(v4l2_format));
    v4l2_fract fps = {__u32(outputCaps.fps().num()),
                      __u32(outputCaps.fps().den())};
    this->d->setFps(this->d->m_fd, fmt.type, fps);
    this->d->m_videoConverter.setOutputCaps(outputCaps);

    if (this->d->m_ioMethod == IoMethodReadWrite
        && capabilities.capabilities & V4L2_CAP_READWRITE
        && this->d->initReadWrite(fmt)) {
    } else if (this->d->m_ioMethod == IoMethodMemoryMap
             && capabilities.capabilities & V4L2_CAP_STREAMING
             && this->d->initMemoryMap(fmt)) {
    } else if (this->d->m_ioMethod == IoMethodUserPointer
             && capabilities.capabilities & V4L2_CAP_STREAMING
             && this->d->initUserPointer(fmt)) {
    } else
        this->d->m_ioMethod = IoMethodUnknown;

    if (this->d->m_ioMethod != IoMethodUnknown)
        return this->d->startOutput(fmt);

    if (capabilities.capabilities & V4L2_CAP_STREAMING) {
        if (this->d->initMemoryMap(fmt))
            this->d->m_ioMethod = IoMethodMemoryMap;
        else if (this->d->initUserPointer(fmt))
            this->d->m_ioMethod = IoMethodUserPointer;
    }

    if (this->d->m_ioMethod == IoMethodUnknown) {
        if (capabilities.capabilities & V4L2_CAP_READWRITE
            && this->d->initReadWrite(fmt))
            this->d->m_ioMethod = IoMethodReadWrite;
        else
            return false;
    }

    return this->d->startOutput(fmt);
}

void VCamV4L2LoopBack::uninit()
{
    this->d->stopOutput(this->d->m_v4l2Format);
    int planesCount = this->d->planesCount(this->d->m_v4l2Format);

    if (!this->d->m_buffers.isEmpty()) {
        if (this->d->m_ioMethod == IoMethodReadWrite) {
            for (auto &buffer: this->d->m_buffers)
                for (int i = 0; i < planesCount; i++)
                    delete [] buffer.start[i];
        } else if (this->d->m_ioMethod == IoMethodMemoryMap) {
            for (auto &buffer: this->d->m_buffers)
                for (int i = 0; i < planesCount; i++)
                    munmap(buffer.start[i], buffer.length[i]);
        } else if (this->d->m_ioMethod == IoMethodUserPointer) {
            for (auto &buffer: this->d->m_buffers)
                for (int i = 0; i < planesCount; i++)
                    delete [] buffer.start[i];
        }
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

    this->d->m_videoConverter.setScalingMode(AkVideoConverter::ScalingMode(values.value("Scaling Mode", 0)));
    this->d->m_videoConverter.setAspectRatioMode(AkVideoConverter::AspectRatioMode(values.value("Aspect Ratio Mode", 0)));
    this->d->m_videoConverter.begin();
    auto videoPacket = this->d->m_videoConverter.convert(packet_);
    this->d->m_videoConverter.end();

    if (!videoPacket)
        return false;

    if (this->d->m_ioMethod == IoMethodReadWrite) {
        this->d->writeFrame(this->d->m_buffers[0].start, videoPacket);
        int planesCount = this->d->planesCount(this->d->m_v4l2Format);

        for (int i = 0; i < planesCount; i++) {
            if (::write(this->d->m_fd,
                           this->d->m_buffers[0].start[i],
                           this->d->m_buffers[0].length[i]) < 0)
                return false;
        }
    } else if (this->d->m_ioMethod == IoMethodMemoryMap
        || this->d->m_ioMethod == IoMethodUserPointer) {
        v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(v4l2_buffer));
        buffer.type = this->d->m_v4l2Format.type;
        buffer.memory = (this->d->m_ioMethod == IoMethodMemoryMap)?
                            V4L2_MEMORY_MMAP:
                            V4L2_MEMORY_USERPTR;

        if (this->d->xioctl(this->d->m_fd, VIDIOC_DQBUF, &buffer) < 0)
            return false;

        if (buffer.index < quint32(this->d->m_buffers.size()))
            this->d->writeFrame(this->d->m_buffers[int(buffer.index)].start,
                    videoPacket);

        return this->d->xioctl(this->d->m_fd, VIDIOC_QBUF, &buffer) >= 0;
    }

    return false;
}

VCamV4L2LoopBackPrivate::VCamV4L2LoopBackPrivate(VCamV4L2LoopBack *self):
    self(self)
{
    this->initDefaultFormats();
    this->m_fsWatcher = new QFileSystemWatcher({"/dev"}, self);
    QObject::connect(this->m_fsWatcher,
                     &QFileSystemWatcher::directoryChanged,
                     self,
                     [this] () {
        this->updateDevices();
    });
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

int VCamV4L2LoopBackPrivate::planesCount(const v4l2_format &format) const
{
    return format.type == V4L2_BUF_TYPE_VIDEO_OUTPUT?
                1:
                format.fmt.pix_mp.num_planes;
}

bool VCamV4L2LoopBackPrivate::isFlatpak() const
{
    static const bool isFlatpak = QFile::exists("/.flatpak-info");

    return isFlatpak;
}

bool VCamV4L2LoopBackPrivate::sudo(const QString &script)
{
    if (this->m_rootMethod.isEmpty()) {
        static const QString msg = "Root method not set";
        qDebug() << msg;
        this->m_error += msg + " ";

        return false;
    }

    QProcess su;

    if (this->isFlatpak()) {
        su.start("flatpak-spawn", QStringList {"--host", this->m_rootMethod, "sh"});
    } else {
        auto sudoBin = this->whereBin(this->m_rootMethod);

        if (sudoBin.isEmpty()) {
            static const QString msg = "Can't find " + this->m_rootMethod;
            qDebug() << msg;
            this->m_error += msg + " ";

            return false;
        }

        auto shBin = this->whereBin("sh");

        if (shBin.isEmpty()) {
            static const QString msg = "Can't find default shell";
            qDebug() << msg;
            this->m_error += msg + " ";

            return false;
        }

        su.start(sudoBin, QStringList {shBin});
    }

    if (su.waitForStarted()) {
       qDebug() << "executing shell script with 'sh'"
                << Qt::endl
                << script.toUtf8().toStdString().c_str();
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
    static bool haveMethods = false;
    static QStringList methods;

    if (!haveMethods) {
        static const QStringList sus {
            "pkexec",
        };

        methods.clear();

        if (this->isFlatpak()) {
            for (auto &su: sus) {
                QProcess suProc;
                suProc.start("flatpak-spawn",
                             QStringList {"--host",
                                          su,
                                          "--version"});
                suProc.waitForFinished(-1);

                if (suProc.exitCode() == 0)
                    methods << su;
            }
        } else {
            for (auto &su: sus)
                if (!this->whereBin(su).isEmpty())
                    methods << su;
        }

        haveMethods = true;
    }

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
    static const V4L2AkFormatMap formatMap {
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

void VCamV4L2LoopBackPrivate::setFps(int fd,
                                     __u32 bufferType,
                                     const v4l2_fract &fps)
{
    v4l2_streamparm streamparm;
    memset(&streamparm, 0, sizeof(v4l2_streamparm));
    streamparm.type = bufferType;

    if (this->xioctl(fd, VIDIOC_G_PARM, &streamparm) >= 0)
        if (streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
            streamparm.parm.capture.timeperframe.numerator = fps.denominator;
            streamparm.parm.capture.timeperframe.denominator = fps.numerator;
            this->xioctl(fd, VIDIOC_S_PARM, &streamparm);
        }
}

bool VCamV4L2LoopBackPrivate::initReadWrite(const v4l2_format &format)
{
    int planesCount = this->planesCount(format);
    this->m_buffers.resize(1);
    bool error = false;

    for (auto &buffer: this->m_buffers)
        for (int i = 0; i < planesCount; i++) {
            buffer.length[i] = format.fmt.pix.sizeimage;
            buffer.start[i] = new char[format.fmt.pix.sizeimage];

            if (!buffer.start[i]) {
                error = true;

                break;
            }

            memset(buffer.start[i], 0, buffer.length[i]);
        }

    if (error) {
        for (auto &buffer: this->m_buffers)
            for (int i = 0; i < planesCount; i++)
                if (buffer.start[i])
                    delete [] buffer.start[i];

        this->m_buffers.clear();

        return false;
    }

    return true;
}

bool VCamV4L2LoopBackPrivate::initMemoryMap(const v4l2_format &format)
{
    v4l2_requestbuffers requestBuffers;
    memset(&requestBuffers, 0, sizeof(v4l2_requestbuffers));
    requestBuffers.type = format.type;
    requestBuffers.memory = V4L2_MEMORY_MMAP;
    requestBuffers.count = __u32(this->m_nBuffers);

    if (this->xioctl(this->m_fd, VIDIOC_REQBUFS, &requestBuffers) < 0)
        return false;

    if (requestBuffers.count < 1)
        return false;

    int planesCount = this->planesCount(format);

    if (planesCount < 1)
        return false;

    this->m_buffers.resize(int(requestBuffers.count));
    bool error = false;

    for (int i = 0; i < int(requestBuffers.count); i++) {
        v4l2_plane planes[planesCount];
        memset(planes, 0, planesCount * sizeof(v4l2_plane));

        v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(v4l2_buffer));
        buffer.type = format.type;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = __u32(i);

        if (format.type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
            buffer.length = planesCount;
            buffer.m.planes = planes;
        }

        if (this->xioctl(this->m_fd, VIDIOC_QUERYBUF, &buffer) < 0) {
            error = true;

            break;
        }

        if (format.type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
            this->m_buffers[i].length[0] = buffer.length;
            this->m_buffers[i].start[0] =
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
        } else {
            for (int j = 0; j < planesCount; j++) {
                this->m_buffers[i].length[j] = buffer.m.planes[j].length;
                this->m_buffers[i].start[j] =
                        reinterpret_cast<char *>(mmap(nullptr,
                                                      buffer.m.planes[j].length,
                                                      PROT_READ | PROT_WRITE,
                                                      MAP_SHARED,
                                                      this->m_fd,
                                                      buffer.m.planes[j].m.mem_offset));

                if(this->m_buffers[i].start[j] == MAP_FAILED){
                    error = true;

                    break;
                }
            }

            if (error)
                break;
        }
    }

    if (error) {
        for (auto &buffer: this->m_buffers)
            for (int i = 0; i < planesCount; i++)
                if (buffer.start[i] != MAP_FAILED)
                    munmap(buffer.start[i], buffer.length[i]);

        this->m_buffers.clear();

        return false;
    }

    return true;
}

bool VCamV4L2LoopBackPrivate::initUserPointer(const v4l2_format &format)
{
    v4l2_requestbuffers requestBuffers;
    memset(&requestBuffers, 0, sizeof(v4l2_requestbuffers));
    requestBuffers.type = format.type;
    requestBuffers.memory = V4L2_MEMORY_USERPTR;
    requestBuffers.count = __u32(this->m_nBuffers);

    if (this->xioctl(this->m_fd, VIDIOC_REQBUFS, &requestBuffers) < 0)
        return false;

    int planesCount = this->planesCount(format);
    this->m_buffers.resize(int(requestBuffers.count));
    bool error = false;

    for (int i = 0; i < int(requestBuffers.count); i++) {
        if (format.type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
            this->m_buffers[i].length[0] = format.fmt.pix.sizeimage;
            this->m_buffers[i].start[0] = new char[format.fmt.pix.sizeimage];

            if (!this->m_buffers[i].start[0]) {
                error = true;

                break;
            }

            memset(this->m_buffers[i].start[0], 0, format.fmt.pix.sizeimage);
        } else {
            for (int j = 0; j < format.fmt.pix_mp.num_planes; j++) {
                auto imageSize = format.fmt.pix_mp.plane_fmt[i].sizeimage;
                this->m_buffers[i].length[i] = imageSize;
                this->m_buffers[i].start[i] = new char[imageSize];

                if (!this->m_buffers[i].start[i]) {
                    error = true;

                    break;
                }

                memset(this->m_buffers[i].start[i], 0, imageSize);
            }

            if (error)
                break;
        }
    }

    if (error) {
        for (auto &buffer: this->m_buffers)
            for (int i = 0; i < planesCount; i++)
                if (buffer.start[i])
                    delete [] buffer.start[i];

        this->m_buffers.clear();

        return false;
    }

    return true;
}

void VCamV4L2LoopBackPrivate::initDefaultFormats()
{
    static const QVector<AkVideoCaps::PixelFormat> pixelFormats {
        AkVideoCaps::Format_yuyv422,
        AkVideoCaps::Format_uyvy422,
        AkVideoCaps::Format_0rgb,
        AkVideoCaps::Format_rgb24,
    };
    static const QVector<QPair<int , int>> resolutions {
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

bool VCamV4L2LoopBackPrivate::startOutput(const v4l2_format &format)
{
    bool error = false;

    if (this->m_ioMethod == IoMethodMemoryMap) {
        for (int i = 0; i < this->m_buffers.size(); i++) {
            v4l2_buffer buffer;
            memset(&buffer, 0, sizeof(v4l2_buffer));
            buffer.type = format.type;
            buffer.memory = V4L2_MEMORY_MMAP;
            buffer.index = __u32(i);

            if (this->xioctl(this->m_fd, VIDIOC_QBUF, &buffer) < 0)
                error = true;
        }

        auto type = v4l2_buf_type(format.type);

        if (this->xioctl(this->m_fd, VIDIOC_STREAMON, &type) < 0)
            error = true;
    } else if (this->m_ioMethod == IoMethodUserPointer) {
        int planesCount = this->planesCount(format);

        if (planesCount > 0) {
            for (int i = 0; i < this->m_buffers.size(); i++) {
                v4l2_buffer buffer;
                memset(&buffer, 0, sizeof(v4l2_buffer));
                buffer.type = format.type;
                buffer.memory = V4L2_MEMORY_USERPTR;
                buffer.index = __u32(i);

                if (this->m_v4l2Format.type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
                    buffer.m.userptr = ulong(this->m_buffers[i].start[0]);
                    buffer.length = __u32(this->m_buffers[i].length[0]);
                } else {
                    v4l2_plane planes[planesCount];
                    memset(planes, 0, planesCount * sizeof(v4l2_plane));
                    buffer.length = format.fmt.pix_mp.num_planes;
                    buffer.m.planes = planes;

                    for (int j = 0; j < buffer.length; j++) {
                        planes[j].m.userptr = ulong(this->m_buffers[i].start[j]);
                        planes[j].length = __u32(this->m_buffers[i].length[j]);
                    }
                }

                if (this->xioctl(this->m_fd, VIDIOC_QBUF, &buffer) < 0)
                    error = true;
            }

            auto type = v4l2_buf_type(format.type);

            if (this->xioctl(this->m_fd, VIDIOC_STREAMON, &type) < 0)
                error = true;
        } else {
            error = true;
        }
    }

    if (error)
        self->uninit();

    return !error;
}

void VCamV4L2LoopBackPrivate::stopOutput(const v4l2_format &format)
{
    if (this->m_ioMethod == IoMethodMemoryMap
        || this->m_ioMethod == IoMethodUserPointer) {
        auto type = v4l2_buf_type(format.type);
        this->xioctl(this->m_fd, VIDIOC_STREAMOFF, &type);
    }
}

void VCamV4L2LoopBackPrivate::writeFrame(char * const *planeData,
                                         const AkVideoPacket &videoPacket)
{
    if (this->m_v4l2Format.type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
        auto oData = planeData[0];
        auto iLineSize = videoPacket.lineSize(0);
        auto oLineSize = this->m_v4l2Format.fmt.pix.bytesperline;
        auto lineSize = qMin<size_t>(iLineSize, oLineSize);

        for (int y = 0; y < this->m_v4l2Format.fmt.pix.height; ++y)
            memcpy(oData + y * oLineSize,
                   videoPacket.constLine(0, y),
                   lineSize);
    } else {
        for (int plane = 0; plane < this->planesCount(this->m_v4l2Format); ++plane) {
            auto oData = planeData[plane];
            auto oLineSize = this->m_v4l2Format.fmt.pix_mp.plane_fmt[plane].bytesperline;
            auto iLineSize = videoPacket.lineSize(plane);
            auto lineSize = qMin<size_t>(iLineSize, oLineSize);
            auto heightDiv = videoPacket.heightDiv(plane);

            for (int y = 0; y < this->m_v4l2Format.fmt.pix_mp.height; ++y) {
                int ys = y >> heightDiv;
                memcpy(oData + ys * oLineSize,
                       videoPacket.constLine(plane, y),
                       lineSize);
            }
        }
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
#ifdef VIDIOC_ENUM_FRAMESIZES
        {VIDIOC_ENUM_FRAMESIZES    , "VIDIOC_ENUM_FRAMESIZES"    },
#endif
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
