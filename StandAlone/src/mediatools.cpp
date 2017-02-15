/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include <QSettings>
#include <QFileInfo>
#include <QDateTime>
#include <QStandardPaths>
#include <QFileDialog>

#include "videodisplay.h"
#include "iconsprovider.h"
#include "mediatools.h"

#define COMMONS_PROJECT_URL "http://webcamoid.github.io/"
#define COMMONS_PROJECT_LICENSE_URL "https://raw.githubusercontent.com/webcamoid/webcamoid/master/COPYING"
#define COMMONS_COPYRIGHT_NOTICE "Copyright (C) 2011-2017  Gonzalo Exequiel Pedone"

MediaTools::MediaTools(QObject *parent):
    QObject(parent),
    m_windowWidth(0),
    m_windowHeight(0),
    m_enableVirtualCamera(false)
{
    // Initialize environment.
    CliOptions cliOptions;

    this->m_engine = new QQmlApplicationEngine();
    this->m_engine->addImageProvider(QLatin1String("icons"), new IconsProvider);
    Ak::setQmlEngine(this->m_engine);
    this->m_pluginConfigs = PluginConfigsPtr(new PluginConfigs(cliOptions, this->m_engine));
    this->m_mediaSource = MediaSourcePtr(new MediaSource(this->m_engine));
    this->m_audioLayer = AudioLayerPtr(new AudioLayer(this->m_engine));
    this->m_videoEffects = VideoEffectsPtr(new VideoEffects(this->m_engine));
    this->m_recording = RecordingPtr(new Recording(this->m_engine));
    this->m_virtualCamera = AkElement::create("VirtualCamera");

    if (this->m_virtualCamera) {
        AkElement::link(this->m_videoEffects.data(),
                        this->m_virtualCamera.data(),
                        Qt::DirectConnection);
        QObject::connect(this->m_virtualCamera.data(),
                         SIGNAL(stateChanged(AkElement::ElementState)),
                         this,
                         SIGNAL(virtualCameraStateChanged(AkElement::ElementState)));
        QObject::connect(this->m_virtualCamera.data(),
                         SIGNAL(convertLibChanged(const QString &)),
                         this,
                         SLOT(saveVirtualCameraConvertLib(const QString &)));
        QObject::connect(this->m_virtualCamera.data(),
                         SIGNAL(outputLibChanged(const QString &)),
                         this,
                         SLOT(saveVirtualCameraOutputLib(const QString &)));
    }

    AkElement::link(this->m_mediaSource.data(),
                    this->m_videoEffects.data(),
                    Qt::DirectConnection);
    AkElement::link(this->m_mediaSource.data(),
                    this->m_audioLayer.data(),
                    Qt::DirectConnection);
    AkElement::link(this->m_videoEffects.data(),
                    this,
                    Qt::DirectConnection);
    AkElement::link(this->m_videoEffects.data(),
                    this->m_recording.data(),
                    Qt::DirectConnection);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::error,
                     this,
                     &MediaTools::error);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::stateChanged,
                     this->m_videoEffects.data(),
                     &VideoEffects::setState);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::stateChanged,
                     this->m_audioLayer.data(),
                     &AudioLayer::setOutputState);
    QObject::connect(this->m_recording.data(),
                     &Recording::stateChanged,
                     this->m_audioLayer.data(),
                     &AudioLayer::setInputState);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::audioCapsChanged,
                     this->m_audioLayer.data(),
                     &AudioLayer::setInputCaps);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::streamChanged,
                     [this] (const QString &stream)
                     {
                         this->m_audioLayer->setInputDescription(this->m_mediaSource->description(stream));
                     });
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::streamChanged,
                     this,
                     &MediaTools::updateVCamState);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::videoCapsChanged,
                     this,
                     &MediaTools::updateVCamCaps);
    QObject::connect(this,
                     &MediaTools::enableVirtualCameraChanged,
                     this,
                     &MediaTools::updateVCamState);
    QObject::connect(this->m_pluginConfigs.data(),
                     &PluginConfigs::pluginsChanged,
                     this->m_videoEffects.data(),
                     &VideoEffects::updateEffects);
    QObject::connect(this->m_audioLayer.data(),
                     &AudioLayer::outputCapsChanged,
                     this->m_recording.data(),
                     &Recording::setAudioCaps);
    QObject::connect(this->m_mediaSource.data(),
                     &MediaSource::videoCapsChanged,
                     this->m_recording.data(),
                     &Recording::setVideoCaps);
    QObject::connect(qApp,
                     &QCoreApplication::aboutToQuit,
                     [this] () {
                        this->m_mediaSource->setState(AkElement::ElementStateNull);
                     });

    this->loadConfigs();
    this->updateVCamCaps(this->m_mediaSource->videoCaps());
    this->m_recording->setVideoCaps(this->m_mediaSource->videoCaps());
    this->m_recording->setAudioCaps(this->m_audioLayer->outputCaps());
    this->m_audioLayer->setInputCaps(this->m_mediaSource->audioCaps());
    this->m_audioLayer->setInputDescription(this->m_mediaSource->description(this->m_mediaSource->stream()));
}

MediaTools::~MediaTools()
{
    this->saveConfigs();
    delete this->m_engine;
}

int MediaTools::windowWidth() const
{
    return this->m_windowWidth;
}

int MediaTools::windowHeight() const
{
    return this->m_windowHeight;
}

bool MediaTools::enableVirtualCamera() const
{
    return this->m_enableVirtualCamera;
}

AkElement::ElementState MediaTools::virtualCameraState() const
{
    if (this->m_virtualCamera)
        return this->m_virtualCamera->state();

    return AkElement::ElementStateNull;
}

QString MediaTools::applicationName() const
{
    return QCoreApplication::applicationName();
}

QString MediaTools::applicationVersion() const
{
    return QCoreApplication::applicationVersion();
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

QString MediaTools::saveFileDialog(const QString &caption,
                                   const QString &fileName,
                                   const QString &directory,
                                   const QString &suffix,
                                   const QString &filters) const
{
    QFileDialog saveFileDialog(NULL,
                               caption,
                               fileName,
                               filters);

    saveFileDialog.setModal(true);
    saveFileDialog.setDefaultSuffix(suffix);
    saveFileDialog.setDirectory(directory);
    saveFileDialog.setFileMode(QFileDialog::AnyFile);
    saveFileDialog.setAcceptMode(QFileDialog::AcceptSave);

    if (saveFileDialog.exec() == QDialog::Accepted)
        return saveFileDialog.selectedFiles().value(0);

    return QString();
}

QString MediaTools::readFile(const QString &fileName) const
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
    if (!this->m_virtualCamera)
        return false;

    auto ctrlInterface = this->m_virtualCamera->controlInterface(this->m_engine, "");

    if (!ctrlInterface)
        return false;

    if (!name.isEmpty())
        ctrlInterface->setObjectName(name);

    return this->embedInterface(this->m_engine, ctrlInterface, where);
}

void MediaTools::removeInterface(const QString &where,
                                 QQmlApplicationEngine *engine)
{
    if (!engine)
        engine = this->m_engine;

    if (!engine)
        return;

    for (const QObject *obj: engine->rootObjects()) {
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        auto childItems = item->childItems();

        for (QQuickItem *child: childItems) {
            child->setParentItem(NULL);
            child->setParent(NULL);

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

bool MediaTools::embedInterface(QQmlApplicationEngine *engine,
                                QObject *ctrlInterface,
                                const QString &where) const
{
    if (!engine || !ctrlInterface)
        return false;

    for (const QObject *obj: engine->rootObjects()) {
        // First, find where to embed the UI.
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        // Create an item with the plugin context.
        auto interfaceItem = qobject_cast<QQuickItem *>(ctrlInterface);

        // Finally, embed the plugin item UI in the desired place.
        interfaceItem->setParentItem(item);
        interfaceItem->setParent(item);

        QQmlProperty::write(interfaceItem,
                            "anchors.fill",
                            qVariantFromValue(item));

        return true;
    }

    return false;
}

void MediaTools::setWindowWidth(int windowWidth)
{
    if (this->m_windowWidth == windowWidth)
        return;

    this->m_windowWidth = windowWidth;
    emit this->windowWidthChanged(windowWidth);
}

void MediaTools::setWindowHeight(int windowHeight)
{
    if (this->m_windowHeight == windowHeight)
        return;

    this->m_windowHeight = windowHeight;
    emit this->windowHeightChanged(windowHeight);
}

void MediaTools::setEnableVirtualCamera(bool enableVirtualCamera)
{
    if (this->m_enableVirtualCamera == enableVirtualCamera)
        return;

    this->m_enableVirtualCamera = enableVirtualCamera;
    emit this->enableVirtualCameraChanged(enableVirtualCamera);
}

void MediaTools::setVirtualCameraState(AkElement::ElementState virtualCameraState)
{
    if (this->m_virtualCamera) {
        auto state = virtualCameraState;
        auto vcamStream = this->m_virtualCamera->property("media").toString();

        if (this->m_enableVirtualCamera
            && virtualCameraState == AkElement::ElementStatePlaying
            && this->m_mediaSource->state() == AkElement::ElementStatePlaying
            && this->m_mediaSource->stream() == vcamStream) {
            // Prevents self blocking by pausing the virtual camera.
            state = AkElement::ElementStatePaused;
        }

        this->m_virtualCamera->setState(state);
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

    config.beginGroup("Libraries");

    if (this->m_virtualCamera) {
        this->m_virtualCamera->setProperty("convertLib",
                                           config.value("VirtualCamera.convertLib",
                                                        this->m_virtualCamera->property("convertLib")));
        this->m_virtualCamera->setProperty("outputLib",
                                           config.value("VirtualCamera.outputLib",
                                                        this->m_virtualCamera->property("outputLib")));
    }

    config.endGroup();

    config.beginGroup("OutputConfigs");
    this->setEnableVirtualCamera(config.value("enableVirtualCamera", false).toBool());
    config.endGroup();

    config.beginGroup("GeneralConfigs");
    QSize windowSize = config.value("windowSize", QSize(1024, 600)).toSize();
    this->m_windowWidth = windowSize.width();
    this->m_windowHeight = windowSize.height();

#ifdef Q_OS_WIN32
    if (this->m_virtualCamera) {
        QString driverPath = config.value("virtualCameraDriverPath",
                                          "VirtualCameraSource.dll").toString();
        this->m_virtualCamera->setProperty("driverPath",
                                           this->convertToAbsolute(driverPath));
    }
#endif

    config.endGroup();
}

void MediaTools::saveVirtualCameraConvertLib(const QString &convertLib)
{
    QSettings config;
    config.beginGroup("Libraries");
    config.setValue("VirtualCamera.convertLib", convertLib);
    config.endGroup();
}

void MediaTools::saveVirtualCameraOutputLib(const QString &outputLib)
{
    QSettings config;
    config.beginGroup("Libraries");
    config.setValue("VirtualCamera.outputLib", outputLib);
    config.endGroup();
}

void MediaTools::saveConfigs()
{
    QSettings config;

    config.beginGroup("OutputConfigs");
    config.setValue("enableVirtualCamera", this->enableVirtualCamera());
    config.endGroup();

    config.beginGroup("GeneralConfigs");
    config.setValue("windowSize", QSize(this->m_windowWidth,
                                        this->m_windowHeight));

#ifdef Q_OS_WIN32
    if (this->m_virtualCamera) {
        QString driverPath = this->m_virtualCamera->property("driverPath").toString();
        static const QDir applicationDir(QCoreApplication::applicationDirPath());
        config.setValue("virtualCameraDriverPath", applicationDir.relativeFilePath(driverPath));
    }
#endif

    config.endGroup();

    config.beginGroup("Libraries");

    if (this->m_virtualCamera) {
        config.setValue("VirtualCamera.convertLib", this->m_virtualCamera->property("convertLib"));
        config.setValue("VirtualCamera.outputLib", this->m_virtualCamera->property("outputLib"));
    }

    config.endGroup();
}

void MediaTools::show()
{
    // @uri Webcamoid
    qmlRegisterType<VideoDisplay>("Webcamoid", 1, 0, "VideoDisplay");
    this->m_engine->rootContext()->setContextProperty("Webcamoid", this);
    this->m_engine->load(QUrl(QStringLiteral("qrc:/Webcamoid/share/qml/main.qml")));

    for (const QObject *obj: this->m_engine->rootObjects()) {
        // First, find where to enbed the UI.
        auto videoDisplay = obj->findChild<VideoDisplay *>("videoDisplay");

        if (!videoDisplay)
            continue;

        AkElement::link(this->m_videoEffects.data(), videoDisplay, Qt::DirectConnection);
        break;
    }

    emit this->interfaceLoaded();
}

void MediaTools::updateVCamCaps(const AkCaps &videoCaps)
{
    if (!this->m_virtualCamera)
        return;

    QMetaObject::invokeMethod(this->m_virtualCamera.data(),
                              "clearStreams");
    QMetaObject::invokeMethod(this->m_virtualCamera.data(),
                              "addStream",
                              Q_ARG(int, 0),
                              Q_ARG(AkCaps, videoCaps));
}

void MediaTools::updateVCamState()
{
    if (!this->m_virtualCamera)
        return;

    if (this->m_enableVirtualCamera) {
        if (this->m_mediaSource->state() == AkElement::ElementStatePlaying) {
            auto vcamStream = this->m_virtualCamera->property("media").toString();

            // Prevents self blocking by pausing the virtual camera.
            auto state = this->m_mediaSource->stream() == vcamStream?
                             AkElement::ElementStatePaused:
                             AkElement::ElementStatePlaying;
            this->m_virtualCamera->setState(state);
        }
    } else
        this->m_virtualCamera->setState(AkElement::ElementStateNull);
}
