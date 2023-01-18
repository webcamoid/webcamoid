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
#include <QTextCodec>
#include <QVariant>
#include <QWaitCondition>
#include <QXmlStreamReader>
#include <akfrac.h>
#include <akvideoconverter.h>

#include "vcamcmio.h"

using CMIOAkFormatMap = QMap<AkVideoCaps::PixelFormat, QString>;

struct DeviceFormat
{
    AkVideoCaps::PixelFormat format {AkVideoCaps::Format_none};
    int width {0};
    int height {0};
    AkFrac fps;
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

class VCamCMIOPrivate
{
    public:
        VCamCMIO *self;
        QString m_device;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, AkVideoCapsList> m_devicesFormats;
        QList<AkVideoCaps::PixelFormat> m_supportedOutputPixelFormats;
        AkVideoCaps::PixelFormat m_defaultOutputPixelFormat;
        QVariantList m_globalControls;
        QVariantMap m_localControls;
        QProcess *m_eventsProc {nullptr};
        FILE *m_streamProc {nullptr};
        AkVideoCaps m_curFormat;
        QMutex m_controlsMutex;
        QString m_error;
        AkVideoCaps m_currentCaps;
        AkVideoConverter m_videoConverter;
        QString m_picture;
        QString m_rootMethod;
        bool m_isInitialized {false};
        bool m_runEventsProc {true};

        VCamCMIOPrivate(VCamCMIO *self=nullptr);
        ~VCamCMIOPrivate();

        QStringList availableRootMethods() const;
        QString whereBin(const QString &binary) const;
        inline const CMIOAkFormatMap &cmioAkFormatMap() const;
        void fillSupportedFormats();
        QVariantMap controlStatus(const QVariantList &controls) const;
        QVariantMap mapDiff(const QVariantMap &map1,
                            const QVariantMap &map2) const;
        QVariantList controls(const QString &device);
        bool setControls(const QString &device,
                         const QVariantMap &controls);
        QString readPicturePath() const;
        QString manager() const;
        void updateDevices();
        template<typename T>
        static inline T alignUp(const T &value, const T &align)
        {
            return (value + align - 1) & ~(align - 1);
        }
};

VCamCMIO::VCamCMIO(QObject *parent):
    VCam(parent)
{
    this->d = new VCamCMIOPrivate(this);
    QStringList preferredRootMethod {
        "osascript",
    };

    auto availableMethods = this->d->availableRootMethods();

    for (auto &method: preferredRootMethod)
        if (availableMethods.contains(method)) {
            this->d->m_rootMethod = method;

            break;
        }
}

VCamCMIO::~VCamCMIO()
{
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

QString VCamCMIO::installedVersion() const
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

QStringList VCamCMIO::webcams() const
{
    return this->d->m_devices;
}

QString VCamCMIO::device() const
{
    return this->d->m_device;
}

QString VCamCMIO::description(const QString &deviceId) const
{
    return this->d->m_descriptions.value(deviceId);
}

QList<AkVideoCaps::PixelFormat> VCamCMIO::supportedOutputPixelFormats() const
{
    return this->d->m_supportedOutputPixelFormats;
}

AkVideoCaps::PixelFormat VCamCMIO::defaultOutputPixelFormat() const
{
    return this->d->m_defaultOutputPixelFormat;
}

AkVideoCapsList VCamCMIO::caps(const QString &deviceId) const
{
    if (!this->d->m_devicesFormats.contains(deviceId))
        return {};

    return this->d->m_devicesFormats[deviceId];
}

AkVideoCaps VCamCMIO::currentCaps() const
{
    return this->d->m_currentCaps;
}

QVariantList VCamCMIO::controls() const
{
    QVariantList controls;

    for (auto &control: this->d->m_globalControls)
        controls << QVariant(control.toList().mid(1));

    return controls;
}

bool VCamCMIO::setControls(const QVariantMap &controls)
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

    if (!this->d->m_streamProc)
        this->d->setControls(this->d->m_device, controls);

    emit this->controlsChanged(controls);

    return true;
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

        if (pid && pid != qApp->applicationPid())
            pids << quint64(pid);
    }

    return pids;
}

QString VCamCMIO::clientExe(quint64 pid) const
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

QString VCamCMIO::picture() const
{
    return this->d->m_picture;
}

QString VCamCMIO::rootMethod() const
{
    return this->d->m_rootMethod;
}

QStringList VCamCMIO::availableRootMethods() const
{
    return this->d->availableRootMethods();
}

QString VCamCMIO::deviceCreate(const QString &description,
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

    // Set file encoding.
    auto codec = QTextCodec::codecForLocale();

    if (codec)
        settings.setIniCodec(codec->name());
    else
        settings.setIniCodec("UTF-8");

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
            settings.setValue("format", this->d->cmioAkFormatMap().value(format.format()));
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
            auto width = VCamCMIOPrivate::alignUp(defaultImage.width(), 32);
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

    QProcess proc;
    proc.start(manager, {"-f", "load", settings.fileName()});
    proc.waitForFinished();

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg);
        }
    }

    QString deviceId;

    if (!proc.exitCode()) {
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

bool VCamCMIO::deviceEdit(const QString &deviceId,
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

    // Set file encoding.
    auto codec = QTextCodec::codecForLocale();

    if (codec)
        settings.setIniCodec(codec->name());
    else
        settings.setIniCodec("UTF-8");

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
            settings.setValue("format", this->d->cmioAkFormatMap().value(format.format()));
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
            auto width = VCamCMIOPrivate::alignUp(defaultImage.width(), 32);
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
    QProcess proc;
    proc.start(manager, {"-f", "load", settings.fileName()});
    proc.waitForFinished();

    if (proc.exitCode()) {
        ok = false;
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg);
        }
    }

    return ok;
}

bool VCamCMIO::changeDescription(const QString &deviceId,
                                 const QString &description)
{
    this->d->m_error = "";
    auto manager = this->d->manager();

    if (manager.isEmpty()) {
        this->d->m_error = "Manager not found";

        return false;
    }

    bool ok = true;
    QProcess proc;
    proc.start(manager, {"-f", "set-description", deviceId, description});
    proc.waitForFinished();

    if (!proc.exitCode()) {
        proc.start(manager, {"-f", "update"});
        proc.waitForFinished();
    }

    if (proc.exitCode()) {
        ok = false;
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg);
        }
    }

    return ok;
}

bool VCamCMIO::deviceDestroy(const QString &deviceId)
{
    this->d->m_error = "";
    auto manager = this->d->manager();

    if (manager.isEmpty()) {
        this->d->m_error = "Manager not found";

        return false;
    }

    bool ok = true;
    QProcess proc;
    proc.start(manager, {"-f", "remove-device", deviceId});
    proc.waitForFinished();

    if (!proc.exitCode()) {
        proc.start(manager, {"-f", "update"});
        proc.waitForFinished();
    }

    if (proc.exitCode()) {
        ok = false;
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg);
        }
    }

    return ok;
}

bool VCamCMIO::destroyAllDevices()
{
    this->d->m_error = "";
    auto manager = this->d->manager();

    if (manager.isEmpty()) {
        this->d->m_error = "Manager not found";

        return false;
    }

    bool ok = true;
    QProcess proc;
    proc.start(manager, {"-f", "remove-devices"});
    proc.waitForFinished();

    if (!proc.exitCode()) {
        proc.start(manager, {"-f", "update"});
        proc.waitForFinished();
    }

    if (proc.exitCode()) {
        ok = false;
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg);
        }
    }

    return ok;
}

bool VCamCMIO::init()
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
                 << this->d->cmioAkFormatMap().value(outputCaps.format())
                 << " "
                 << outputCaps.width()
                 << " "
                 << outputCaps.height();
    this->d->m_streamProc = popen(params.toStdString().c_str(), "w");

    if (this->d->m_streamProc)
        this->d->m_curFormat = this->d->m_currentCaps;

    this->d->m_videoConverter.setOutputCaps(outputCaps);
    this->d->m_isInitialized = true;

    return this->d->m_streamProc != nullptr;
}

void VCamCMIO::uninit()
{
    if (this->d->m_streamProc) {
        pclose(this->d->m_streamProc);
        this->d->m_streamProc = nullptr;
    }

    this->d->m_curFormat = AkVideoCaps();
}

void VCamCMIO::setDevice(const QString &device)
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

void VCamCMIO::setCurrentCaps(const AkVideoCaps &currentCaps)
{
    if (this->d->m_currentCaps == currentCaps)
        return;

    this->d->m_currentCaps = currentCaps;
    emit this->currentCapsChanged(this->d->m_currentCaps);
}

void VCamCMIO::setPicture(const QString &picture)
{
    if (this->d->m_picture == picture)
        return;

    this->d->m_picture = picture;
    emit this->pictureChanged(this->d->m_picture);
}

void VCamCMIO::setRootMethod(const QString &rootMethod)
{
    if (this->d->m_rootMethod == rootMethod)
        return;

    this->d->m_rootMethod = rootMethod;
    emit this->rootMethodChanged(this->d->m_rootMethod);
}

bool VCamCMIO::applyPicture()
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return {};

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
            auto width = VCamCMIOPrivate::alignUp(defaultImage.width(), 32);
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

    QProcess proc;
    proc.start(manager,
               {"-p",
                "set-picture",
                defaultFrame});
    proc.waitForFinished();

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->d->m_error += QString(errorMsg);
        }

        return false;
    }

    return true;
}

bool VCamCMIO::write(const AkVideoPacket &frame)
{
    if (!this->d->m_isInitialized)
        return false;

    if (!this->d->m_streamProc)
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

    bool ok = true;

    for (int y = 0; y < videoPacket.caps().height(); y++) {
        auto line = videoPacket.constLine(0, y);
        auto lineSize = videoPacket.bytesUsed(0);
        ok = fwrite(line, lineSize, 1, this->d->m_streamProc) > 0;

        if (!ok)
            break;
    }

    return ok;
}

VCamCMIOPrivate::VCamCMIOPrivate(VCamCMIO *self):
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

VCamCMIOPrivate::~VCamCMIOPrivate()
{
    if (this->m_eventsProc) {
        this->m_runEventsProc = false;
        //this->m_eventsProc->terminate();
        this->m_eventsProc->kill();
        this->m_eventsProc->waitForFinished();
        delete this->m_eventsProc;
    }
}

QStringList VCamCMIOPrivate::availableRootMethods() const
{
    static const QStringList sus {
        "osascript"
    };

    QStringList methods;

    for (auto &su: sus)
        if (!this->whereBin(su).isEmpty())
            methods << su;

    return methods;
}

QString VCamCMIOPrivate::whereBin(const QString &binary) const
{
    auto paths =
            QProcessEnvironment::systemEnvironment().value("PATH").split(':');

    for (auto &path: paths)
        if (QDir(path).exists(binary))
            return QDir(path).filePath(binary);

    return {};
}

const CMIOAkFormatMap &VCamCMIOPrivate::cmioAkFormatMap() const
{
    static const CMIOAkFormatMap formatMap {
        // RGB formats
        {AkVideoCaps::Format_0rgb    , "RGB32"},
        {AkVideoCaps::Format_rgb24   , "RGB24"},
        {AkVideoCaps::Format_rgb565le, "RGB16"},
        {AkVideoCaps::Format_rgb555le, "RGB15"},

        // RGB formats
        {AkVideoCaps::Format_0bgr    , "BGR32"},
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

void VCamCMIOPrivate::fillSupportedFormats()
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
            auto format = this->cmioAkFormatMap().key(line.trimmed(),
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
                this->cmioAkFormatMap().key(proc.readAllStandardOutput().trimmed(),
                                            AkVideoCaps::Format_none);
}

QVariantMap VCamCMIOPrivate::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    for (auto &control: controls) {
        auto params = control.toList();
        auto controlDescription = params[1].toString();
        controlStatus[controlDescription] = params[7];
    }

    return controlStatus;
}

QVariantMap VCamCMIOPrivate::mapDiff(const QVariantMap &map1,
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

QVariantList VCamCMIOPrivate::controls(const QString &device)
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

bool VCamCMIOPrivate::setControls(const QString &device,
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

QString VCamCMIOPrivate::readPicturePath() const
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

QString VCamCMIOPrivate::manager() const
{
    const QString pluginPath =
            "/Library/CoreMediaIO/Plug-Ins/DAL/AkVirtualCamera.plugin/Contents/Resources/AkVCamManager";

    if (!QFileInfo::exists(pluginPath))
        return {};

    return QFileInfo(pluginPath).canonicalFilePath();
}

void VCamCMIOPrivate::updateDevices()
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
                qDebug() << xmlInfo.errorString();
                this->m_error = xmlInfo.errorString();
            }

            return;
        }

        case QXmlStreamReader::StartElement: {
            pathList << xmlInfo.name().toString();

            if (path.isEmpty() && xmlInfo.name() != QStringLiteral("info"))
                return;

            if (path == "info/devices"
                && xmlInfo.name() == QStringLiteral("device"))
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
                        this->cmioAkFormatMap().key(xmlInfo.text().trimmed().toString(),
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

#include "moc_vcamcmio.cpp"
