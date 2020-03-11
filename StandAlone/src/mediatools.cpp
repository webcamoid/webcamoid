/* Webcamoid, webcam capture application.
 * Copyright (C) 2015  Gonzalo Exequiel Pedone
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

#include <QApplication>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QMutex>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQuickItem>
#include <QResource>
#include <QSettings>
#include <QSize>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <ak.h>
#include <akcaps.h>
#include <akaudiocaps.h>
#include <akvideocaps.h>

#include "mediatools.h"
#include "videodisplay.h"
#include "iconsprovider.h"
#include "pluginconfigs.h"
#include "mediasource.h"
#include "audiolayer.h"
#include "videoeffects.h"
#include "recording.h"
#include "updates.h"
#include "clioptions.h"

#define COMMONS_PROJECT_URL "https://webcamoid.github.io/"
#define COMMONS_PROJECT_LICENSE_URL "https://raw.githubusercontent.com/webcamoid/webcamoid/master/COPYING"
#define COMMONS_PROJECT_DOWNLOADS_URL "https://webcamoid.github.io/#downloads"
#define COMMONS_PROJECT_ISSUES_URL "https://github.com/webcamoid/webcamoid/issues"
#define COMMONS_COPYRIGHT_NOTICE "Copyright (C) 2011-2020  Gonzalo Exequiel Pedone"

class MediaToolsPrivate
{
    public:
        QQmlApplicationEngine *m_engine {nullptr};
        PluginConfigsPtr m_pluginConfigs;
        MediaSourcePtr m_mediaSource;
        AudioLayerPtr m_audioLayer;
        VideoEffectsPtr m_videoEffects;
        RecordingPtr m_recording;
        UpdatesPtr m_updates;
        AkElementPtr m_virtualCamera;
        QSystemTrayIcon *m_trayIcon {nullptr};
        CliOptions m_cliOptions;
        int m_windowWidth {0};
        int m_windowHeight {0};
        bool m_enableVirtualCamera {false};

        bool embedInterface(QQmlApplicationEngine *engine,
                            QObject *ctrlInterface,
                            const QString &where) const;
};

MediaTools::MediaTools(QObject *parent):
    QObject(parent)
{
    this->d = new MediaToolsPrivate;

    // Initialize environment.
    this->d->m_trayIcon = new QSystemTrayIcon(QApplication::windowIcon(), this);
    this->d->m_engine = new QQmlApplicationEngine();
    this->d->m_engine->addImageProvider(QLatin1String("icons"), new IconsProvider);
    Ak::setQmlEngine(this->d->m_engine);
    this->d->m_pluginConfigs =
            PluginConfigsPtr(new PluginConfigs(this->d->m_cliOptions,
                                               this->d->m_engine));
    this->d->m_mediaSource = MediaSourcePtr(new MediaSource(this->d->m_engine));
    this->d->m_audioLayer = AudioLayerPtr(new AudioLayer(this->d->m_engine));
    this->d->m_videoEffects = VideoEffectsPtr(new VideoEffects(this->d->m_engine));
    this->d->m_recording = RecordingPtr(new Recording(this->d->m_engine));
    this->d->m_updates = UpdatesPtr(new Updates(this->d->m_engine));
    this->d->m_virtualCamera = AkElement::create("VirtualCamera");

    if (this->d->m_virtualCamera) {
        AkElement::link(this->d->m_videoEffects.data(),
                        this->d->m_virtualCamera.data(),
                        Qt::DirectConnection);
        QObject::connect(this->d->m_virtualCamera.data(),
                         SIGNAL(stateChanged(AkElement::ElementState)),
                         this,
                         SIGNAL(virtualCameraStateChanged(AkElement::ElementState)));
    }

    AkElement::link(this->d->m_mediaSource.data(),
                    this->d->m_videoEffects.data(),
                    Qt::DirectConnection);
    AkElement::link(this->d->m_mediaSource.data(),
                    this->d->m_audioLayer.data(),
                    Qt::DirectConnection);
    AkElement::link(this->d->m_videoEffects.data(),
                    this->d->m_recording.data(),
                    Qt::DirectConnection);
    AkElement::link(this->d->m_audioLayer.data(),
                    this->d->m_recording.data(),
                    Qt::DirectConnection);
    QObject::connect(this->d->m_mediaSource.data(),
                     &MediaSource::error,
                     this,
                     &MediaTools::error);
    QObject::connect(this->d->m_mediaSource.data(),
                     &MediaSource::stateChanged,
                     this->d->m_videoEffects.data(),
                     &VideoEffects::setState);
    QObject::connect(this->d->m_mediaSource.data(),
                     &MediaSource::stateChanged,
                     this->d->m_audioLayer.data(),
                     &AudioLayer::setOutputState);
    QObject::connect(this->d->m_recording.data(),
                     &Recording::stateChanged,
                     this->d->m_audioLayer.data(),
                     &AudioLayer::setInputState);
    QObject::connect(this->d->m_mediaSource.data(),
                     &MediaSource::audioCapsChanged,
                     this->d->m_audioLayer.data(),
                     &AudioLayer::setInputCaps);
    QObject::connect(this->d->m_mediaSource.data(),
                     &MediaSource::streamChanged,
                     this->d->m_audioLayer.data(),
                     [this] (const QString &stream)
                     {
                         this->d->m_audioLayer->setInputDescription(this->d->m_mediaSource->description(stream));
                     });
    QObject::connect(this->d->m_mediaSource.data(),
                     &MediaSource::streamChanged,
                     this,
                     &MediaTools::updateVCamState);
    QObject::connect(this->d->m_mediaSource.data(),
                     &MediaSource::videoCapsChanged,
                     this,
                     &MediaTools::updateVCamCaps);
    QObject::connect(this,
                     &MediaTools::enableVirtualCameraChanged,
                     this,
                     &MediaTools::updateVCamState);
    QObject::connect(this->d->m_pluginConfigs.data(),
                     &PluginConfigs::pluginsChanged,
                     this->d->m_videoEffects.data(),
                     &VideoEffects::updateEffects);
    QObject::connect(this->d->m_audioLayer.data(),
                     &AudioLayer::outputCapsChanged,
                     this->d->m_recording.data(),
                     &Recording::setAudioCaps);
    QObject::connect(this->d->m_mediaSource.data(),
                     &MediaSource::videoCapsChanged,
                     this->d->m_recording.data(),
                     &Recording::setVideoCaps);
    QObject::connect(qApp,
                     &QCoreApplication::aboutToQuit,
                     this->d->m_mediaSource.data(),
                     [this] () {
                        this->d->m_mediaSource->setState(AkElement::ElementStateNull);
                     });

    this->loadConfigs();
    this->updateVCamCaps(this->d->m_mediaSource->videoCaps());
    this->d->m_recording->setVideoCaps(this->d->m_mediaSource->videoCaps());
    this->d->m_recording->setAudioCaps(this->d->m_audioLayer->outputCaps());
    this->d->m_audioLayer->setInputCaps(this->d->m_mediaSource->audioCaps());
    this->d->m_audioLayer->setInputDescription(this->d->m_mediaSource->description(this->d->m_mediaSource->stream()));
}

MediaTools::~MediaTools()
{
    this->saveConfigs();
    delete this->d->m_engine;
    delete this->d;
}

int MediaTools::windowWidth() const
{
    return this->d->m_windowWidth;
}

int MediaTools::windowHeight() const
{
    return this->d->m_windowHeight;
}

bool MediaTools::enableVirtualCamera() const
{
    return this->d->m_enableVirtualCamera;
}

AkElement::ElementState MediaTools::virtualCameraState() const
{
    if (this->d->m_virtualCamera)
        return this->d->m_virtualCamera->state();

    return AkElement::ElementStateNull;
}

QString MediaTools::applicationName() const
{
    return QCoreApplication::applicationName();
}

QString MediaTools::applicationVersion() const
{
#ifdef DAILY_BUILD
    return QString(tr("Daily Build"));
#else
    return QCoreApplication::applicationVersion();
#endif
}

QString MediaTools::qtVersion() const
{
    return QT_VERSION_STR;
}

QString MediaTools::copyrightNotice() const
{
    return COMMONS_COPYRIGHT_NOTICE;
}

QString MediaTools::projectUrl() const
{
    return COMMONS_PROJECT_URL;
}

QString MediaTools::projectLicenseUrl() const
{
    return COMMONS_PROJECT_LICENSE_URL;
}

QString MediaTools::projectDownloadsUrl() const
{
    return COMMONS_PROJECT_DOWNLOADS_URL;
}

QString MediaTools::projectIssuesUrl() const
{
    return COMMONS_PROJECT_ISSUES_URL;
}

QString MediaTools::fileNameFromUri(const QString &uri) const
{
    return QFileInfo(uri).baseName();
}

bool MediaTools::matches(const QString &pattern,
                         const QStringList &strings) const
{
    if (pattern.isEmpty())
        return true;

    for (const QString &str: strings)
        if (str.contains(QRegExp(pattern,
                                 Qt::CaseInsensitive,
                                 QRegExp::Wildcard)))
            return true;

    return false;
}

QString MediaTools::currentTime() const
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss");
}

QStringList MediaTools::standardLocations(const QString &type) const
{
    static const QMap<QString, QStandardPaths::StandardLocation> stdPaths = {
        {"movies"  , QStandardPaths::MoviesLocation  },
        {"pictures", QStandardPaths::PicturesLocation},
    };

    if (stdPaths.contains(type))
        return QStandardPaths::standardLocations(stdPaths[type]);

    return QStringList();
}

QString MediaTools::readFile(const QString &fileName)
{
    QFile file(fileName);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString data = file.readAll();
    file.close();

    return data;
}

QString MediaTools::urlToLocalFile(const QUrl &url) const
{
    return url.toLocalFile();
}

bool MediaTools::embedVirtualCameraControls(const QString &where,
                                            const QString &name)
{
    if (!this->d->m_virtualCamera)
        return false;

    auto ctrlInterface = this->d->m_virtualCamera->controlInterface(this->d->m_engine, "");

    if (!ctrlInterface)
        return false;

    if (!name.isEmpty())
        ctrlInterface->setObjectName(name);

    return this->d->embedInterface(this->d->m_engine, ctrlInterface, where);
}

void MediaTools::removeInterface(const QString &where,
                                 QQmlApplicationEngine *engine)
{
    if (!engine)
        engine = this->d->m_engine;

    if (!engine)
        return;

    for (auto &obj: engine->rootObjects()) {
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        auto childItems = item->childItems();

        for (QQuickItem *child: childItems) {
            child->setParentItem(nullptr);
            child->setParent(nullptr);

            delete child;
        }
    }
}

QString MediaTools::convertToAbsolute(const QString &path)
{
    if (!QDir::isRelativePath(path))
        return QDir::cleanPath(path);

    static const QDir applicationDir(QCoreApplication::applicationDirPath());
    QString absPath = applicationDir.absoluteFilePath(path);

    return QDir::cleanPath(absPath).replace('/', QDir::separator());
}

bool MediaToolsPrivate::embedInterface(QQmlApplicationEngine *engine,
                                       QObject *ctrlInterface,
                                       const QString &where) const
{
    if (!engine || !ctrlInterface)
        return false;

    for (auto &obj: engine->rootObjects()) {
        // First, find where to embed the UI.
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        // Create an item with the plugin context.
        auto interfaceItem = qobject_cast<QQuickItem *>(ctrlInterface);

        // Finally, embed the plugin item UI in the desired place.
        interfaceItem->setParentItem(item);

        return true;
    }

    return false;
}

void MediaTools::setWindowWidth(int windowWidth)
{
    if (this->d->m_windowWidth == windowWidth)
        return;

    this->d->m_windowWidth = windowWidth;
    emit this->windowWidthChanged(windowWidth);
}

void MediaTools::setWindowHeight(int windowHeight)
{
    if (this->d->m_windowHeight == windowHeight)
        return;

    this->d->m_windowHeight = windowHeight;
    emit this->windowHeightChanged(windowHeight);
}

void MediaTools::setEnableVirtualCamera(bool enableVirtualCamera)
{
    if (this->d->m_enableVirtualCamera == enableVirtualCamera)
        return;

    this->d->m_enableVirtualCamera = enableVirtualCamera;
    emit this->enableVirtualCameraChanged(enableVirtualCamera);
}

void MediaTools::setVirtualCameraState(AkElement::ElementState virtualCameraState)
{
    if (this->d->m_virtualCamera) {
        auto state = virtualCameraState;
        auto vcamStream = this->d->m_virtualCamera->property("media").toString();

        if (this->d->m_enableVirtualCamera
            && virtualCameraState == AkElement::ElementStatePlaying
            && this->d->m_mediaSource->state() == AkElement::ElementStatePlaying
            && this->d->m_mediaSource->stream() == vcamStream) {
            // Prevents self blocking by pausing the virtual camera.
            state = AkElement::ElementStatePaused;
        }

        this->d->m_virtualCamera->setState(state);
    }
}

void MediaTools::resetWindowWidth()
{
    this->setWindowWidth(0);
}

void MediaTools::resetWindowHeight()
{
    this->setWindowHeight(0);
}

void MediaTools::resetEnableVirtualCamera()
{
    this->setEnableVirtualCamera(false);
}

void MediaTools::resetVirtualCameraState()
{
    this->setVirtualCameraState(AkElement::ElementStateNull);
}

void MediaTools::loadConfigs()
{
    QSettings config;

    config.beginGroup("GeneralConfigs");
    QSize windowSize = config.value("windowSize", QSize(1024, 600)).toSize();
    this->d->m_windowWidth = windowSize.width();
    this->d->m_windowHeight = windowSize.height();
    config.endGroup();

    config.beginGroup("VirtualCamera");
    this->setEnableVirtualCamera(config.value("enable", false).toBool());
    config.endGroup();

    auto optPaths =
            this->d->m_cliOptions.value(this->d->m_cliOptions.vcamPathOpt()).split(';');

    QStringList driverPaths;

    for (auto path: optPaths) {
        path = this->convertToAbsolute(path);

        if (QFileInfo::exists(path))
            driverPaths << path;
    }

    if (this->d->m_virtualCamera)
        QMetaObject::invokeMethod(this->d->m_virtualCamera.data(),
                                  "addDriverPaths",
                                  Q_ARG(QStringList, driverPaths));
}

void MediaTools::saveConfigs()
{
    QSettings config;

    config.beginGroup("GeneralConfigs");
    config.setValue("windowSize", QSize(this->d->m_windowWidth,
                                        this->d->m_windowHeight));
    config.endGroup();

    config.beginGroup("VirtualCamera");
    config.setValue("enable", this->enableVirtualCamera());
    config.endGroup();
}

void MediaTools::show()
{
    // @uri Webcamoid
    qmlRegisterType<VideoDisplay>("Webcamoid", 1, 0, "VideoDisplay");
    this->d->m_engine->rootContext()->setContextProperty("Webcamoid", this);

    // Map tray icon to QML
    this->d->m_engine->rootContext()->setContextProperty("trayIcon", this->d->m_trayIcon);

    // Map tray icon enums to QML
    this->d->m_engine->rootContext()->setContextProperty("TrayIcon_NoIcon", QSystemTrayIcon::NoIcon);
    this->d->m_engine->rootContext()->setContextProperty("TrayIcon_Information", QSystemTrayIcon::Information);
    this->d->m_engine->rootContext()->setContextProperty("TrayIcon_Warning", QSystemTrayIcon::Warning);
    this->d->m_engine->rootContext()->setContextProperty("TrayIcon_Critical", QSystemTrayIcon::Critical);

    this->d->m_engine->load(QUrl(QStringLiteral("qrc:/Webcamoid/share/qml/main.qml")));

    for (auto &obj: this->d->m_engine->rootObjects()) {
        // First, find where to enbed the UI.
        auto videoDisplay = obj->findChild<VideoDisplay *>("videoDisplay");

        if (!videoDisplay)
            continue;

        AkElement::link(this->d->m_videoEffects.data(),
                        videoDisplay,
                        Qt::DirectConnection);
        break;
    }

    emit this->interfaceLoaded();
}

void MediaTools::makedirs(const QString &path)
{
    QDir().mkpath(path);
}

void MediaTools::updateVCamCaps(const AkCaps &videoCaps)
{
    if (!this->d->m_virtualCamera)
        return;

    QMetaObject::invokeMethod(this->d->m_virtualCamera.data(),
                              "clearStreams");
    QMetaObject::invokeMethod(this->d->m_virtualCamera.data(),
                              "addStream",
                              Q_ARG(int, 0),
                              Q_ARG(AkCaps, videoCaps));
}

void MediaTools::updateVCamState()
{
    if (!this->d->m_virtualCamera)
        return;

    if (this->d->m_enableVirtualCamera) {
        if (this->d->m_mediaSource->state() == AkElement::ElementStatePlaying) {
            auto vcamStream = this->d->m_virtualCamera->property("media").toString();

            // Prevents self blocking by pausing the virtual camera.
            auto state = this->d->m_mediaSource->stream() == vcamStream?
                             AkElement::ElementStatePaused:
                             AkElement::ElementStatePlaying;
            this->d->m_virtualCamera->setState(state);
        }
    } else
        this->d->m_virtualCamera->setState(AkElement::ElementStateNull);
}

#include "moc_mediatools.cpp"
