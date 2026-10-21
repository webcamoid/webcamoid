/* Webcamoid, camera capture application.
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
#include <QBuffer>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFileSystemWatcher>
#include <QImage>
#include <QMutex>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSettings>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QVersionNumber>
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

#include <ak.h>
#include <akalgorithm.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideoconverter.h>
#include <akvideoformatspec.h>

#include "vcamak.h"

#define MAX_CAMERAS 64

#define AKVCAM_RW_MODE_READWRITE 0x1U
#define AKVCAM_RW_MODE_MMAP      0x2U
#define AKVCAM_RW_MODE_USERPTR   0x4U

enum IoMethod
{
    IoMethodUnknown = -1,
    IoMethodReadWrite,
    IoMethodMemoryMap,
    IoMethodUserPointer
};

struct CaptureBuffer
{
    char *start[VIDEO_MAX_PLANES];
    size_t length[VIDEO_MAX_PLANES];
};

using RwMode = __u32;

struct DeviceInfo
{
    QString path;
    QString description;
    bool isOutput;
};

struct V4L2AkFormat
{
    uint32_t v4l2;
    AkVideoCaps::PixelFormat ak;
    QString str;
};

using V4L2AkFormatMap = QVector<V4L2AkFormat>;

class VCamAkPrivate
{
    public:
        VCamAk *self;
        QString m_device;
        mutable QMutex m_deviceMutex;
        QFileSystemWatcher *m_fsWatcher {nullptr};
        QVector<CaptureBuffer> m_buffers;
        QString m_error;
        QString m_picture;
        QString m_rootMethod {"pkexec"};
        v4l2_format m_v4l2Format;
        IoMethod m_ioMethod {IoMethodUnknown};
        int m_fd {-1};
        int m_nBuffers {32};

        explicit VCamAkPrivate(VCamAk *self);
        VCamAkPrivate(const VCamAkPrivate &other) = delete;
        ~VCamAkPrivate();

        inline int planesCount(const v4l2_format &format) const;
        inline int xioctl(int fd, ulong request, void *arg) const;
        bool sudo(const QString &script);
        QString whereBin(const QString &binary) const;
        QString sysfsControls(const QString &deviceId) const;
        QStringList connectedDevices(const QString &deviceId) const;
        QString readPicturePath() const;
        inline const V4L2AkFormatMap &v4l2AkFormatMap() const;
        inline const V4L2AkFormat &formatByV4L2(uint32_t v4l2) const;
        inline const V4L2AkFormat &formatByAk(AkVideoCaps::PixelFormat ak) const;
        inline const V4L2AkFormat &formatByStr(const QString &str) const;
        void setFps(int fd, __u32 bufferType, const v4l2_fract &fps);
        bool initReadWrite(const v4l2_format &format);
        bool initMemoryMap(const v4l2_format &format);
        bool initUserPointer(const v4l2_format &format);
        bool startOutput(const v4l2_format &format);
        void stopOutput(const v4l2_format &format);
        void copyFrame(char * const *planeData,
                       const AkVideoPacket &videoPacket);
        QString cleanDescription(const QString &description) const;
        QVector<int> requestDeviceNR(size_t count) const;
        bool waitForDevice(const QString &deviceId) const;
        inline QStringList v4l2Devices() const;
        QList<DeviceInfo> devicesInfo() const;
        QString findOutputDevice(const QString &description) const;
        void updateDevice();
        QString currentDevice() const;
        bool applyConfig(const QString &description, int deviceNR);
        inline QString stringFromIoctl(ulong cmd) const;
};

VCamAk::VCamAk(QObject *parent):
    VCam(parent)
{
    this->d = new VCamAkPrivate(this);
    this->d->m_picture = this->d->readPicturePath();
    QObject::connect(this,
                     &VCam::descriptionChanged,
                     this,
                     [this] () {
        this->d->updateDevice();
    });
}

VCamAk::~VCamAk()
{
    delete this->d;
}

QString VCamAk::error() const
{
    return this->d->m_error;
}

bool VCamAk::isInstalled() const
{
    static bool haveResult = false;
    static bool result = false;

    if (!haveResult) {
        static const char moduleName[] = "akvcam";

        if (Ak::isFlatpak()) {
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

QString VCamAk::installedVersion() const
{
    static bool haveVersion = false;
    static QString version;

    if (!haveVersion) {
        static const char moduleName[] = "akvcam";

        if (Ak::isFlatpak()) {
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

QString VCamAk::device() const
{
    return this->d->currentDevice();
}

QList<quint64> VCamAk::clientsPids() const
{
    auto devices = this->d->devicesInfo();
    QList<quint64> clientsPids;

    if (Ak::isFlatpak()) {
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
            if (device.isOutput)
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
                QString target = fdInfo.isSymLink()?
                                     fdInfo.symLinkTarget():
                                     fdInfo.absoluteFilePath();
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
    }

    std::sort(clientsPids.begin(), clientsPids.end());

    return clientsPids;
}

QString VCamAk::clientExe(quint64 pid) const
{
    if (Ak::isFlatpak()) {
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

QString VCamAk::picture() const
{
    return this->d->m_picture;
}

QString VCamAk::deviceCreate(const QString &description)
{
    this->d->m_error = "";

    if (!this->clientsPids().isEmpty()) {
        this->d->m_error = "The driver is in use";

        return {};
    }

    // The dynamic and elastic devices are supported since akvcam 2.0.0.
    auto curVersion = QVersionNumber::fromString(this->installedVersion());

    // If the version can't be parsed, don't block the creation.
    if (!curVersion.isNull() && curVersion < QVersionNumber(2, 0, 0)) {
        this->d->m_error = "akvcam 2.0.0 or newer is required";

        return {};
    }

    auto desc = this->d->cleanDescription(description);

    if (!this->d->findOutputDevice(desc).isEmpty()) {
        this->d->m_error = "The virtual camera already exists";

        return {};
    }

    auto deviceNR = this->d->requestDeviceNR(1);

    if (deviceNR.isEmpty()) {
        this->d->m_error = "No available devices to create a virtual camera";

        return {};
    }

    // This replaces any previous configuration, including old layouts with
    // many cameras.
    if (!this->d->applyConfig(desc, deviceNR[0]))
        return {};

    auto deviceId = QString("/dev/video%1").arg(deviceNR[0]);

    if (!this->d->waitForDevice(deviceId)) {
        this->d->m_error = "Time exceeded while waiting for the device";

        return {};
    }

    this->setDescription(desc);
    this->d->updateDevice();

    return deviceId;
}

bool VCamAk::changeDescription(const QString &description)
{
    this->d->m_error = "";

    if (this->d->currentDevice().isEmpty()) {
        this->d->m_error = "The virtual camera was not created";

        return false;
    }

    if (!this->clientsPids().isEmpty()) {
        this->d->m_error = "The driver is in use";

        return false;
    }

    auto deviceId = this->d->currentDevice();
    int deviceNR = QString(deviceId).remove("/dev/video").toInt();
    auto desc = this->d->cleanDescription(description);

    if (!this->d->applyConfig(desc, deviceNR))
        return false;

    if (!this->d->waitForDevice(deviceId)) {
        this->d->m_error = "Time exceeded while waiting for the device";

        return false;
    }

    this->setDescription(desc);
    this->d->updateDevice();

    return true;
}

bool VCamAk::deviceDestroy()
{
    this->d->m_error = "";

    if (!this->clientsPids().isEmpty()) {
        this->d->m_error = "The driver is in use";

        return false;
    }

    // Write the script file.

    QString script;
    QTextStream ts(&script);
    ts << "rmmod akvcam 2>/dev/null" << Qt::endl;
    ts << "sed -i '/akvcam/d' /etc/modules 2>/dev/null" << Qt::endl;
    ts << "sed -i '/akvcam/d' /etc/modules-load.d/*.conf 2>/dev/null" << Qt::endl;
    ts << "sed -i '/akvcam/d' /etc/modprobe.d/*.conf 2>/dev/null" << Qt::endl;
    ts << "rm -f /etc/modules-load.d/akvcam.conf" << Qt::endl;
    ts << "rm -f /etc/modprobe.d/akvcam.conf" << Qt::endl;
    ts << "rm -f /etc/akvcam/config.ini" << Qt::endl;

    if (!this->d->sudo(script))
        return false;

    this->d->updateDevice();

    return true;
}

bool VCamAk::openDevice(AkVideoCaps &caps)
{
    this->d->m_error = "";
    this->d->m_ioMethod = IoMethodUnknown;

    if (this->d->currentDevice().isEmpty()) {
        this->d->m_error = "The virtual camera was not created";

        return false;
    }

    // Frames written must be blocking so we does not waste CPU time.
    this->d->m_fd = ::open(this->d->currentDevice().toStdString().c_str(),
                           O_RDWR | O_NONBLOCK,
                           0);

    if (this->d->m_fd < 0) {
        this->d->m_error = "Can't open the virtual camera";

        return false;
    }

    v4l2_capability capabilities;
    memset(&capabilities, 0, sizeof(v4l2_capability));

    if (this->d->xioctl(this->d->m_fd, VIDIOC_QUERYCAP, &capabilities) < 0) {
        qDebug() << "VirtualCamera:  Can't query capabilities.";
        ::close(this->d->m_fd);
        this->d->m_fd = -1;

        return false;
    }

    // The dynamic formats of the driver accepts any of the mapped formats.
    // If the format is unknown, fallback to the most compatible one.
    auto v4l2PixelFormat = this->d->formatByAk(caps.format()).v4l2;

    if (v4l2PixelFormat == 0) {
        caps = AkVideoCaps(AkVideoCaps::Format_yuyv422,
                           caps.width(),
                           caps.height(),
                           caps.fps());
        v4l2PixelFormat = this->d->formatByAk(caps.format()).v4l2;
    }

    // The driver always use the single-planar API.
    v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    this->d->xioctl(this->d->m_fd, VIDIOC_G_FMT, &fmt);
    fmt.fmt.pix.pixelformat = v4l2PixelFormat;
    fmt.fmt.pix.width = __u32(caps.width());
    fmt.fmt.pix.height = __u32(caps.height());

    if (this->d->xioctl(this->d->m_fd, VIDIOC_S_FMT, &fmt) < 0) {
        qDebug() << "VirtualCamera: Can't set format:" << caps;
        this->d->m_error = "Can't set the output format";
        ::close(this->d->m_fd);
        this->d->m_fd = -1;

        return false;
    }

    memcpy(&this->d->m_v4l2Format, &fmt, sizeof(v4l2_format));

    // The driver corrects the values that it doesn't support.
    auto akFormat = this->d->formatByV4L2(fmt.fmt.pix.pixelformat).ak;

    if (akFormat == AkVideoCaps::Format_none)
        akFormat = caps.format();

    auto fps = caps.fps();
    this->d->setFps(this->d->m_fd,
                    fmt.type,
                    {__u32(fps.num()), __u32(fps.den())});

    v4l2_streamparm streamparm;
    memset(&streamparm, 0, sizeof(v4l2_streamparm));
    streamparm.type = fmt.type;

    if (this->d->xioctl(this->d->m_fd, VIDIOC_G_PARM, &streamparm) >= 0) {
        auto &tpf = streamparm.parm.output.timeperframe;

        if (tpf.numerator > 0 && tpf.denominator > 0)
            fps = AkFrac(tpf.denominator, tpf.numerator);
    }

    caps = AkVideoCaps(akFormat,
                       int(fmt.fmt.pix.width),
                       int(fmt.fmt.pix.height),
                       fps);

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
        else {
            this->d->m_error = "Can't initialize the output buffers";
            ::close(this->d->m_fd);
            this->d->m_fd = -1;

            return false;
        }
    }

    if (!this->d->startOutput(fmt)) {
        this->d->m_error = "Can't start the output";
        this->closeDevice();

        return false;
    }

    return true;
}

void VCamAk::closeDevice()
{
    if (this->d->m_fd < 0)
        return;

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

    ::close(this->d->m_fd);
    this->d->m_fd = -1;
    this->d->m_buffers.clear();
}

void VCamAk::setPicture(const QString &picture)
{
    if (this->d->m_picture == picture)
        return;

    this->d->m_picture = picture;
    emit this->pictureChanged(this->d->m_picture);
}

bool VCamAk::applyPicture()
{
    this->d->m_error = "";

    if (!this->clientsPids().isEmpty()) {
        this->d->m_error = "The driver is in use";

        return false;
    }

    // Without a virtual camera the picture will be used on its creation.
    if (this->d->currentDevice().isEmpty())
        return true;

    auto deviceId = this->d->currentDevice();
    int deviceNR = QString(deviceId).remove("/dev/video").toInt();

    if (!this->d->applyConfig(this->description(), deviceNR))
        return false;

    if (!this->d->waitForDevice(deviceId)) {
        this->d->m_error = "Time exceeded while waiting for the device";

        return false;
    }

    this->d->updateDevice();

    return true;
}

bool VCamAk::writeFrame(const AkVideoPacket &videoPacket)
{
    if (this->d->m_buffers.isEmpty() || this->d->m_fd < 0)
        return false;

    if (this->d->m_ioMethod == IoMethodReadWrite) {
        this->d->copyFrame(this->d->m_buffers[0].start, videoPacket);
        int planesCount = this->d->planesCount(this->d->m_v4l2Format);

        for (int i = 0; i < planesCount; i++) {
            if (::write(this->d->m_fd,
                        this->d->m_buffers[0].start[i],
                        this->d->m_buffers[0].length[i]) < 0)
                return false;
        }

        return true;
    }

    if (this->d->m_ioMethod == IoMethodMemoryMap
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
            this->d->copyFrame(this->d->m_buffers[int(buffer.index)].start,
                               videoPacket);

        return this->d->xioctl(this->d->m_fd, VIDIOC_QBUF, &buffer) >= 0;
    }

    return false;
}

VCamAkPrivate::VCamAkPrivate(VCamAk *self):
    self(self)
{
    memset(&this->m_v4l2Format, 0, sizeof(v4l2_format));
    this->m_fsWatcher = new QFileSystemWatcher({"/dev"}, self);
    QObject::connect(this->m_fsWatcher,
                     &QFileSystemWatcher::directoryChanged,
                     this->self,
                     [this] () {
        this->updateDevice();
    });
    this->updateDevice();
}

VCamAkPrivate::~VCamAkPrivate()
{
}

int VCamAkPrivate::xioctl(int fd, ulong request, void *arg) const
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

int VCamAkPrivate::planesCount(const v4l2_format &format) const
{
    return format.type == V4L2_BUF_TYPE_VIDEO_OUTPUT?
                1:
                format.fmt.pix_mp.num_planes;
}

bool VCamAkPrivate::sudo(const QString &script)
{
    if (this->m_rootMethod.isEmpty()) {
        const QString msg = "Root method not set";
        qDebug() << msg;
        this->m_error += msg + " ";

        return false;
    }

    QProcess su;

    if (Ak::isFlatpak()) {
        su.start("flatpak-spawn", QStringList {"--host", this->m_rootMethod, "sh"});
    } else {
        auto sudoBin = this->whereBin(this->m_rootMethod);

        if (sudoBin.isEmpty()) {
            const QString msg = "Can't find " + this->m_rootMethod;
            qDebug() << msg;
            this->m_error += msg + " ";

            return false;
        }

        auto shBin = this->whereBin("sh");

        if (shBin.isEmpty()) {
            const QString msg = "Can't find default shell";
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

QString VCamAkPrivate::whereBin(const QString &binary) const
{
    // Limit search paths to trusted directories only.
    static const QStringList paths {
        "/usr/bin",        // GNU/Linux
        "/bin",            // NetBSD
        "/usr/local/bin",  // FreeBSD
        "/usr/local/sbin", // FreeBSD

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

QString VCamAkPrivate::sysfsControls(const QString &deviceId) const
{
    auto sysfsPath = deviceId;
    sysfsPath = sysfsPath.replace("/dev/video",
                                  "/sys/devices/virtual/video4linux/video");
    sysfsPath += "/controls";

    return QFileInfo::exists(sysfsPath + "/connected_devices")?
                sysfsPath: QString();
}

QStringList VCamAkPrivate::connectedDevices(const QString &deviceId) const
{
    auto sysfsControls = this->sysfsControls(deviceId);

    if (sysfsControls.isEmpty())
        return {};

    sysfsControls += "/connected_devices";

    if (!QFileInfo::exists(sysfsControls))
        return {};

    QFile connectedDevices(sysfsControls);
    QStringList devices;

    if (connectedDevices.open(QIODevice::ReadOnly | QIODevice::Text))
        for (auto &device: connectedDevices.readAll().split('\n')) {
            auto dev = device.trimmed();

            if (!dev.isEmpty())
                devices << dev;
        }

    return devices;
}

QString VCamAkPrivate::readPicturePath() const
{
    QSettings settings("/etc/akvcam/config.ini", QSettings::IniFormat);

    return settings.value("default_frame").toString();
}

const V4L2AkFormatMap &VCamAkPrivate::v4l2AkFormatMap() const
{
    static const V4L2AkFormatMap formatMap {
        {0                  , AkVideoCaps::Format_none    , ""},

        // RGB formats
        {V4L2_PIX_FMT_RGB32 , AkVideoCaps::Format_xrgb    , "RGB32"},
        {V4L2_PIX_FMT_RGB24 , AkVideoCaps::Format_rgb24   , "RGB24"},

        // BGR formats
        {V4L2_PIX_FMT_BGR32 , AkVideoCaps::Format_xbgr    , "BGR32"},
        {V4L2_PIX_FMT_BGR24 , AkVideoCaps::Format_bgr24   , "BGR24"},

        // YUV formats
        {V4L2_PIX_FMT_UYVY  , AkVideoCaps::Format_uyvy422 , "UYVY"},
        {V4L2_PIX_FMT_YUYV  , AkVideoCaps::Format_yuyv422 , "YUY2"},
        {V4L2_PIX_FMT_YVYU  , AkVideoCaps::Format_yvyu422 , "YVYU"},
        {V4L2_PIX_FMT_VYUY  , AkVideoCaps::Format_vyuy422 , "VYUY"},

        // Packed RGB formats with alpha channel
        {V4L2_PIX_FMT_RGB565, AkVideoCaps::Format_rgb565le, "RGB16"},
        {V4L2_PIX_FMT_RGB555, AkVideoCaps::Format_rgb555le, "RGB15"},
#ifdef V4L2_PIX_FMT_RGB565X
        {V4L2_PIX_FMT_RGB565X, AkVideoCaps::Format_rgb565be, "RGB565X"},
#endif
#ifdef V4L2_PIX_FMT_RGB555X
        {V4L2_PIX_FMT_RGB555X, AkVideoCaps::Format_rgb555be, "RGB555X"},
#endif
#ifdef V4L2_PIX_FMT_ARGB32
        {V4L2_PIX_FMT_ARGB32, AkVideoCaps::Format_argb    , "ARGB32"},
#endif
#ifdef V4L2_PIX_FMT_ABGR32
        {V4L2_PIX_FMT_ABGR32, AkVideoCaps::Format_abgr    , "ABGR32"},
#endif
#ifdef V4L2_PIX_FMT_BGRA32
        {V4L2_PIX_FMT_BGRA32, AkVideoCaps::Format_bgra    , "BGRA32"},
#endif
#ifdef V4L2_PIX_FMT_BGRX32
        {V4L2_PIX_FMT_BGRX32, AkVideoCaps::Format_bgrx    , "BGRX32"},
#endif
#ifdef V4L2_PIX_FMT_RGBA32
        {V4L2_PIX_FMT_RGBA32, AkVideoCaps::Format_rgba    , "RGBA32"},
#endif
#ifdef V4L2_PIX_FMT_RGBX32
        {V4L2_PIX_FMT_RGBX32, AkVideoCaps::Format_rgbx    , "RGBX32"},
#endif

        // Planar formats, they are stored in a single contiguous buffer
        {V4L2_PIX_FMT_NV12  , AkVideoCaps::Format_nv12    , "NV12"},
        {V4L2_PIX_FMT_NV21  , AkVideoCaps::Format_nv21    , "NV21"},
        {V4L2_PIX_FMT_NV16  , AkVideoCaps::Format_nv16    , "NV16"},
        {V4L2_PIX_FMT_NV61  , AkVideoCaps::Format_nv61    , "NV61"},
        {V4L2_PIX_FMT_YUV420, AkVideoCaps::Format_yuv420p , "YUV420"},
        {V4L2_PIX_FMT_YVU420, AkVideoCaps::Format_yvu420p , "YVU420"},
        {V4L2_PIX_FMT_YUV422P, AkVideoCaps::Format_yuv422p, "YUV422P"},
        {V4L2_PIX_FMT_YUV410, AkVideoCaps::Format_yuv410p , "YUV410"},
        {V4L2_PIX_FMT_YVU410, AkVideoCaps::Format_yvu410p , "YVU410"},
        {V4L2_PIX_FMT_YUV411P, AkVideoCaps::Format_yuv411p, "YUV411P"},
    };

    return formatMap;
}

const V4L2AkFormat &VCamAkPrivate::formatByV4L2(uint32_t v4l2) const
{
    auto &formatMap = this->v4l2AkFormatMap();

    for (auto &format: formatMap)
        if (format.v4l2 == v4l2)
            return format;

    return formatMap.first();
}

const V4L2AkFormat &VCamAkPrivate::formatByAk(AkVideoCaps::PixelFormat ak) const
{
    auto &formatMap = this->v4l2AkFormatMap();

    for (auto &format: formatMap)
        if (format.ak == ak)
            return format;

    return formatMap.first();
}

const V4L2AkFormat &VCamAkPrivate::formatByStr(const QString &str) const
{
    auto &formatMap = this->v4l2AkFormatMap();

    for (auto &format: formatMap)
        if (format.str == str)
            return format;

    return formatMap.first();
}

void VCamAkPrivate::setFps(int fd,
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

bool VCamAkPrivate::initReadWrite(const v4l2_format &format)
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

bool VCamAkPrivate::initMemoryMap(const v4l2_format &format)
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

bool VCamAkPrivate::initUserPointer(const v4l2_format &format)
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

bool VCamAkPrivate::startOutput(const v4l2_format &format)
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

    return !error;
}

void VCamAkPrivate::stopOutput(const v4l2_format &format)
{
    if (this->m_ioMethod == IoMethodMemoryMap
        || this->m_ioMethod == IoMethodUserPointer) {
        auto type = v4l2_buf_type(format.type);
        this->xioctl(this->m_fd, VIDIOC_STREAMOFF, &type);
    }
}

void VCamAkPrivate::copyFrame(char * const *planeData,
                              const AkVideoPacket &videoPacket)
{
    /* The driver uses the single-planar API, so all the planes of the frame
     * are stored one after the other in a single buffer.
     */
    auto oData = planeData[0];
    size_t capacity = this->m_v4l2Format.fmt.pix.sizeimage;
    size_t height = this->m_v4l2Format.fmt.pix.height;
    size_t oLineSize0 = this->m_v4l2Format.fmt.pix.bytesperline;
    size_t used0 = videoPacket.bytesUsed(0);
    size_t offset = 0;

    for (size_t plane = 0; plane < videoPacket.planes(); ++plane) {
        size_t used = videoPacket.bytesUsed(plane);
        size_t iLineSize = videoPacket.lineSize(plane);
        size_t oLineSize = used0 > 0? oLineSize0 * used / used0: used;
        size_t lines = height >> videoPacket.heightDiv(plane);

        if (capacity > 0 && offset + oLineSize * lines > capacity)
            break;

        auto iData = videoPacket.constPlane(int(plane));

        if (iLineSize == oLineSize) {
            memcpy(oData + offset, iData, oLineSize * lines);
        } else {
            auto lineSize = qMin(used, oLineSize);

            for (size_t y = 0; y < lines; ++y)
                memcpy(oData + offset + y * oLineSize,
                       iData + y * iLineSize,
                       lineSize);
        }

        offset += oLineSize * lines;
    }
}

QString VCamAkPrivate::cleanDescription(const QString &description) const
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

QVector<int> VCamAkPrivate::requestDeviceNR(size_t count) const
{
    QVector<int> nrs;

    for (int i = 0; i < MAX_CAMERAS && count > 0; i++)
        if (!QFileInfo::exists(QString("/dev/video%1").arg(i))) {
            nrs << i;
            count--;
        }

    return nrs;
}

bool VCamAkPrivate::waitForDevice(const QString &deviceId) const
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

QStringList VCamAkPrivate::v4l2Devices() const
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

QList<DeviceInfo> VCamAkPrivate::devicesInfo() const
{
    QList<DeviceInfo> devices;
    QDir devicesDir("/dev");
    auto devicesFiles = this->v4l2Devices();

    for (auto &devicePath: devicesFiles) {
        auto fileName = devicesDir.absoluteFilePath(devicePath);
        int fd = ::open(fileName.toStdString().c_str(), O_RDWR | O_NONBLOCK, 0);

        if (fd < 0)
            continue;

        v4l2_capability capability;
        memset(&capability, 0, sizeof(v4l2_capability));

        if (this->xioctl(fd, VIDIOC_QUERYCAP, &capability) >= 0) {
            QString driver = reinterpret_cast<const char *>(capability.driver);

            if (driver == "akvcam")
                devices << DeviceInfo {
                    fileName,
                    reinterpret_cast<const char *>(capability.card),
                    !(capability.capabilities
                      & (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_CAPTURE_MPLANE))
                };
        }

        ::close(fd);
    }

    return devices;
}

QString VCamAkPrivate::findOutputDevice(const QString &description) const
{
    if (description.isEmpty())
        return {};

    // V4L2 limits the name of the device to 31 characters.
    auto card = this->cleanDescription(description).left(31);

    for (auto &device: this->devicesInfo())
        if (device.isOutput && device.description == card)
            return device.path;

    return {};
}

void VCamAkPrivate::updateDevice()
{
    auto device = this->findOutputDevice(this->self->description());

    {
        QMutexLocker locker(&this->m_deviceMutex);

        if (this->m_device == device)
            return;

        this->m_device = device;
    }

    emit this->self->deviceChanged(device);
}

QString VCamAkPrivate::currentDevice() const
{
    QMutexLocker locker(&this->m_deviceMutex);

    return this->m_device;
}

bool VCamAkPrivate::applyConfig(const QString &description, int deviceNR)
{
    /* The virtual camera is a single elastic output device with dynamic
     * formats in direct mode, the driver creates the capture devices on
     * demand with the same format that the output device is using.
     */
    QTemporaryDir tempDir;
    QSettings settings(tempDir.path() + "/config.ini", QSettings::IniFormat);

    // Write 'config.ini'.
    settings.beginGroup("Cameras");
    settings.beginWriteArray("cameras");
    settings.setArrayIndex(0);
    settings.setValue("type", "output");
    settings.setValue("mode", QStringList {"mmap", "userptr"});
    settings.setValue("description", description);
    settings.setValue("videonr", deviceNR);
    settings.setValue("elastic", true);
    settings.setValue("direct_mode", true);
    settings.endArray();
    settings.endGroup();

    // Copy default frame to file system.
    QImage defaultImage;
    QBuffer imageBuffer;

    if (!this->m_picture.isEmpty() && defaultImage.load(this->m_picture)) {
        defaultImage = defaultImage.convertToFormat(QImage::Format_RGB888);
        auto width = AkAlgorithm::alignUp(defaultImage.width(), 32);
        defaultImage = defaultImage.scaled(width,
                                           defaultImage.height(),
                                           Qt::IgnoreAspectRatio,
                                           Qt::SmoothTransformation);

        if (imageBuffer.open(QIODevice::WriteOnly)
            && defaultImage.save(&imageBuffer, "BMP"))
            settings.setValue("default_frame", "/etc/akvcam/default_frame.bmp");
        else
            settings.setValue("default_frame", "");
    } else {
        settings.setValue("default_frame", "");
    }

    settings.sync();

    // Write the script file.

    QString script;
    QTextStream ts(&script);
    ts << "rmmod akvcam 2>/dev/null" << Qt::endl;
    ts << "sed -i '/akvcam/d' /etc/modules 2>/dev/null" << Qt::endl;
    ts << "sed -i '/akvcam/d' /etc/modules-load.d/*.conf 2>/dev/null" << Qt::endl;
    ts << "sed -i '/akvcam/d' /etc/modprobe.d/*.conf 2>/dev/null" << Qt::endl;
    ts << "echo akvcam > /etc/modules-load.d/akvcam.conf" << Qt::endl;

#ifdef QT_DEBUG
    ts << "echo options akvcam loglevel=7 > /etc/modprobe.d/akvcam.conf" << Qt::endl;
#else
    ts << "rm -f /etc/modprobe.d/akvcam.conf" << Qt::endl;
#endif

    ts << "mkdir -p /etc/akvcam" << Qt::endl;

    if (!defaultImage.isNull() && !imageBuffer.data().isEmpty())
        ts << "echo '" << imageBuffer.data().toBase64() << "' | base64 -d - > /etc/akvcam/default_frame.bmp" << Qt::endl;

    // Create a heredoc with the configuration.
    ts << "cat << EOF > /etc/akvcam/config.ini" << Qt::endl;

    QFile configIni(settings.fileName());

    if (configIni.open(QIODevice::ReadOnly | QIODevice::Text))
        ts << configIni.readAll();

    ts << "EOF" << Qt::endl;

#ifdef QT_DEBUG
    ts << "modprobe akvcam loglevel=7" << Qt::endl;
#else
    ts << "modprobe akvcam" << Qt::endl;
#endif

    // Execute the script
    return this->sudo(script);
}

inline QString VCamAkPrivate::stringFromIoctl(ulong cmd) const
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

#include "moc_vcamak.cpp"
