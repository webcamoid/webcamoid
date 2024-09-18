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
#include <QImage>
#include <QMap>
#include <QMutex>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QVariant>
#include <QWaitCondition>
#include <QXmlStreamReader>
#include <akfrac.h>
#include <akvideoconverter.h>
#include <windows.h>
#include <shlobj.h>

#include "vcamdshow.h"

using DShowAkFormatMap = QMap<AkVideoCaps::PixelFormat, QString>;

struct DeviceFormat
{
    AkVideoCaps::PixelFormat format {AkVideoCaps::Format_none};
    int width {0};
    int height {0};
    AkFrac fps;
};

struct StreamProcess
{
    HANDLE stdinReadPipe {nullptr};
    HANDLE stdinWritePipe {nullptr};
    SECURITY_ATTRIBUTES pipeAttributes;
    STARTUPINFOA startupInfo;
    PROCESS_INFORMATION procInfo;
    QMutex stdinMutex;
};

struct DeviceControl
{
    QString id;
    QString description;
    QString type;
    int minimum {0};
    int maximum {0};
    int step {0};
    int defaultValue {0};
    int value {0};
    QStringList menu;
};

struct DeviceInfo
{
    QString description;
    AkVideoCapsList formats;
};

class VCamDShowPrivate
{
    public:
        VCamDShow *self;
        QString m_device;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, AkVideoCapsList> m_devicesFormats;
        QList<AkVideoCaps::PixelFormat> m_supportedOutputPixelFormats;
        AkVideoCaps::PixelFormat m_defaultOutputPixelFormat;
        QVariantList m_globalControls;
        QVariantMap m_localControls;
        QProcess *m_eventsProc {nullptr};
        StreamProcess m_streamProc;
        AkVideoCaps m_curFormat;
        QMutex m_controlsMutex;
        QString m_error;
        AkVideoCaps m_currentCaps;
        AkVideoConverter m_videoConverter;
        QString m_picture;
        QString m_rootMethod;
        bool m_isInitialized {false};
        bool m_runEventsProc {true};

        VCamDShowPrivate(VCamDShow *self=nullptr);
        ~VCamDShowPrivate();

        QStringList availableRootMethods() const;
        QString whereBin(const QString &binary) const;
        inline const DShowAkFormatMap &dshowAkFormatMap() const;
        void fillSupportedFormats();
        QVariantMap controlStatus(const QVariantList &controls) const;
        QVariantMap mapDiff(const QVariantMap &map1,
                            const QVariantMap &map2) const;
        QVariantList controls(const QString &device);
        bool setControls(const QString &device,
                         const QVariantMap &controls);
        QString readPicturePath() const;
        QString servicePath(const QString &serviceName) const;
        QString manager(const QString &arch={}) const;
        void updateDevices();
        template<typename T>
        static inline T alignUp(const T &value, const T &align)
        {
            return (value + align - 1) & ~(align - 1);
        }

        // Execute commands with elevated privileges.
        int sudo(const QStringList &parameters,
                 const QString &directory={},
                 bool show=false);
};

VCamDShow::VCamDShow(QObject *parent):
    VCam(parent)
{
    this->d = new VCamDShowPrivate(this);
    QStringList preferredRootMethod {
        "runas",
    };

    auto availableMethods = this->d->availableRootMethods();

    for (auto &method: preferredRootMethod)
        if (availableMethods.contains(method)) {
            this->d->m_rootMethod = method;

            break;
        }
}

VCamDShow::~VCamDShow()
{
    this->uninit();
    delete this->d;
}

QString VCamDShow::error() const
{
    return this->d->m_error;
}

bool VCamDShow::isInstalled() const
{
    return !this->d->manager().isEmpty();
}

QString VCamDShow::installedVersion() const
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return {};

    QProcess proc;
    proc.start(manager, {"-p", "-v"});
    proc.waitForFinished();

    if (proc.exitCode())
        return {};

    return proc.readAllStandardOutput().trimmed();
}

QStringList VCamDShow::webcams() const
{
    return this->d->m_devices;
}

QString VCamDShow::device() const
{
    return this->d->m_device;
}

QString VCamDShow::description(const QString &deviceId) const
{
    return this->d->m_descriptions.value(deviceId);
}

QList<AkVideoCaps::PixelFormat> VCamDShow::supportedOutputPixelFormats() const
{
    return this->d->m_supportedOutputPixelFormats;
}

AkVideoCaps::PixelFormat VCamDShow::defaultOutputPixelFormat() const
{
    return this->d->m_defaultOutputPixelFormat;
}

AkVideoCapsList VCamDShow::caps(const QString &deviceId) const
{
    if (!this->d->m_devicesFormats.contains(deviceId))
        return {};

    return this->d->m_devicesFormats[deviceId];
}

AkVideoCaps VCamDShow::currentCaps() const
{
    return this->d->m_currentCaps;
}

QVariantList VCamDShow::controls() const
{
    QVariantList controls;

    for (auto &control: this->d->m_globalControls)
        controls << QVariant(control.toList().mid(1));

    return controls;
}

bool VCamDShow::setControls(const QVariantMap &controls)
{
    this->d->m_controlsMutex.lock();
    auto globalControls = this->d->m_globalControls;
    this->d->m_controlsMutex.unlock();

    for (int i = 0; i < globalControls.count(); i++) {
        auto control = globalControls[i].toList();
        auto controlDescription = control[1].toString();

        if (controls.contains(controlDescription)) {
            control[7] = controls[controlDescription];
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

    if (!this->d->m_streamProc.stdinReadPipe)
        this->d->setControls(this->d->m_device, controls);

    emit this->controlsChanged(controls);

    return true;
}

QList<quint64> VCamDShow::clientsPids() const
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

        if (pid && pid != qApp->applicationPid())
            pids << quint64(pid);
    }

    return pids;
}

QString VCamDShow::clientExe(quint64 pid) const
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

QString VCamDShow::picture() const
{
    return this->d->m_picture;
}

QString VCamDShow::rootMethod() const
{
    return this->d->m_rootMethod;
}

QStringList VCamDShow::availableRootMethods() const
{
    return this->d->availableRootMethods();
}

QString VCamDShow::deviceCreate(const QString &description,
                                const AkVideoCapsList &formats)
{
    this->d->m_error = "";
    auto manager = this->d->manager();

    if (manager.isEmpty()) {
        this->d->m_error = "Manager not found";

        return {};
    }

    // Read devices information.
    QList<DeviceInfo> devices;

    for (auto it = this->d->m_descriptions.begin();
         it != this->d->m_descriptions.end();
         it++) {
        devices << DeviceInfo {it.value(), this->d->m_devicesFormats[it.key()]};
    }

    // Create capture device.
    devices << DeviceInfo {description, formats};

    QTemporaryDir tempDir;
    QSettings settings(tempDir.path() + "/config.ini", QSettings::IniFormat);

    // Write 'config.ini'.
    int i = 0;
    int j = 0;

    for (auto &device: devices) {
        QStringList formatsIndex;

        for (int i = 0; i < device.formats.size(); i++)
            formatsIndex << QString("%1").arg(i + j + 1);

        settings.beginGroup("Cameras");
        settings.beginWriteArray("cameras");
        settings.setArrayIndex(i);

        settings.setValue("description", device.description);
        settings.setValue("formats", formatsIndex);
        settings.endArray();
        settings.endGroup();

        settings.beginGroup("Formats");
        settings.beginWriteArray("formats");

        for (auto &format: device.formats) {
            settings.setArrayIndex(j);
            settings.setValue("format", this->d->dshowAkFormatMap().value(format.format()));
            settings.setValue("width", format.width());
            settings.setValue("height", format.height());
            settings.setValue("fps", format.fps().toString());
            j++;
        }

        settings.endArray();
        settings.endGroup();

        i++;
    }

    // Copy default frame to file system.
    QImage defaultImage;
    QString defaultFrame;

    if (!this->d->m_picture.isEmpty()
        && defaultImage.load(this->d->m_picture)) {
        auto dataLocation =
                QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).value(0);
        dataLocation += QDir::separator() + qApp->applicationName();

        if (QDir().mkpath(dataLocation)) {
            defaultImage = defaultImage.convertToFormat(QImage::Format_RGB888);
            auto width = VCamDShowPrivate::alignUp(defaultImage.width(), 32);
            defaultImage = defaultImage.scaled(width,
                                               defaultImage.height(),
                                               Qt::IgnoreAspectRatio,
                                               Qt::SmoothTransformation);
            defaultFrame = dataLocation
                         + QDir::separator()
                         + "default_frame.png";

            if (!defaultImage.save(defaultFrame))
                defaultFrame.clear();
        }
    }

    settings.setValue("default_frame", defaultFrame);

#ifdef QT_DEBUG
    settings.setValue("loglevel", "7");
#endif

    settings.sync();
    int exitCode = this->d->sudo({manager, "-f", "load", settings.fileName()});

    if (exitCode) {
        auto errorMsg = QString("Manager exited with code %1").arg(exitCode);
        qDebug() << errorMsg.toStdString().c_str();
        this->d->m_error += errorMsg;
    }

    QString deviceId;

    if (!exitCode) {
        QProcess proc;
        proc.start(manager, {"-p", "devices"});
        proc.waitForFinished();

        if (!proc.exitCode()) {
            QStringList virtualDevices;

            for (auto &line: proc.readAllStandardOutput().split('\n'))
                virtualDevices << line.trimmed();

            if (virtualDevices.size() >= devices.size())
                deviceId = virtualDevices.last();
        }
    }

    return deviceId;
}

bool VCamDShow::deviceEdit(const QString &deviceId,
                           const QString &description,
                           const AkVideoCapsList &formats)
{
    this->d->m_error = "";
    auto manager = this->d->manager();

    if (manager.isEmpty()) {
        this->d->m_error = "Manager not found";

        return false;
    }

    // Read devices information.
    QList<DeviceInfo> devices;

    for (auto it = this->d->m_descriptions.begin();
         it != this->d->m_descriptions.end();
         it++) {
        if (it.key() == deviceId)
            devices << DeviceInfo {description, formats};
        else
            devices << DeviceInfo {it.value(), this->d->m_devicesFormats[it.key()]};
    }

    QTemporaryDir tempDir;
    QSettings settings(tempDir.path() + "/config.ini", QSettings::IniFormat);

    // Write 'config.ini'.
    int i = 0;
    int j = 0;

    for (auto &device: devices) {
        QStringList formatsIndex;

        for (int i = 0; i < device.formats.size(); i++)
            formatsIndex << QString("%1").arg(i + j + 1);

        settings.beginGroup("Cameras");
        settings.beginWriteArray("cameras");
        settings.setArrayIndex(i);

        settings.setValue("description", device.description);
        settings.setValue("formats", formatsIndex);
        settings.endArray();
        settings.endGroup();

        settings.beginGroup("Formats");
        settings.beginWriteArray("formats");

        for (auto &format: device.formats) {
            settings.setArrayIndex(j);
            settings.setValue("format", this->d->dshowAkFormatMap().value(format.format()));
            settings.setValue("width", format.width());
            settings.setValue("height", format.height());
            settings.setValue("fps", format.fps().toString());
            j++;
        }

        settings.endArray();
        settings.endGroup();

        i++;
    }

    // Copy default frame to file system.
    QImage defaultImage;
    QString defaultFrame;

    if (!this->d->m_picture.isEmpty()
        && defaultImage.load(this->d->m_picture)) {
        auto dataLocation =
                QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).value(0);
        dataLocation += QDir::separator() + qApp->applicationName();

        if (QDir().mkpath(dataLocation)) {
            defaultImage = defaultImage.convertToFormat(QImage::Format_RGB888);
            auto width = VCamDShowPrivate::alignUp(defaultImage.width(), 32);
            defaultImage = defaultImage.scaled(width,
                                               defaultImage.height(),
                                               Qt::IgnoreAspectRatio,
                                               Qt::SmoothTransformation);
            defaultFrame = dataLocation
                         + QDir::separator()
                         + "default_frame.png";

            if (!defaultImage.save(defaultFrame))
                defaultFrame.clear();
        }
    }

    settings.setValue("default_frame", defaultFrame);

#ifdef QT_DEBUG
    settings.setValue("loglevel", "7");
#endif

    settings.sync();

    bool ok = true;
    int exitCode = this->d->sudo({manager, "-f", "load", settings.fileName()});

    if (exitCode) {
        ok = false;
        auto errorMsg = QString("Manager exited with code %1").arg(exitCode);
        qDebug() << errorMsg.toStdString().c_str();
        this->d->m_error += errorMsg;
    }

    return ok;
}

bool VCamDShow::changeDescription(const QString &deviceId,
                                  const QString &description)
{
    this->d->m_error = "";
    auto manager = this->d->manager();

    if (manager.isEmpty()) {
        this->d->m_error = "Manager not found";

        return false;
    }

    // Read devices information.
    QList<DeviceInfo> devices;

    for (auto it = this->d->m_descriptions.begin();
         it != this->d->m_descriptions.end();
         it++) {
        if (it.key() == deviceId)
            devices << DeviceInfo {description, this->d->m_devicesFormats[it.key()]};
        else
            devices << DeviceInfo {it.value(), this->d->m_devicesFormats[it.key()]};
    }

    QTemporaryDir tempDir;
    QSettings settings(tempDir.path() + "/config.ini", QSettings::IniFormat);

    // Write 'config.ini'.
    int i = 0;
    int j = 0;

    for (auto &device: devices) {
        QStringList formatsIndex;

        for (int i = 0; i < device.formats.size(); i++)
            formatsIndex << QString("%1").arg(i + j + 1);

        settings.beginGroup("Cameras");
        settings.beginWriteArray("cameras");
        settings.setArrayIndex(i);

        settings.setValue("description", device.description);
        settings.setValue("formats", formatsIndex);
        settings.endArray();
        settings.endGroup();

        settings.beginGroup("Formats");
        settings.beginWriteArray("formats");

        for (auto &format: device.formats) {
            settings.setArrayIndex(j);
            settings.setValue("format", this->d->dshowAkFormatMap().value(format.format()));
            settings.setValue("width", format.width());
            settings.setValue("height", format.height());
            settings.setValue("fps", format.fps().toString());
            j++;
        }

        settings.endArray();
        settings.endGroup();

        i++;
    }

    // Copy default frame to file system.
    QImage defaultImage;
    QString defaultFrame;

    if (!this->d->m_picture.isEmpty()
        && defaultImage.load(this->d->m_picture)) {
        auto dataLocation =
                QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).value(0);
        dataLocation += QDir::separator() + qApp->applicationName();

        if (QDir().mkpath(dataLocation)) {
            defaultImage = defaultImage.convertToFormat(QImage::Format_RGB888);
            auto width = VCamDShowPrivate::alignUp(defaultImage.width(), 32);
            defaultImage = defaultImage.scaled(width,
                                               defaultImage.height(),
                                               Qt::IgnoreAspectRatio,
                                               Qt::SmoothTransformation);
            defaultFrame = dataLocation
                         + QDir::separator()
                         + "default_frame.png";

            if (!defaultImage.save(defaultFrame))
                defaultFrame.clear();
        }
    }

    settings.setValue("default_frame", defaultFrame);

#ifdef QT_DEBUG
    settings.setValue("loglevel", "7");
#endif

    settings.sync();

    bool ok = true;
    int exitCode = this->d->sudo({manager, "-f", "load", settings.fileName()});

    if (exitCode) {
        ok = false;
        auto errorMsg = QString("Manager exited with code %1").arg(exitCode);
        qDebug() << errorMsg.toStdString().c_str();
        this->d->m_error += errorMsg;
    }

    return ok;
}

bool VCamDShow::deviceDestroy(const QString &deviceId)
{
    this->d->m_error = "";
    auto manager = this->d->manager();

    if (manager.isEmpty()) {
        this->d->m_error = "Manager not found";

        return false;
    }

    // Read devices information.
    QList<DeviceInfo> devices;

    for (auto it = this->d->m_descriptions.begin();
         it != this->d->m_descriptions.end();
         it++) {
        if (it.key() != deviceId)
            devices << DeviceInfo {it.value(), this->d->m_devicesFormats[it.key()]};
    }

    QTemporaryDir tempDir;
    QSettings settings(tempDir.path() + "/config.ini", QSettings::IniFormat);

    // Write 'config.ini'.
    int i = 0;
    int j = 0;

    for (auto &device: devices) {
        QStringList formatsIndex;

        for (int i = 0; i < device.formats.size(); i++)
            formatsIndex << QString("%1").arg(i + j + 1);

        settings.beginGroup("Cameras");
        settings.beginWriteArray("cameras");
        settings.setArrayIndex(i);

        settings.setValue("description", device.description);
        settings.setValue("formats", formatsIndex);
        settings.endArray();
        settings.endGroup();

        settings.beginGroup("Formats");
        settings.beginWriteArray("formats");

        for (auto &format: device.formats) {
            settings.setArrayIndex(j);
            settings.setValue("format", this->d->dshowAkFormatMap().value(format.format()));
            settings.setValue("width", format.width());
            settings.setValue("height", format.height());
            settings.setValue("fps", format.fps().toString());
            j++;
        }

        settings.endArray();
        settings.endGroup();

        i++;
    }

    // Copy default frame to file system.
    QImage defaultImage;
    QString defaultFrame;

    if (!this->d->m_picture.isEmpty()
        && defaultImage.load(this->d->m_picture)) {
        auto dataLocation =
                QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).value(0);
        dataLocation += QDir::separator() + qApp->applicationName();

        if (QDir().mkpath(dataLocation)) {
            defaultImage = defaultImage.convertToFormat(QImage::Format_RGB888);
            auto width = VCamDShowPrivate::alignUp(defaultImage.width(), 32);
            defaultImage = defaultImage.scaled(width,
                                               defaultImage.height(),
                                               Qt::IgnoreAspectRatio,
                                               Qt::SmoothTransformation);
            defaultFrame = dataLocation
                         + QDir::separator()
                         + "default_frame.png";

            if (!defaultImage.save(defaultFrame))
                defaultFrame.clear();
        }
    }

    settings.setValue("default_frame", defaultFrame);

#ifdef QT_DEBUG
    settings.setValue("loglevel", "7");
#endif

    settings.sync();

    bool ok = true;
    int exitCode = this->d->sudo({manager, "-f", "load", settings.fileName()});

    if (exitCode) {
        ok = false;
        auto errorMsg = QString("Manager exited with code %1").arg(exitCode);
        qDebug() << errorMsg.toStdString().c_str();
        this->d->m_error += errorMsg;
    }

    return ok;
}

bool VCamDShow::destroyAllDevices()
{
    this->d->m_error = "";
    auto manager = this->d->manager();

    if (manager.isEmpty()) {
        this->d->m_error = "Manager not found";

        return false;
    }

    QTemporaryDir tempDir;
    QSettings settings(tempDir.path() + "/config.ini", QSettings::IniFormat);

    // Copy default frame to file system.
    QImage defaultImage;
    QString defaultFrame;

    if (!this->d->m_picture.isEmpty()
        && defaultImage.load(this->d->m_picture)) {
        auto dataLocation =
                QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).value(0);
        dataLocation += QDir::separator() + qApp->applicationName();

        if (QDir().mkpath(dataLocation)) {
            defaultImage = defaultImage.convertToFormat(QImage::Format_RGB888);
            auto width = VCamDShowPrivate::alignUp(defaultImage.width(), 32);
            defaultImage = defaultImage.scaled(width,
                                               defaultImage.height(),
                                               Qt::IgnoreAspectRatio,
                                               Qt::SmoothTransformation);
            defaultFrame = dataLocation
                         + QDir::separator()
                         + "default_frame.png";

            if (!defaultImage.save(defaultFrame))
                defaultFrame.clear();
        }
    }

    settings.setValue("default_frame", defaultFrame);

#ifdef QT_DEBUG
    settings.setValue("loglevel", "7");
#endif

    settings.sync();

    bool ok = true;
    int exitCode = this->d->sudo({manager, "-f", "load", settings.fileName()});

    if (exitCode) {
        ok = false;
        auto errorMsg = QString("Manager exited with code %1").arg(exitCode);
        qDebug() << errorMsg.toStdString().c_str();
        this->d->m_error += errorMsg;
    }

    return ok;
}

bool VCamDShow::init()
{
    this->d->m_isInitialized = false;

    if (this->d->m_device.isEmpty() || this->d->m_devices.isEmpty())
        return false;

    auto manager = this->d->manager();

    if (manager.isEmpty())
        return false;

    auto outputCaps = this->d->m_currentCaps;
    outputCaps.setFormat(AkVideoCaps::Format_rgb24);

    QString params;
    QTextStream paramsStream(&params);
    paramsStream << "\""
                 << manager
                 << "\" "
                 << "stream"
                 << " "
                 << this->d->m_device
                 << " "
                 << this->d->dshowAkFormatMap().value(outputCaps.format())
                 << " "
                 << outputCaps.width()
                 << " "
                 << outputCaps.height();

    this->d->m_streamProc.stdinReadPipe = nullptr;
    this->d->m_streamProc.stdinWritePipe = nullptr;
    memset(&this->d->m_streamProc.pipeAttributes,
           0,
           sizeof(SECURITY_ATTRIBUTES));
    this->d->m_streamProc.pipeAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    this->d->m_streamProc.pipeAttributes.bInheritHandle = true;
    this->d->m_streamProc.pipeAttributes.lpSecurityDescriptor = nullptr;

    if (!CreatePipe(&this->d->m_streamProc.stdinReadPipe,
                    &this->d->m_streamProc.stdinWritePipe,
                    &this->d->m_streamProc.pipeAttributes,
                    0)) {
        return false;
    }

    if (!SetHandleInformation(this->d->m_streamProc.stdinWritePipe,
                              HANDLE_FLAG_INHERIT,
                              0)) {
        CloseHandle(this->d->m_streamProc.stdinWritePipe);
        CloseHandle(this->d->m_streamProc.stdinReadPipe);

        return false;
    }

    STARTUPINFOA startupInfo;
    memset(&startupInfo, 0, sizeof(STARTUPINFOA));
    startupInfo.cb = sizeof(STARTUPINFOA);
    startupInfo.hStdInput = this->d->m_streamProc.stdinReadPipe;
    startupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    startupInfo.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION procInfo;
    memset(&procInfo, 0, sizeof(PROCESS_INFORMATION));

    if (!CreateProcessA(nullptr,
                        const_cast<char *>(params.toStdString().c_str()),
                        nullptr,
                        nullptr,
                        true,
                        0,
                        nullptr,
                        nullptr,
                        &startupInfo,
                        &procInfo)) {
        CloseHandle(this->d->m_streamProc.stdinWritePipe);
        CloseHandle(this->d->m_streamProc.stdinReadPipe);

        return false;
    }

    this->d->m_videoConverter.setOutputCaps(outputCaps);
    this->d->m_isInitialized = true;

    return true;
}

void VCamDShow::uninit()
{
    if (this->d->m_streamProc.stdinReadPipe) {
        this->d->m_streamProc.stdinMutex.lock();
        CloseHandle(this->d->m_streamProc.stdinWritePipe);
        this->d->m_streamProc.stdinWritePipe = nullptr;
        CloseHandle(this->d->m_streamProc.stdinReadPipe);
        this->d->m_streamProc.stdinReadPipe = nullptr;
        this->d->m_streamProc.stdinMutex.unlock();

        WaitForSingleObject(this->d->m_streamProc.procInfo.hProcess, INFINITE);
        CloseHandle(this->d->m_streamProc.procInfo.hProcess);
        CloseHandle(this->d->m_streamProc.procInfo.hThread);
    }
}

void VCamDShow::setDevice(const QString &device)
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
        this->d->m_globalControls = this->d->controls(device);
        this->d->m_controlsMutex.unlock();
    }

    this->d->m_controlsMutex.lock();
    auto status = this->d->controlStatus(this->d->m_globalControls);
    this->d->m_controlsMutex.unlock();

    emit this->deviceChanged(device);
    emit this->controlsChanged(status);
}

void VCamDShow::setCurrentCaps(const AkVideoCaps &currentCaps)
{
    if (this->d->m_currentCaps == currentCaps)
        return;

    this->d->m_currentCaps = currentCaps;
    emit this->currentCapsChanged(this->d->m_currentCaps);
}

void VCamDShow::setPicture(const QString &picture)
{
    if (this->d->m_picture == picture)
        return;

    this->d->m_picture = picture;
    emit this->pictureChanged(this->d->m_picture);
}

void VCamDShow::setRootMethod(const QString &rootMethod)
{
    if (this->d->m_rootMethod == rootMethod)
        return;

    this->d->m_rootMethod = rootMethod;
    emit this->rootMethodChanged(this->d->m_rootMethod);
}

bool VCamDShow::applyPicture()
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return false;

    // Read devices information.
    QList<DeviceInfo> devices;

    for (auto it = this->d->m_descriptions.begin();
         it != this->d->m_descriptions.end();
         it++) {
        devices << DeviceInfo {it.value(), this->d->m_devicesFormats[it.key()]};
    }

    QTemporaryDir tempDir;
    QSettings settings(tempDir.path() + "/config.ini", QSettings::IniFormat);

    // Write 'config.ini'.
    int i = 0;
    int j = 0;

    for (auto &device: devices) {
        QStringList formatsIndex;

        for (int i = 0; i < device.formats.size(); i++)
            formatsIndex << QString("%1").arg(i + j + 1);

        settings.beginGroup("Cameras");
        settings.beginWriteArray("cameras");
        settings.setArrayIndex(i);

        settings.setValue("description", device.description);
        settings.setValue("formats", formatsIndex);
        settings.endArray();
        settings.endGroup();

        settings.beginGroup("Formats");
        settings.beginWriteArray("formats");

        for (auto &format: device.formats) {
            settings.setArrayIndex(j);
            settings.setValue("format", this->d->dshowAkFormatMap().value(format.format()));
            settings.setValue("width", format.width());
            settings.setValue("height", format.height());
            settings.setValue("fps", format.fps().toString());
            j++;
        }

        settings.endArray();
        settings.endGroup();

        i++;
    }

    // Copy default frame to file system.
    QImage defaultImage;
    QString defaultFrame;

    if (!this->d->m_picture.isEmpty()
        && defaultImage.load(this->d->m_picture)) {
        auto dataLocation =
                QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).value(0);
        dataLocation += QDir::separator() + qApp->applicationName();

        if (QDir().mkpath(dataLocation)) {
            defaultImage = defaultImage.convertToFormat(QImage::Format_RGB888);
            auto width = VCamDShowPrivate::alignUp(defaultImage.width(), 32);
            defaultImage = defaultImage.scaled(width,
                                               defaultImage.height(),
                                               Qt::IgnoreAspectRatio,
                                               Qt::SmoothTransformation);
            defaultFrame = dataLocation
                         + QDir::separator()
                         + "default_frame.png";

            if (!defaultImage.save(defaultFrame))
                defaultFrame.clear();
        }
    }

    settings.setValue("default_frame", defaultFrame);

#ifdef QT_DEBUG
    settings.setValue("loglevel", "7");
#endif

    settings.sync();

    bool ok = true;
    int exitCode = this->d->sudo({manager, "load", settings.fileName()});

    if (exitCode) {
        ok = false;
        auto errorMsg = QString("Manager exited with code %1").arg(exitCode);
        qDebug() << errorMsg.toStdString().c_str();
        this->d->m_error += errorMsg;
    }

    return ok;
}

bool VCamDShow::write(const AkVideoPacket &frame)
{
    if (!this->d->m_isInitialized)
        return false;

    if (!this->d->m_streamProc.stdinReadPipe)
        return false;

    this->d->m_controlsMutex.lock();
    auto curControls = this->d->controlStatus(this->d->m_globalControls);
    this->d->m_controlsMutex.unlock();

    if (this->d->m_localControls != curControls) {
        auto controls = this->d->mapDiff(this->d->m_localControls,
                                         curControls);
        this->d->setControls(this->d->m_device, controls);
        this->d->m_localControls = curControls;
    }

    this->d->m_videoConverter.begin();
    auto videoPacket = this->d->m_videoConverter.convert(frame);
    this->d->m_videoConverter.end();

    if (!videoPacket)
        return false;

    this->d->m_streamProc.stdinMutex.lock();
    bool ok = false;

    if (this->d->m_streamProc.stdinWritePipe) {
        ok = true;

        for (int y = 0; y < videoPacket.caps().height(); y++) {
            auto line = videoPacket.constLine(0, y);
            auto lineSize = videoPacket.bytesUsed(0);
            DWORD bytesWritten = 0;
            ok = WriteFile(this->d->m_streamProc.stdinWritePipe,
                           line,
                           DWORD(lineSize),
                           &bytesWritten,
                           nullptr);

            if (!ok)
                break;
        }
    }

    this->d->m_streamProc.stdinMutex.unlock();

    return ok;
}

VCamDShowPrivate::VCamDShowPrivate(VCamDShow *self):
    self(self)
{
    auto manager = this->manager();

    if (!manager.isEmpty()) {
        this->m_eventsProc = new QProcess;
        this->m_eventsProc->setReadChannel(QProcess::StandardOutput);
        this->m_eventsProc->start(manager, {"-p", "listen-events"});

        QObject::connect(this->m_eventsProc,
                         &QProcess::readyReadStandardOutput,
                         [this] () {
            while (this->m_runEventsProc && this->m_eventsProc->canReadLine()) {
                auto event = this->m_eventsProc->readLine().trimmed();
                qDebug() << "Event:" << event;

                if (event == "DevicesUpdated") {
                    this->updateDevices();
                } else if (event == "PictureUpdated") {
                    this->m_picture = this->readPicturePath();
                }
            }
        });
    }

    this->fillSupportedFormats();
    this->m_picture = this->readPicturePath();
    this->updateDevices();
}

VCamDShowPrivate::~VCamDShowPrivate()
{
    if (this->m_eventsProc) {
        this->m_runEventsProc = false;
        //this->m_eventsProc->terminate();
        this->m_eventsProc->kill();
        this->m_eventsProc->waitForFinished();
        delete this->m_eventsProc;
    }
}

QStringList VCamDShowPrivate::availableRootMethods() const
{
    static const QStringList sus {
        "runas"
    };

    QStringList methods;

    for (auto &su: sus)
        if (!this->whereBin(su).isEmpty())
            methods << su;

    return methods;
}

QString VCamDShowPrivate::whereBin(const QString &binary) const
{
    auto binaryExe = binary + ".exe";
    auto paths =
            QProcessEnvironment::systemEnvironment().value("Path").split(';');

    for (auto &path: paths)
        if (QDir(path).exists(binaryExe))
            return QDir(path).filePath(binaryExe);

    return {};
}

const DShowAkFormatMap &VCamDShowPrivate::dshowAkFormatMap() const
{
    static const DShowAkFormatMap formatMap {
        // RGB formats
        {AkVideoCaps::Format_xrgb    , "RGB32"},
        {AkVideoCaps::Format_rgb24   , "RGB24"},
        {AkVideoCaps::Format_rgb565le, "RGB16"},
        {AkVideoCaps::Format_rgb555le, "RGB15"},

        // RGB formats
        {AkVideoCaps::Format_xbgr    , "BGR32"},
        {AkVideoCaps::Format_bgr24   , "BGR24"},
        {AkVideoCaps::Format_bgr565le, "BGR16"},
        {AkVideoCaps::Format_bgr555le, "BGR15"},

        // YUV formats
        {AkVideoCaps::Format_uyvy422 , "UYVY"},
        {AkVideoCaps::Format_yuyv422 , "YUY2"},
        {AkVideoCaps::Format_nv12    , "NV12"},
        {AkVideoCaps::Format_nv21    , "NV21"},
    };

    return formatMap;
}

void VCamDShowPrivate::fillSupportedFormats()
{
    auto manager = this->manager();

    if (manager.isEmpty())
        return;

    QProcess proc;
    proc.start(manager, {"-p", "supported-formats", "-o"});
    proc.waitForFinished();
    this->m_supportedOutputPixelFormats.clear();

    if (proc.exitCode() == 0) {
        for (auto &line: proc.readAllStandardOutput().split('\n')) {
            auto format = this->dshowAkFormatMap().key(line.trimmed(),
                                                       AkVideoCaps::Format_none);

            if (format != AkVideoCaps::Format_none)
                this->m_supportedOutputPixelFormats << format;
        }
    }

    proc.start(manager, {"-p", "default-format", "-o"});
    proc.waitForFinished();
    this->m_defaultOutputPixelFormat = AkVideoCaps::Format_none;

    if (proc.exitCode() == 0)
        this->m_defaultOutputPixelFormat =
                this->dshowAkFormatMap().key(proc.readAllStandardOutput().trimmed(),
                                             AkVideoCaps::Format_none);
}

QVariantMap VCamDShowPrivate::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    for (auto &control: controls) {
        auto params = control.toList();
        auto controlDescription = params[1].toString();
        controlStatus[controlDescription] = params[7];
    }

    return controlStatus;
}

QVariantMap VCamDShowPrivate::mapDiff(const QVariantMap &map1,
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

QVariantList VCamDShowPrivate::controls(const QString &device)
{
    auto manager = this->manager();

    if (manager.isEmpty())
        return {};

    QProcess proc;
    proc.start(manager, {"-p", "dump"});
    proc.waitForFinished();

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->m_error = QString(errorMsg);
        }

        return {};
    }

    QXmlStreamReader xmlInfo(proc.readAllStandardOutput());
    QStringList pathList;
    QString path;
    QVariantList controls;
    QVariantList curDeviceControls;
    DeviceControl deviceControl;
    bool readControls = false;

    while (!xmlInfo.atEnd()) {
        if (!readControls && !controls.isEmpty())
            break;

        auto token = xmlInfo.readNext();

        switch (token) {
        case QXmlStreamReader::Invalid: {
            if (this->m_error != xmlInfo.errorString()) {
                qDebug() << xmlInfo.errorString().toStdString().c_str();
                this->m_error = xmlInfo.errorString();
            }

            return {};
        }

        case QXmlStreamReader::StartElement: {
            pathList << xmlInfo.name().toString();

            if (path.isEmpty() && xmlInfo.name() != QStringLiteral("info"))
                return {};

            if (path == "info/devices"
                && xmlInfo.name() == QStringLiteral("device"))
                curDeviceControls.clear();
            else if (path == "info/devices/device/controls"
                     && xmlInfo.name() == QStringLiteral("control"))
                deviceControl = {};

            break;
        }

        case QXmlStreamReader::EndElement: {
            if (path == "info/devices/device") {
                if (readControls)
                    controls = curDeviceControls;

                readControls = false;
            } else if (path == "info/devices/device/controls/control") {
                if (readControls
                    && !deviceControl.id.isEmpty()
                    && !deviceControl.description.isEmpty()
                    && !deviceControl.type.isEmpty()
                    && (deviceControl.type != "menu"
                        || !deviceControl.menu.isEmpty())) {
                    QVariantList controlVar {
                        deviceControl.id,
                        deviceControl.description,
                        deviceControl.type,
                        deviceControl.minimum,
                        deviceControl.maximum,
                        deviceControl.step,
                        deviceControl.defaultValue,
                        deviceControl.value,
                        deviceControl.menu
                    };
                    curDeviceControls << QVariant(controlVar);
                }
            }

            pathList.removeLast();

            break;
        }
        case QXmlStreamReader::Characters: {
            if (path == "info/devices/device/id")
                readControls = xmlInfo.text().trimmed() == device;

            if (readControls) {
                if (path == "info/devices/device/controls/control/id")
                    deviceControl.id = xmlInfo.text().trimmed().toString();
                else if (path == "info/devices/device/controls/control/description")
                    deviceControl.description = xmlInfo.text().trimmed().toString();
                else if (path == "info/devices/device/controls/control/type")
                    deviceControl.type = xmlInfo.text().trimmed().toString().toLower();
                else if (path == "info/devices/device/controls/control/minimum")
                    deviceControl.minimum = xmlInfo.text().trimmed().toInt();
                else if (path == "info/devices/device/controls/control/maximum")
                    deviceControl.maximum = xmlInfo.text().trimmed().toInt();
                else if (path == "info/devices/device/controls/control/step")
                    deviceControl.step = xmlInfo.text().trimmed().toInt();
                else if (path == "info/devices/device/controls/control/default-value")
                    deviceControl.defaultValue = xmlInfo.text().trimmed().toInt();
                else if (path == "info/devices/device/controls/control/value")
                    deviceControl.value = xmlInfo.text().trimmed().toInt();
                else if (path == "info/devices/device/controls/control/menu/item")
                    deviceControl.menu << xmlInfo.text().trimmed().toString();
            }

            break;
        }

        default:
            break;
        }

        path = pathList.join("/");
    }

    return controls;
}

bool VCamDShowPrivate::setControls(const QString &device,
                                   const QVariantMap &controls)
{
    auto manager = this->manager();

    if (manager.isEmpty())
        return false;

    auto result = true;
    QStringList args;

    for (auto &control: this->m_globalControls) {
        auto controlParams = control.toList();
        auto description = controlParams[1].toString();

        if (!controls.contains(description)) {
            result = false;

            continue;
        }

        auto name = controlParams[0].toString();
        args << QString("%1=%2").arg(name).arg(controls[description].toInt());
    }

    QProcess proc;
    proc.start(manager, QStringList {"set-controls", device} + args);
    proc.waitForFinished();

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->m_error += QString(errorMsg);
        }

        result = false;
    }

    return result;
}

QString VCamDShowPrivate::readPicturePath() const
{
    auto manager = this->manager();

    if (manager.isEmpty())
        return {};

    QProcess proc;
    proc.start(manager, {"-p", "picture"});
    proc.waitForFinished();

    if (proc.exitCode())
        return {};

    return QString(proc.readAllStandardOutput().trimmed());
}

QString VCamDShowPrivate::servicePath(const QString &serviceName) const
{
    auto manager = OpenSCManager(nullptr, nullptr, GENERIC_READ);

    if (!manager) {
        qCritical() << "Failed to connect with the service control manager";

        return {};
    }

    QString path;
    auto service = OpenServiceA(manager,
                                serviceName.toStdString().c_str(),
                                SERVICE_QUERY_CONFIG);

    if (!service) {
        qCritical() << "Failed to open" << serviceName << "service";
        CloseServiceHandle(manager);

        return {};
    }

    DWORD bytesNeeded = 0;
    auto ok = QueryServiceConfig(service, nullptr, 0, &bytesNeeded);

    if (!ok || bytesNeeded < 1) {
        qCritical() << "Failed to query the buffer size for" << serviceName << "service configuration";
        CloseServiceHandle(service);
        CloseServiceHandle(manager);

        return {};
    }

    auto bufSize = bytesNeeded;
    auto serviceConfig =
            reinterpret_cast<LPQUERY_SERVICE_CONFIG>(LocalAlloc(LMEM_FIXED,
                                                                bufSize));

    if (!serviceConfig) {
        qCritical() << "Failed to allocate the buffer for the query";
        CloseServiceHandle(service);
        CloseServiceHandle(manager);

        return {};
    }

    ok = QueryServiceConfig(service,
                            serviceConfig,
                            bufSize,
                            &bytesNeeded);

    if (!ok) {
        qCritical() << "Can't query the configurations for" << serviceName << "service";
        LocalFree(serviceConfig);
        CloseServiceHandle(service);
        CloseServiceHandle(manager);

        return {};
    }

    path = QString::fromStdWString(serviceConfig->lpBinaryPathName);
    LocalFree(serviceConfig);
    CloseServiceHandle(service);
    CloseServiceHandle(manager);

    return path;
}

QString VCamDShowPrivate::manager(const QString &arch) const
{
    QStringList archs;

    if (QSysInfo::buildCpuArchitecture() == "x86_64") {
        archs << "x64" << "x86";
    } else {
        archs << "x86";

        if (QSysInfo::currentCpuArchitecture() == "x86_64")
            archs << "x64";
    }

    auto assistant = this->servicePath("AkVCamAssistant");

    if (assistant.isEmpty()) {
        /* Maybe the assistant is not running, try searching the manager in
         * the standard install path
         */

        TCHAR programFiles[MAX_PATH];

        if (FAILED(SHGetFolderPath(NULL,
                                   CSIDL_PROGRAM_FILES,
                                   NULL,
                                   0,
                                   programFiles))) {
            return {};
        };

        if (!arch.isEmpty()) {
            auto manager = QString("%1\\AkVirtualCamera\\%2\\AkVCamManager.exe").arg(programFiles, arch);

            return QFileInfo::exists(manager)? manager: QString();
        }

        for (auto &arch: archs) {
            auto manager = QString("%1\\AkVirtualCamera\\%2\\AkVCamManager.exe").arg(programFiles, arch);

            if (QFileInfo::exists(manager))
                return manager;
        }

        return {};
    }

    auto pluginDir = QFileInfo(assistant).absoluteDir();
    pluginDir.cdUp();

    if (!arch.isEmpty()) {
        auto manager = pluginDir.absoluteFilePath(arch + "\\AkVCamManager.exe");

        return QFileInfo::exists(manager)? manager: QString();
    }

    for (auto &arch: archs) {
        auto manager = pluginDir.absoluteFilePath(arch + "\\AkVCamManager.exe");

        if (QFileInfo::exists(manager))
            return manager;
    }

    return {};
}

void VCamDShowPrivate::updateDevices()
{
    auto manager = this->manager();

    if (manager.isEmpty())
        return;

    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesFormats) devicesFormats;

    QProcess proc;
    proc.start(manager, {"-p", "dump"});
    proc.waitForFinished();

    if (proc.exitCode() != 0)
        return;

    QXmlStreamReader xmlInfo(proc.readAllStandardOutput());
    QStringList pathList;
    QString path;
    QString curDevice;
    QString curDescription;
    AkVideoCapsList curDeviceCaps;
    DeviceFormat curFormat;

    while (!xmlInfo.atEnd()) {
        auto token = xmlInfo.readNext();

        switch (token) {
        case QXmlStreamReader::Invalid: {
            if (this->m_error != xmlInfo.errorString()) {
                qDebug() << xmlInfo.errorString().toStdString().c_str();
                this->m_error = xmlInfo.errorString();
            }

            return;
        }

        case QXmlStreamReader::StartElement: {
            pathList << xmlInfo.name().toString();

            if (path.isEmpty() && xmlInfo.name() != QStringLiteral("info"))
                return;

            if (path == "info/devices" && xmlInfo.name() == QStringLiteral("device"))
                curDeviceCaps.clear();
            else if (path == "info/devices/device/formats"
                     && xmlInfo.name() == QStringLiteral("format"))
                curFormat = {};

            break;
        }

        case QXmlStreamReader::EndElement: {
            if (path == "info/devices/device") {
                if (!curDevice.isEmpty()
                    && !curDescription.isEmpty()
                    && !curDeviceCaps.isEmpty()) {
                    devices << curDevice;
                    descriptions[curDevice] = curDescription;
                    devicesFormats[curDevice] = curDeviceCaps;
                }
            } else if (path == "info/devices/device/formats/format") {
                AkVideoCaps caps(curFormat.format,
                                 curFormat.width,
                                 curFormat.height,
                                 curFormat.fps);

                if (caps)
                    curDeviceCaps << caps;
            }

            pathList.removeLast();

            break;
        }
        case QXmlStreamReader::Characters: {
            if (path == "info/devices/device/id")
                curDevice = xmlInfo.text().trimmed().toString();
            else if (path == "info/devices/device/description")
                curDescription = xmlInfo.text().trimmed().toString();
            else if (path == "info/devices/device/formats/format/pixel-format")
                curFormat.format =
                        this->dshowAkFormatMap().key(xmlInfo.text().trimmed().toString(),
                                                     AkVideoCaps::Format_none);
            else if (path == "info/devices/device/formats/format/width")
                curFormat.width = int(xmlInfo.text().trimmed().toUInt());
            else if (path == "info/devices/device/formats/format/height")
                curFormat.height = int(xmlInfo.text().trimmed().toUInt());
            else if (path == "info/devices/device/formats/format/fps")
                curFormat.fps = xmlInfo.text().trimmed().toString();

            break;
        }

        default:
            break;
        }

        path = pathList.join("/");
    }

    if (devicesFormats.isEmpty()) {
        devices.clear();
        descriptions.clear();
    }

    this->m_descriptions = descriptions;
    this->m_devicesFormats = devicesFormats;

    if (this->m_devices != devices) {
        this->m_devices = devices;
        emit self->webcamsChanged(this->m_devices);
    }
}

int VCamDShowPrivate::sudo(const QStringList &parameters,
                           const QString &directory,
                           bool show)
{
    if (parameters.size() < 1)
        return E_FAIL;

    auto command = parameters[0];
    QString params;

    for (int i = 1; i < parameters.size(); i++) {
        if (i > 1)
            params += " ";

        if (parameters[i].contains(" "))
            params += "\"" + parameters[i] + "\"";
        else
            params += parameters[i];
    }

    SHELLEXECUTEINFOA execInfo;
    memset(&execInfo, 0, sizeof(SHELLEXECUTEINFOA));
    execInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
    execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    execInfo.hwnd = nullptr;
    execInfo.lpVerb = "runas";
    execInfo.lpFile = command.toStdString().c_str();
    execInfo.lpParameters = params.toStdString().c_str();
    execInfo.lpDirectory = directory.toStdString().c_str();
    execInfo.nShow = show? SW_SHOWNORMAL: SW_HIDE;
    execInfo.hInstApp = nullptr;
    ShellExecuteExA(&execInfo);

    if (!execInfo.hProcess) {
        this->m_error = "Failed executing script";

        return E_FAIL;
    }

    WaitForSingleObject(execInfo.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(execInfo.hProcess, &exitCode);
    CloseHandle(execInfo.hProcess);

    if (FAILED(exitCode))
        this->m_error = QString("Script failed with code %1").arg(exitCode);

    return int(exitCode);
}

#include "moc_vcamdshow.cpp"
