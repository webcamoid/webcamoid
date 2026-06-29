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
#include <akpacket.h>
#include <akvideocaps.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <iak/akelement.h>

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

#include "virtualcameras.h"
#include "updates.h"

#ifdef Q_OS_WIN32
    #define DEFAULT_VCAM_DRIVER "VideoSink/VirtualCamera/Impl/AkVirtualCameraDShow"
#elif defined(Q_OS_LINUX)
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
        QStringList m_selectedOutputs;
        AkElementPtr m_cameraOutput {akPluginManager->create<AkElement>("VideoSink/VirtualCamera")};
        QMap<QString, AkElementPtr> m_outputPlugins;
        QString m_vcamDriver;
        QThreadPool m_threadPool;
        AkElement::ElementState m_state {AkElement::ElementStateNull};
        QString m_latestVersion;
        bool m_currentVCamInstalled {false};
        bool m_isPassThroughVCam {false};

        explicit VirtualCamerasPrivate(VirtualCameras *self);
        void connectSignals();
        bool embedControls(const QString &where,
                           const AkElementPtr &element,
                           const QString &pluginId,
                           const QString &name) const;
        static QString sanitizeKey(const QString &key);
        void loadProperties();
        void updateOutputPlugins();
        void saveOutputs();
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

                bool isPassThrough =
                        this->d->m_cameraOutput->property("isPassThrough").toBool();

                if (isPassThrough != this->d->m_isPassThroughVCam) {
                    this->d->m_isPassThroughVCam = isPassThrough;
                    emit this->isPassThroughVCamChanged(isPassThrough);
                }
            } else {
                if (this->d->m_isPassThroughVCam) {
                    this->d->m_isPassThroughVCam = false;
                    emit this->isPassThroughVCamChanged(false);
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

        this->d->m_isPassThroughVCam =
                this->d->m_cameraOutput->property("isPassThrough").toBool();
    }
}

VirtualCameras::~VirtualCameras()
{
    this->setState(AkElement::ElementStateNull);
    this->d->saveOutputs();
    delete this->d;
}

QStringList VirtualCameras::outputs() const
{
    QStringList outputs;

    if (this->d->m_cameraOutput)
        outputs = this->d->m_cameraOutput->property("medias").toStringList();

    return outputs;
}

QStringList VirtualCameras::selectedOutputs() const
{
    return this->d->m_selectedOutputs;
}

AkVideoCaps::PixelFormatList VirtualCameras::supportedOutputPixelFormats() const
{
    if (!this->d->m_cameraOutput)
        return {};

    AkVideoCaps::PixelFormatList pixelFormats;
    QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                              "supportedOutputPixelFormats",
                              Q_RETURN_ARG(AkVideoCaps::PixelFormatList, pixelFormats));

    return pixelFormats;
}

AkVideoCaps::PixelFormat VirtualCameras::defaultOutputPixelFormat() const
{
    if (!this->d->m_cameraOutput)
        return AkVideoCaps::Format_none;

    AkVideoCaps::PixelFormat pixelFormat;
    QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                              "defaultOutputPixelFormat",
                              Q_RETURN_ARG(AkVideoCaps::PixelFormat, pixelFormat));

    return pixelFormat;
}

AkVideoCapsList VirtualCameras::supportedOutputVideoCaps(const QString &device) const
{
    if (!this->d->m_cameraOutput)
        return {};

    AkVideoCapsList caps;
    QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                              "outputCaps",
                              Q_RETURN_ARG(AkVideoCapsList, caps),
                              Q_ARG(QString, device));

    return caps;
}

AkElement::ElementState VirtualCameras::state() const
{
    return this->d->m_state;
}

QString VirtualCameras::description(const QString &device) const
{
    QString description;

    if (this->d->m_cameraOutput)
        QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                                  "description",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, device));

    return description;
}

QString VirtualCameras::createOutput(const QString &description,
                                   const AkVideoCapsList &formats)
{
    if (!this->d->m_cameraOutput)
        return {};

    QString deviceId;
    QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                              "createWebcam",
                              Q_RETURN_ARG(QString, deviceId),
                              Q_ARG(QString, description),
                              Q_ARG(AkVideoCapsList, formats));

    return deviceId;
}

QString VirtualCameras::createOutput(const QString &description,
                                   const QVariantList &formats)
{
    AkVideoCapsList fmts;

    for (auto &format: formats)
        fmts << format.value<AkVideoCaps>();

    return this->createOutput(description, fmts);
}

bool VirtualCameras::editOutput(const QString &output,
                            const QString &description,
                            const AkVideoCapsList &formats)
{
    if (!this->d->m_cameraOutput)
        return {};

    bool result;
    QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                              "editWebcam",
                              Q_RETURN_ARG(bool, result),
                              Q_ARG(QString, output),
                              Q_ARG(QString, description),
                              Q_ARG(AkVideoCapsList, formats));

    return result;
}

bool VirtualCameras::removeOutput(const QString &output)
{
    if (!this->d->m_cameraOutput)
        return {};

    bool result;
    QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                              "removeWebcam",
                              Q_RETURN_ARG(bool, result),
                              Q_ARG(QString, output));

    return result;
}

bool VirtualCameras::removeAllOutputs()
{
    if (!this->d->m_cameraOutput)
        return {};

    bool result;
    QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                              "removeAllWebcams",
                              Q_RETURN_ARG(bool, result));

    return result;
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

bool VirtualCameras::embedOutputControls(const QString &where,
                                       const QString &device,
                                       const QString &name) const
{
    auto element = this->d->m_outputPlugins.value(device);

    if (!element)
        return false;

    return this->d->embedControls(where,
                                  element,
                                  "VideoSink/VirtualCamera",
                                  name);
}

void VirtualCameras::removeInterface(const QString &where) const
{
    if (!this->d->m_engine)
        return;

    for (auto &obj: this->d->m_engine->rootObjects()) {
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        auto childItems = item->childItems();

        for (auto &child: childItems) {
            child->setParentItem(nullptr);
            child->setParent(nullptr);
            delete child;
        }
    }
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

QString VirtualCameras::rootMethod() const
{
    if (!this->d->m_cameraOutput)
        return {};

    return this->d->m_cameraOutput->property("rootMethod").toString();
}

QStringList VirtualCameras::availableRootMethods() const
{
    if (!this->d->m_cameraOutput)
        return {};

    return this->d->m_cameraOutput->property("availableRootMethods").toStringList();
}

bool VirtualCameras::isVCamSupported() const
{
#if defined(Q_OS_WIN32) \
    || (defined(Q_OS_BSD4) && !defined(Q_OS_DARWIN))
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
#if defined(Q_OS_WIN32)
    return {"https://api.github.com/repos/webcamoid/akvirtualcamera/releases/latest"};
#elif defined(Q_OS_LINUX)
    return {"https://api.github.com/repos/webcamoid/akvcam/releases/latest"};
#else
    return {};
#endif
}

QString VirtualCameras::vcamDownloadUrl() const
{
#if defined(Q_OS_WIN32)
    return {"https://github.com/webcamoid/akvirtualcamera/releases/latest"};
#elif defined(Q_OS_LINUX)
    return {"https://github.com/webcamoid/akvcam/releases/latest"};
#else
    return {};
#endif
}

QString VirtualCameras::defaultVCamDriver() const
{
    return {DEFAULT_VCAM_DRIVER};
}

bool VirtualCameras::isPassThroughVCam() const
{
    return this->d->m_isPassThroughVCam;
}

AkPacket VirtualCameras::iStream(const AkPacket &packet)
{
    if (this->d->m_state == AkElement::ElementStatePlaying)
        for (auto &output: this->d->m_outputPlugins)
            output->iStream(packet);

    return {};
}

bool VirtualCameras::applyPicture()
{
    if (!this->d->m_cameraOutput)
        return {};

    bool ok = false;
    QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                              "applyPicture",
                              Q_RETURN_ARG(bool, ok));

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
        QString errorString = "Can't execute installer";

#ifdef Q_OS_WIN32
        SHELLEXECUTEINFOA execInfo;
        memset(&execInfo, 0, sizeof(SHELLEXECUTEINFOA));
        execInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
        execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        execInfo.hwnd = nullptr;
        execInfo.lpVerb = "runas";
        execInfo.lpFile = installer.toStdString().c_str();
        execInfo.lpParameters = "";
        execInfo.lpDirectory = "";
        execInfo.nShow = SW_SHOWNORMAL;
        execInfo.hInstApp = nullptr;
        ShellExecuteExA(&execInfo);

        if (execInfo.hProcess) {
            WaitForSingleObject(execInfo.hProcess, INFINITE);

            DWORD dExitCode;
            GetExitCodeProcess(execInfo.hProcess, &dExitCode);
            CloseHandle(execInfo.hProcess);

            if (dExitCode == 0)
                errorString = "";
            else
                errorString = QString("Installer failed with code %1").arg(exitCode);

            exitCode = int(dExitCode);
        }
#else
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
        errorString = proc.errorString();

    #ifndef Q_PROCESSOR_X86
        emit this->vcamCliInstallFinished();
    #endif
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

void VirtualCameras::setSelectedOutputs(const QStringList &selectedOutputs)
{
    if (this->d->m_selectedOutputs == selectedOutputs)
        return;

    this->d->m_selectedOutputs = selectedOutputs;
    emit this->selectedOutputsChanged(selectedOutputs);
    this->d->updateOutputPlugins();
    this->d->saveOutputs();
}

void VirtualCameras::setState(AkElement::ElementState state)
{
    if (this->d->m_state == state)
        return;

    this->d->m_state = state;
    emit this->stateChanged(state);

    for (auto &output: this->d->m_outputPlugins)
        output->setState(state);
}

void VirtualCameras::setPicture(const QString &picture)
{
    if (this->d->m_cameraOutput)
        this->d->m_cameraOutput->setProperty("picture", picture);
}

void VirtualCameras::setRootMethod(const QString &rootMethod)
{
    if (this->d->m_cameraOutput)
        this->d->m_cameraOutput->setProperty("rootMethod", rootMethod);
}

void VirtualCameras::resetSelectedOutputs()
{
    this->setSelectedOutputs({});
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

void VirtualCameras::resetRootMethod()
{
    if (this->d->m_cameraOutput)
        QMetaObject::invokeMethod(this->d->m_cameraOutput.data(),
                                  "resetRootMethod");
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

void VirtualCameras::saveVirtualCameraRootMethod(const QString &rootMethod)
{
    QSettings config;
    config.beginGroup("VirtualCamera");
    config.setValue("rootMethod", rootMethod);
    config.endGroup();
}

VirtualCamerasPrivate::VirtualCamerasPrivate(VirtualCameras *self):
    self(self)
{
}

void VirtualCamerasPrivate::connectSignals()
{
    if (this->m_cameraOutput) {
        QObject::connect(this->m_cameraOutput.data(),
                         SIGNAL(mediasChanged(QStringList)),
                         self,
                         SIGNAL(outputsChanged(QStringList)));
        QObject::connect(this->m_cameraOutput.data(),
                         SIGNAL(pictureChanged(QString)),
                         self,
                         SIGNAL(pictureChanged(QString)));
        QObject::connect(this->m_cameraOutput.data(),
                         SIGNAL(errorChanged(QString)),
                         self,
                         SIGNAL(outputErrorChanged(QString)));
        QObject::connect(this->m_cameraOutput.data(),
                         SIGNAL(rootMethodChanged(QString)),
                         self,
                         SIGNAL(rootMethodChanged(QString)),
                         Qt::DirectConnection);
        QObject::connect(this->m_cameraOutput.data(),
                         SIGNAL(rootMethodChanged(QString)),
                         self,
                         SLOT(saveVirtualCameraRootMethod(QString)));
    }
}

bool VirtualCamerasPrivate::embedControls(const QString &where,
                                        const AkElementPtr &element,
                                        const QString &pluginId,
                                        const QString &name) const
{
    if (!element)
        return false;

    auto controlInterface = element->controlInterface(this->m_engine, pluginId);

    if (!controlInterface)
        return false;

    if (!name.isEmpty())
        controlInterface->setObjectName(name);

    for (auto &obj: this->m_engine->rootObjects()) {
        // First, find where to embed the UI.
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        // Create an item with the plugin context.
        auto interfaceItem = qobject_cast<QQuickItem *>(controlInterface);

        // Finally, embed the plugin item UI in the desired place.
        interfaceItem->setParentItem(item);

        return true;
    }

    return false;
}

QString VirtualCamerasPrivate::sanitizeKey(const QString &key)
{
    QString sanitized(key);

    return sanitized.replace(" ", "_")
                    .replace(".", "_")
                    .replace(",", "_");
}

void VirtualCamerasPrivate::loadProperties()
{
    QSettings config;

    config.beginGroup("VirtualCamera");

    if (this->m_cameraOutput) {
        auto rootMethod =
                config.value("rootMethod",
                             this->m_cameraOutput->property("rootMethod")).toString();
        auto availableMethods =
                this->m_cameraOutput->property("availableRootMethods").toStringList();

        if (availableMethods.contains(rootMethod))
            this->m_cameraOutput->setProperty("rootMethod", rootMethod);

        auto medias = this->m_cameraOutput->property("medias").toStringList();
        int size = config.beginReadArray("outputs");

        for (int i = 0; i < size; i++) {
            config.setArrayIndex(i);
            auto output = config.value("output").toString();

            if (!medias.contains(output))
                this->m_selectedOutputs << output;
        }

        config.endArray();
    }

    config.endGroup();

    this->updateOutputPlugins();
}

void VirtualCamerasPrivate::updateOutputPlugins()
{
    this->m_outputPlugins.clear();
    QStringList outputs;

    if (this->m_cameraOutput)
        outputs = this->m_cameraOutput->property("medias").toStringList();

    QSettings config;

    for (auto &output: outputs) {
        auto out = akPluginManager->create<AkElement>("VideoSink/VirtualCamera");
        out->setProperty("media", output);

        config.beginGroup("VirtualCamera_" + sanitizeKey(output));
        auto controlKeys = config.allKeys();
        QVariantMap controls;

        for (const auto &key: controlKeys)
            controls[key] = config.value(key);

        config.endGroup();

        if (!controls.isEmpty())
            QMetaObject::invokeMethod(out.data(),
                                      "setControls",
                                      Q_ARG(QVariantMap, controls));

        this->m_outputPlugins[output] = out;
    }
}

void VirtualCamerasPrivate::saveOutputs()
{
    QStringList outputs;
    AkElement::ElementState curState = AkElement::ElementStateNull;

    if (this->m_state != AkElement::ElementStateNull) {
        curState = this->m_state;
        self->setState(AkElement::ElementStateNull);
    }

    if (this->m_cameraOutput)
        outputs = this->m_cameraOutput->property("medias").toStringList();

    if (outputs.isEmpty())
        return;

    QSettings config;
    config.beginGroup("VirtualCamera");
    config.beginWriteArray("outputs");

    int i = 0;

    for (const auto &output: outputs) {
        config.setArrayIndex(i);
        config.setValue("output", output);
        i++;
    }

    config.endArray();
    config.endGroup();

    for (auto it = this->m_outputPlugins.begin();
         it != this->m_outputPlugins.end();
         ++it) {
        config.beginGroup("VirtualCamera_" + sanitizeKey(it.key()));
        QVariantList controls;
        QMetaObject::invokeMethod(it.value().data(),
                                  "controls",
                                  Q_RETURN_ARG(QVariantList, controls));

        for (const auto &control: controls) {
            auto controlValues = control.toList();
            config.setValue(sanitizeKey(controlValues[0].toString()), controlValues[7]);
        }

        config.endGroup();
    }

    if (curState != AkElement::ElementStateNull)
        self->setState(curState);
}

QString VirtualCamerasPrivate::vcamDownloadUrl() const
{
    if (this->m_latestVersion.isEmpty())
        return {};

#if defined(Q_OS_WIN32)
    return QString("https://github.com/webcamoid/akvirtualcamera/releases/download/%1/akvirtualcamera-windows-%1.exe")
           .arg(this->m_latestVersion);
#elif defined(Q_OS_LINUX)
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
