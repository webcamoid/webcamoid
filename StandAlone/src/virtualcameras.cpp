/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQuickItem>
#include <QSettings>
#include <QtConcurrent>

#include <ak.h>
#include <akaudiocaps.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <iak/akelement.h>

#include "virtualcameras.h"
#include "updates.h"

#ifdef Q_OS_LINUX
    #define DEFAULT_VCAM_DRIVER "VideoSink/VirtualCamera/Impl/AkVCam"
#else
    #define DEFAULT_VCAM_DRIVER ""
#endif

using ObjectPtr = QSharedPointer<QObject>;

class VirtualCamerasPrivate
{
    public:
        VirtualCameras *self;
        QQmlApplicationEngine *m_engine {nullptr};
        AkElementPtr m_cameraOutput {akPluginManager->create<AkElement>("VideoSink/VirtualCamera")};
        QString m_vcamDriver;
        QThreadPool m_threadPool;
        AkElement::ElementState m_state {AkElement::ElementStateNull};
        QString m_latestVersion;
        bool m_currentVCamInstalled {false};
        bool m_busy {false};

        explicit VirtualCamerasPrivate(VirtualCameras *self);
        void connectSignals();
        void loadProperties();
        void stopOutput();
        void restoreOutput();
        inline QString vcamDownloadUrl() const;
};

VirtualCameras::VirtualCameras(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new VirtualCamerasPrivate(this);
    this->setQmlEngine(engine);
    this->d->connectSignals();
    this->d->loadProperties();
    this->d->m_latestVersion = this->currentVCamVersion();
    this->d->m_vcamDriver =
            akPluginManager->defaultPlugin("VideoSink/VirtualCamera/Impl/*").id();
    QObject::connect(akPluginManager,
                     &AkPluginManager::linksChanged,
                     this,
                     [this] (const AkPluginLinks &links) {
        if (links.contains("VideoSink/VirtualCamera/Impl/*")
            && links["VideoSink/VirtualCamera/Impl/*"] != this->d->m_vcamDriver) {
            this->d->m_vcamDriver = links["VideoSink/VirtualCamera/Impl/*"];
            emit this->vcamDriverChanged(this->d->m_vcamDriver);

            if (this->d->m_cameraOutput) {
                auto version =
                        this->d->m_cameraOutput->property("driverVersion").toString();
                emit this->currentVCamVersionChanged(version);
                auto installed =
                    this->d->m_cameraOutput->property("driverInstalled").toBool();

                if (this->d->m_currentVCamInstalled != installed) {
                    this->d->m_currentVCamInstalled = installed;
                    emit this->currentVCamInstalledChanged(installed);
                }
            }
        }
    });

    if (this->d->m_cameraOutput) {
        this->d->m_currentVCamInstalled =
            this->d->m_cameraOutput->property("driverInstalled").toBool();

        if (!this->d->m_currentVCamInstalled) {
            QString pluginId;
            auto plugins =
                    akPluginManager->listPlugins("VideoSink/VirtualCamera/Impl/*",
                                                 {"VirtualCameraImpl"},
                                                 AkPluginManager::FilterEnabled);
            for (auto &plugin: plugins) {
                auto pluginInstance = akPluginManager->create<QObject>(plugin);

                if (pluginInstance && pluginInstance->property("isInstalled").toBool()) {
                    if (pluginId.isEmpty())
                        pluginId = plugin;

                    if (plugin == DEFAULT_VCAM_DRIVER) {
                        pluginId = plugin;

                        break;
                    }
                }
            }

            if (pluginId.isEmpty())
                pluginId = DEFAULT_VCAM_DRIVER;

            akPluginManager->link("VideoSink/VirtualCamera/Impl/*", pluginId);
        }
    }
}

VirtualCameras::~VirtualCameras()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QString VirtualCameras::output() const
{
    if (!this->d->m_cameraOutput)
        return {};

    return this->d->m_cameraOutput->property("device").toString();
}

QString VirtualCameras::outputDescription() const
{
    if (!this->d->m_cameraOutput)
        return {};

    return this->d->m_cameraOutput->property("description").toString();
}

AkVideoCaps VirtualCameras::outputCaps() const
{
    if (!this->d->m_cameraOutput)
        return {};

    return this->d->m_cameraOutput->property("outputCaps").value<AkVideoCaps>();
}

AkVideoConverter::AspectRatioMode VirtualCameras::aspectRatioMode() const
{
    if (!this->d->m_cameraOutput)
        return AkVideoConverter::AspectRatioMode_Expanding;

    return this->d->m_cameraOutput->property("aspectRatioMode")
                                  .value<AkVideoConverter::AspectRatioMode>();
}

AkVideoCaps::PixelFormatList VirtualCameras::supportedOutputPixelFormats() const
{
    // The format is chosen here, the plugins accept any of them.
    return {
        AkVideoCaps::Format_yuyv422,
        AkVideoCaps::Format_uyvy422,
        AkVideoCaps::Format_yuv420p,
        AkVideoCaps::Format_nv12,
        AkVideoCaps::Format_xrgb,
        AkVideoCaps::Format_rgb24,
        AkVideoCaps::Format_bgr24
    };
}

AkVideoCaps::PixelFormat VirtualCameras::defaultOutputPixelFormat() const
{
    return AkVideoCaps::Format_yuyv422;
}

AkElement::ElementState VirtualCameras::state() const
{
    return this->d->m_state;
}

bool VirtualCameras::createOutput(const QString &description)
{
    if (!this->d->m_cameraOutput)
        return false;

    emit this->creatingOutput(description);

    // The driver can't be reconfigured while it is being used.
    this->d->m_busy = true;
    this->d->stopOutput();
    QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                              "createWebcam",
                              Q_ARG(QString, description));

    return true;
}

bool VirtualCameras::editOutput(const QString &description)
{
    if (!this->d->m_cameraOutput)
        return false;

    emit this->editingOutput(description);
    this->d->m_busy = true;
    this->d->stopOutput();
    QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                              "changeDescription",
                              Q_ARG(QString, description));

    return true;
}

bool VirtualCameras::removeOutput()
{
    if (!this->d->m_cameraOutput)
        return false;

    emit this->removingOutput(this->outputDescription());
    this->d->m_busy = true;
    this->d->stopOutput();
    QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                              "removeWebcam");

    return true;
}

void VirtualCameras::setOutputFormat(quint32 format,
                                     int width,
                                     int height,
                                     int fps)
{
    this->setOutputCaps(AkVideoCaps(AkVideoCaps::PixelFormat(format),
                                    width,
                                    height,
                                    AkFrac(fps, 1)));
}

QString VirtualCameras::outputError() const
{
    if (!this->d->m_cameraOutput)
        return {};

    QString error;
    QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                              "error",
                              Q_RETURN_ARG(QString, error));

    return error;
}

QList<quint64> VirtualCameras::clientsPids() const
{
    if (!this->d->m_cameraOutput)
        return {};

    auto pids = this->d->m_cameraOutput->property("clientsPids");

    return pids.value<QList<quint64>>();
}

QString VirtualCameras::clientExe(quint64 pid) const
{
    if (!this->d->m_cameraOutput)
        return {};

    QString exe;
    QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                              "clientExe",
                              Q_RETURN_ARG(QString, exe),
                              Q_ARG(quint64, pid));

    return exe;
}

bool VirtualCameras::driverInstalled() const
{
    if (!this->d->m_cameraOutput)
        return false;

    return this->d->m_cameraOutput->property("driverInstalled").toBool();
}

QString VirtualCameras::picture() const
{
    if (!this->d->m_cameraOutput)
        return {};

    return this->d->m_cameraOutput->property("picture").toString();
}

bool VirtualCameras::isVCamSupported() const
{
#if defined(Q_OS_BSD4) && !defined(Q_OS_DARWIN)
    return true;
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    return !Ak::isFlatpak() || Ak::hasFlatpakVCam();
#else
    return false;
#endif
}

VirtualCameras::VCamStatus VirtualCameras::vcamInstallStatus() const
{
    bool akvcamInstalled = false;
    bool otherInstalled = false;
    auto plugins = akPluginManager->listPlugins("VideoSink/VirtualCamera/Impl/*",
                                                {"VirtualCameraImpl"},
                                                AkPluginManager::FilterEnabled);

    for (auto &plugin: plugins) {
        auto pluginInstance = akPluginManager->create<QObject>(plugin);

        if (pluginInstance && pluginInstance->property("isInstalled").toBool()) {
            if (plugin == DEFAULT_VCAM_DRIVER) {
                akvcamInstalled = true;

                break;
            }

            otherInstalled = true;
        }
    }

    if (akvcamInstalled)
        return VCamInstalled;

    if (otherInstalled)
        return VCamInstalledOther;

    return VCamNotInstalled;
}

QString VirtualCameras::vcamDriver() const
{
    return this->d->m_vcamDriver;
}

QString VirtualCameras::currentVCamVersion() const
{
    if (this->d->m_cameraOutput)
        return this->d->m_cameraOutput->property("driverVersion").toString();

    return {};
}

bool VirtualCameras::isCurrentVCamInstalled() const
{
    return this->d->m_currentVCamInstalled;
}

bool VirtualCameras::canEditVCamDescription() const
{
    if (this->d->m_cameraOutput)
        return this->d->m_cameraOutput->property("canEditVCamDescription").toBool();

    return false;
}

QString VirtualCameras::vcamUpdateUrl() const
{
#ifdef Q_OS_LINUX
    return {"https://api.github.com/repos/webcamoid/akvcam/releases/latest"};
#else
    return {};
#endif
}

QString VirtualCameras::vcamDownloadUrl() const
{
#ifdef Q_OS_LINUX
    return {"https://github.com/webcamoid/akvcam/releases/latest"};
#else
    return {};
#endif
}

QString VirtualCameras::defaultVCamDriver() const
{
    return {DEFAULT_VCAM_DRIVER};
}

AkPacket VirtualCameras::iStream(const AkPacket &packet)
{
    if (this->d->m_state == AkElement::ElementStatePlaying
        && this->d->m_cameraOutput)
        this->d->m_cameraOutput->iStream(packet);

    return {};
}

bool VirtualCameras::applyPicture()
{
    if (!this->d->m_cameraOutput)
        return {};

    // The driver can't be reconfigured while it is being used.
    this->d->m_busy = true;
    this->d->stopOutput();

    bool ok = false;
    QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                              "applyPicture",
                              Q_RETURN_ARG(bool, ok));

    this->d->m_busy = false;
    this->d->restoreOutput();

    return ok;
}

void VirtualCameras::setLatestVCamVersion(const QString &version)
{
    this->d->m_latestVersion = version;
}

bool VirtualCameras::downloadVCam()
{
    if (!Updates::isOnline())
        return false;

    auto installerUrl = this->d->vcamDownloadUrl();

    if (installerUrl.isEmpty())
        return false;

    auto tempLocation =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation);

    if (tempLocation.isEmpty())
        return false;

    auto outFile = QDir(tempLocation).filePath(QUrl(installerUrl).fileName());

    emit this->startVCamDownload(tr("Virtual Camera"),
                                    installerUrl,
                                    outFile);

    return true;
}

bool VirtualCameras::executeVCamInstaller(const QString &installer)
{
    if (installer.isEmpty())
        return false;

    QFile(installer).setPermissions(QFileDevice::ReadOwner
                                    | QFileDevice::WriteOwner
                                    | QFileDevice::ExeOwner
                                    | QFileDevice::ReadUser
                                    | QFileDevice::WriteUser
                                    | QFileDevice::ExeUser
                                    | QFileDevice::ReadGroup
                                    | QFileDevice::ExeGroup
                                    | QFileDevice::ReadOther
                                    | QFileDevice::ExeOther);

    auto result =
            QtConcurrent::run(&this->d->m_threadPool, [this, installer] () {
        qDebug() << "Executing installer:" << installer;
        int exitCode = -1;

        QProcess proc;

#ifdef Q_PROCESSOR_X86
        if (Ak::isFlatpak())
            proc.start("flatpak-spawn", QStringList {"--host", installer});
        else
            proc.start(installer, QStringList {});
#else
        auto readLine = [this, &proc] () {
            while (proc.canReadLine())
                emit this->vcamCliInstallLineReady(proc.readLine());
        };

        QObject::connect(&proc,
                         &QProcess::started,
                         this,
                         &VirtualCameras::vcamCliInstallStarted);
        QObject::connect(&proc,
                         &QProcess::readyReadStandardOutput,
                         this,
                         readLine,
                         Qt::DirectConnection);
        QObject::connect(&proc,
                         &QProcess::readyReadStandardError,
                         this,
                         readLine,
                         Qt::DirectConnection);

        if (Ak::isFlatpak())
            proc.start("flatpak-spawn",
                       QStringList {"--host",
                                    "pkexec",
                                    "/bin/sh",
                                    "-c",
                                    "yes | " + installer});
        else
            proc.start("pkexec",
                       QStringList {"/bin/sh",
                                    "-c",
                                    "yes | " + installer});
#endif

        proc.waitForFinished(-1);
        exitCode = proc.exitCode();
        auto errorString = proc.errorString();

#ifndef Q_PROCESSOR_X86
        emit this->vcamCliInstallFinished();
#endif

        if (exitCode != 0)
            qDebug() << "Failed to run virtual camera installer:"
                     << exitCode
                     << ":"
                     << errorString;

        emit this->vcamInstallFinished(exitCode, errorString);
    });
    Q_UNUSED(result)

    return true;
}

void VirtualCameras::checkVCamDownloadReady(const QString &url,
                                        const QString &filePath,
                                        DownloadManager::DownloadStatus status,
                                        const QString &error)
{
    auto installerUrl = this->d->vcamDownloadUrl();

    if (installerUrl.isEmpty())
        return;

    if (installerUrl != url)
        return;

    switch (status) {
    case DownloadManager::DownloadStatusFinished:
        emit this->vcamDownloadReady(filePath);

        break;

    case DownloadManager::DownloadStatusFailed:
        emit this->vcamDownloadFailed(error);

        break;

    default:
        break;
    }
}

void VirtualCameras::setOutputCaps(const AkVideoCaps &outputCaps)
{
    if (this->d->m_cameraOutput)
        this->d->m_cameraOutput->setProperty("outputCaps",
                                             QVariant::fromValue(outputCaps));
}

void VirtualCameras::setAspectRatioMode(AkVideoConverter::AspectRatioMode aspectRatioMode)
{
    if (this->d->m_cameraOutput)
        this->d->m_cameraOutput->setProperty("aspectRatioMode",
                                             QVariant::fromValue(aspectRatioMode));
}

void VirtualCameras::setState(AkElement::ElementState state)
{
    if (this->d->m_state == state)
        return;

    this->d->m_state = state;
    emit this->stateChanged(state);

    if (!this->d->m_busy && this->d->m_cameraOutput)
        this->d->m_cameraOutput->setState(state);
}

void VirtualCameras::setPicture(const QString &picture)
{
    if (this->d->m_cameraOutput)
        this->d->m_cameraOutput->setProperty("picture", picture);
}

void VirtualCameras::resetOutputCaps()
{
    if (this->d->m_cameraOutput)
        QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                                  "resetOutputCaps");
}

void VirtualCameras::resetAspectRatioMode()
{
    if (this->d->m_cameraOutput)
        QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                                  "resetAspectRatioMode");
}

void VirtualCameras::resetState()
{
    this->setState(AkElement::ElementStateNull);
}

void VirtualCameras::resetPicture()
{
    if (this->d->m_cameraOutput)
        QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                                  "resetPicture");
}

void VirtualCameras::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->d->m_engine == engine)
        return;

    this->d->m_engine = engine;

    if (engine) {
        engine->rootContext()->setContextProperty("virtualCameras", this);
        qRegisterMetaType<VCamStatus>("VCamStatus");
        qmlRegisterType<VirtualCameras>("Webcamoid", 1, 0, "VirtualCameras");
    }
}

void VirtualCameras::onOutputChanged(const QString &output)
{
    emit this->outputChanged(output);

    if (this->d->m_busy)
        return;

    if (output.isEmpty())
        this->d->stopOutput();
    else
        this->d->restoreOutput();
}

void VirtualCameras::onOutputDescriptionChanged(const QString &description)
{
    QSettings config;
    config.beginGroup("VirtualCamera");
    config.setValue("description", description);
    config.endGroup();

    emit this->outputDescriptionChanged(description);
}

void VirtualCameras::onOutputCapsChanged(const AkVideoCaps &outputCaps)
{
    QSettings config;
    config.beginGroup("VirtualCamera");
    config.setValue("outputCaps/format", uint(outputCaps.format()));
    config.setValue("outputCaps/width", outputCaps.width());
    config.setValue("outputCaps/height", outputCaps.height());
    config.setValue("outputCaps/fps", outputCaps.fps().toString());
    config.endGroup();

    emit this->outputCapsChanged(outputCaps);
}

void VirtualCameras::onAspectRatioModeChanged(AkVideoConverter::AspectRatioMode aspectRatioMode)
{
    QSettings config;
    config.beginGroup("VirtualCamera");
    config.setValue("aspectRatioMode", int(aspectRatioMode));
    config.endGroup();

    emit this->aspectRatioModeChanged(aspectRatioMode);
}

void VirtualCameras::onOutputCreated(bool ok, const QString &msg)
{
    this->d->m_busy = false;
    this->d->restoreOutput();
    emit this->outputCreated(ok, msg);
}

void VirtualCameras::onOutputEdited(bool ok, const QString &msg)
{
    this->d->m_busy = false;
    this->d->restoreOutput();
    emit this->outputEdited(ok, msg);
}

void VirtualCameras::onOutputRemoved(bool ok, const QString &msg)
{
    this->d->m_busy = false;
    this->d->restoreOutput();
    emit this->outputRemoved(ok, msg);
}

VirtualCamerasPrivate::VirtualCamerasPrivate(VirtualCameras *self):
    self(self)
{
}

void VirtualCamerasPrivate::connectSignals()
{
    if (!this->m_cameraOutput)
        return;

    auto output = this->m_cameraOutput.data();
    QObject::connect(output,
                     SIGNAL(deviceChanged(QString)),
                     self,
                     SLOT(onOutputChanged(QString)));
    QObject::connect(output,
                     SIGNAL(descriptionChanged(QString)),
                     self,
                     SLOT(onOutputDescriptionChanged(QString)));
    QObject::connect(output,
                     SIGNAL(outputCapsChanged(AkVideoCaps)),
                     self,
                     SLOT(onOutputCapsChanged(AkVideoCaps)));
    QObject::connect(output,
                     SIGNAL(aspectRatioModeChanged(AkVideoConverter::AspectRatioMode)),
                     self,
                     SLOT(onAspectRatioModeChanged(AkVideoConverter::AspectRatioMode)));
    QObject::connect(output,
                     SIGNAL(webcamCreated(bool,QString)),
                     self,
                     SLOT(onOutputCreated(bool,QString)));
    QObject::connect(output,
                     SIGNAL(descriptionEdited(bool,QString)),
                     self,
                     SLOT(onOutputEdited(bool,QString)));
    QObject::connect(output,
                     SIGNAL(webcamRemoved(bool,QString)),
                     self,
                     SLOT(onOutputRemoved(bool,QString)));
    QObject::connect(output,
                     SIGNAL(pictureChanged(QString)),
                     self,
                     SIGNAL(pictureChanged(QString)));
    QObject::connect(output,
                     SIGNAL(errorChanged(QString)),
                     self,
                     SIGNAL(outputErrorChanged(QString)));
}

void VirtualCamerasPrivate::loadProperties()
{
    if (!this->m_cameraOutput)
        return;

    QSettings config;
    config.beginGroup("VirtualCamera");

    if (config.contains("outputCaps/format")) {
        AkVideoCaps caps(AkVideoCaps::PixelFormat(config.value("outputCaps/format").toUInt()),
                         config.value("outputCaps/width", 640).toInt(),
                         config.value("outputCaps/height", 480).toInt(),
                         AkFrac(config.value("outputCaps/fps", "30/1").toString()));
        this->m_cameraOutput->setProperty("outputCaps",
                                          QVariant::fromValue(caps));
    }

    if (config.contains("aspectRatioMode")) {
        auto mode = config.value("aspectRatioMode").toInt();

        if (mode >= AkVideoConverter::AspectRatioMode_Ignore
            && mode <= AkVideoConverter::AspectRatioMode_Expanding)
            this->m_cameraOutput->setProperty("aspectRatioMode",
                                              QVariant::fromValue(AkVideoConverter::AspectRatioMode(mode)));
    }

    // The description is the key used by the plugin to find the camera.
    auto description = config.value("description").toString();

    if (!description.isEmpty())
        this->m_cameraOutput->setProperty("description", description);

    config.endGroup();
}

void VirtualCamerasPrivate::stopOutput()
{
    if (this->m_cameraOutput)
        this->m_cameraOutput->setState(AkElement::ElementStateNull);
}

void VirtualCamerasPrivate::restoreOutput()
{
    if (this->m_busy
        || !this->m_cameraOutput
        || this->m_state == AkElement::ElementStateNull
        || this->m_cameraOutput->property("device").toString().isEmpty())
        return;

    this->m_cameraOutput->setState(this->m_state);
}

QString VirtualCamerasPrivate::vcamDownloadUrl() const
{
    if (this->m_latestVersion.isEmpty())
        return {};

#ifdef Q_OS_LINUX
    #ifdef Q_PROCESSOR_X86
        return QString("https://github.com/webcamoid/akvcam/releases/download/%1/akvcam-installer-gui-linux-%1.run")
               .arg(this->m_latestVersion);
    #else
        return QString("https://github.com/webcamoid/akvcam/releases/download/%1/akvcam-installer-cli-linux-%1.run")
               .arg(this->m_latestVersion);
    #endif
#else
    return {};
#endif
}

#include "moc_virtualcameras.cpp"
