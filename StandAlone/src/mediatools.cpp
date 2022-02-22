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
#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QThread>
#include <ak.h>
#include <akcaps.h>
#include <akaudiocaps.h>
#include <akvideocaps.h>
#include <akpluginmanager.h>

#include "mediatools.h"
#include "audiolayer.h"
#include "clioptions.h"
#include "downloadmanager.h"
#include "iconsprovider.h"
#include "pluginconfigs.h"
#include "recording.h"
#include "updates.h"
#include "videodisplay.h"
#include "videoeffects.h"
#include "videolayer.h"

#define COMMONS_PROJECT_URL "https://webcamoid.github.io/"
#define COMMONS_PROJECT_LICENSE_URL "https://raw.githubusercontent.com/webcamoid/webcamoid/master/COPYING"
#define COMMONS_PROJECT_DOWNLOADS_URL "https://webcamoid.github.io/#downloads"
#define COMMONS_PROJECT_ISSUES_URL "https://github.com/webcamoid/webcamoid/issues"
#define COMMONS_PROJECT_COMMIT_URL "https://github.com/webcamoid/webcamoid/commit"
#define COMMONS_PROJECT_DONATIONS_URL "https://github.com/webcamoid/webcamoid#donations"
#define COMMONS_COPYRIGHT_NOTICE "Copyright (C) 2011-2022  Gonzalo Exequiel Pedone"

struct LogingOptions
{
    QMutex mutex;
    QString logFile;
};

Q_GLOBAL_STATIC(LogingOptions, globalLogingOptions)

class MediaToolsPrivate
{
    public:
        QQmlApplicationEngine *m_engine {nullptr};
        AudioLayerPtr m_audioLayer;
        PluginConfigsPtr m_pluginConfigs;
        RecordingPtr m_recording;
        UpdatesPtr m_updates;
        VideoEffectsPtr m_videoEffects;
        VideoLayerPtr m_videoLayer;
        DownloadManagerPtr m_downloadManager;
        int m_windowWidth {0};
        int m_windowHeight {0};

        void loadLinks();
        void saveLinks(const AkPluginLinks &links);
};

MediaTools::MediaTools(const CliOptions &cliOptions, QObject *parent):
    QObject(parent)
{
    this->d = new MediaToolsPrivate;
    Ak::registerTypes();
    this->d->loadLinks();

    // Initialize environment.
    this->d->m_engine = new QQmlApplicationEngine();

    // Set theme.
    this->d->m_engine->addImportPath(":/Webcamoid/share/themes");
    QQuickStyle::setStyle("WebcamoidTheme");

    this->d->m_engine->addImageProvider(QLatin1String("icons"),
                                        new IconsProvider);
    Ak::setQmlEngine(this->d->m_engine);
    this->d->m_pluginConfigs =
            PluginConfigsPtr(new PluginConfigs(cliOptions,
                                               this->d->m_engine));
    this->d->m_videoLayer =
            VideoLayerPtr(new VideoLayer(this->d->m_engine));
    this->d->m_audioLayer = AudioLayerPtr(new AudioLayer(this->d->m_engine));
    this->d->m_videoEffects =
            VideoEffectsPtr(new VideoEffects(this->d->m_engine));
    this->d->m_recording = RecordingPtr(new Recording(this->d->m_engine));
    this->d->m_updates = UpdatesPtr(new Updates(this->d->m_engine));
    this->d->m_downloadManager =
            DownloadManagerPtr(new DownloadManager(this->d->m_engine));
    this->d->m_updates->watch("Webcamoid",
                              COMMONS_VERSION,
                              "https://api.github.com/repos/webcamoid/webcamoid/releases/latest");
    this->d->m_updates->watch("VirtualCamera",
                              this->d->m_videoLayer->currentVCamVersion(),
                              this->d->m_videoLayer->vcamUpdateUrl());
    QObject::connect(this->d->m_updates.data(),
                     &Updates::newVersionAvailable,
                     this,
                     [this] (const QString &component,
                             const QString &latestVersion) {
        if (component == "VirtualCamera")
            this->d->m_videoLayer->setLatestVCamVersion(latestVersion);
    });

    AkElement::link(this->d->m_videoLayer.data(),
                    this->d->m_videoEffects.data(),
                    Qt::DirectConnection);
    AkElement::link(this->d->m_videoLayer.data(),
                    this->d->m_audioLayer.data(),
                    Qt::DirectConnection);
    AkElement::link(this->d->m_videoEffects.data(),
                    this->d->m_recording.data(),
                    Qt::DirectConnection);
    AkElement::link(this->d->m_videoEffects.data(),
                    this->d->m_videoLayer.data(),
                    Qt::DirectConnection);
    AkElement::link(this->d->m_audioLayer.data(),
                    this->d->m_recording.data(),
                    Qt::DirectConnection);
    QObject::connect(this->d->m_videoLayer.data(),
                     &VideoLayer::stateChanged,
                     this->d->m_videoEffects.data(),
                     &VideoEffects::setState);
    QObject::connect(this->d->m_videoLayer.data(),
                     &VideoLayer::stateChanged,
                     this->d->m_audioLayer.data(),
                     &AudioLayer::setOutputState);
    QObject::connect(this->d->m_recording.data(),
                     &Recording::stateChanged,
                     this->d->m_audioLayer.data(),
                     &AudioLayer::setInputState);
    QObject::connect(this->d->m_videoLayer.data(),
                     &VideoLayer::startVCamDownload,
                     this,
                     [this] (const QString &title,
                             const QString &fromUrl,
                             const QString &toFile) {
        this->d->m_downloadManager->clear();
        this->d->m_downloadManager->enqueue(title, fromUrl, toFile);
    });
    QObject::connect(this->d->m_videoLayer.data(),
                     &VideoLayer::inputAudioCapsChanged,
                     this->d->m_audioLayer.data(),
                     [this] (const AkAudioCaps &audioCaps)
                     {
                        auto stream = this->d->m_videoLayer->videoInput();

                        if (stream.isEmpty())
                            this->d->m_audioLayer->resetInput();
                        else
                            this->d->m_audioLayer->setInput(stream,
                                                            this->d->m_videoLayer->description(stream),
                                                            audioCaps);
                     });
    QObject::connect(this->d->m_videoLayer.data(),
                     &VideoLayer::videoInputChanged,
                     this->d->m_audioLayer.data(),
                     [this] (const QString &stream)
                     {
                        if (stream.isEmpty())
                            this->d->m_audioLayer->resetInput();
                        else
                            this->d->m_audioLayer->setInput(stream,
                                                            this->d->m_videoLayer->description(stream),
                                                            this->d->m_videoLayer->inputAudioCaps());
                     });
    QObject::connect(akPluginManager,
                     &AkPluginManager::pluginsChanged,
                     this->d->m_videoEffects.data(),
                     &VideoEffects::updateAvailableEffects);
    QObject::connect(this->d->m_audioLayer.data(),
                     &AudioLayer::outputCapsChanged,
                     this->d->m_recording.data(),
                     &Recording::setAudioCaps);
    QObject::connect(this->d->m_videoLayer.data(),
                     &VideoLayer::inputVideoCapsChanged,
                     this->d->m_recording.data(),
                     &Recording::setVideoCaps);
    QObject::connect(qApp,
                     &QCoreApplication::aboutToQuit,
                     this->d->m_videoLayer.data(),
                     [this] () {
                        this->d->m_videoLayer->setState(AkElement::ElementStateNull);
                     });
    QObject::connect(akPluginManager,
                     &AkPluginManager::linksChanged,
                     this,
                     [this] (const AkPluginLinks &links) {
                        this->d->saveLinks(links);
                     });
    QObject::connect(this->d->m_downloadManager.data(),
                     &DownloadManager::finished,
                     this,
                     [this] (const QString &url) {
        auto filePath = this->d->m_downloadManager->downloadFile(url);
        auto status = this->d->m_downloadManager->downloadStatus(url);
        auto error = this->d->m_downloadManager->downloadErrorString(url);
        this->d->m_videoLayer->checkVCamDownloadReady(url,
                                                      filePath,
                                                      status,
                                                      error);
    });

    this->loadConfigs();
    this->d->m_recording->setVideoCaps(this->d->m_videoLayer->inputVideoCaps());
    this->d->m_recording->setAudioCaps(this->d->m_audioLayer->outputCaps());
    auto stream = this->d->m_videoLayer->videoInput();

    if (stream.isEmpty())
        this->d->m_audioLayer->resetInput();
    else
        this->d->m_audioLayer->setInput(stream,
                                        this->d->m_videoLayer->description(stream),
                                        this->d->m_videoLayer->inputAudioCaps());

    this->d->m_videoLayer->setLatestVCamVersion(this->d->m_updates->latestVersion("VirtualCamera"));
    this->d->m_updates->start();
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

QString MediaTools::applicationName() const
{
    return QCoreApplication::applicationName();
}

QString MediaTools::applicationVersion() const
{
    return QCoreApplication::applicationVersion();
}

bool MediaTools::isDailyBuild() const
{
#ifdef DAILY_BUILD
    return true;
#else
    return false;
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

QString MediaTools::projectGitCommit() const
{
#ifdef GIT_COMMIT_HASH
    return {GIT_COMMIT_HASH};
#else
    return {};
#endif
}

QString MediaTools::projectGitShortCommit() const
{
#ifdef GIT_COMMIT_HASH
    return QString(GIT_COMMIT_HASH).left(7);
#else
    return {};
#endif
}

QString MediaTools::projectGitCommitUrl() const
{
#ifdef GIT_COMMIT_HASH
    return {COMMONS_PROJECT_COMMIT_URL "/" GIT_COMMIT_HASH};
#else
    return {};
#endif
}

QString MediaTools::projectDonationsUrl() const
{
    return {COMMONS_PROJECT_DONATIONS_URL};
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

    auto re = QRegularExpression::fromWildcard(pattern,
                                               Qt::CaseInsensitive,
                                               QRegularExpression::UnanchoredWildcardConversion);

    for (auto &str: strings)
        if (re.match(str).hasMatch())
            return true;

    return false;
}

QString MediaTools::currentTime() const
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss");
}

QString MediaTools::currentTime(const QString &format) const
{
    return QDateTime::currentDateTime().toString(format);
}

QStringList MediaTools::standardLocations(const QString &type) const
{
    static const QMap<QString, QStandardPaths::StandardLocation> stdPaths {
        {"movies"  , QStandardPaths::MoviesLocation  },
        {"pictures", QStandardPaths::PicturesLocation},
    };

    if (stdPaths.contains(type))
        return QStandardPaths::standardLocations(stdPaths[type]);

    return {};
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

QString MediaTools::convertToAbsolute(const QString &path)
{
    if (!QDir::isRelativePath(path))
        return QDir::cleanPath(path);

    static const QDir applicationDir(QCoreApplication::applicationDirPath());
    auto absPath = applicationDir.absoluteFilePath(path);

    return QDir::cleanPath(absPath).replace('/', QDir::separator());
}

void MediaTools::setLogFile(const QString &logFile)
{
    globalLogingOptions->mutex.lock();
    globalLogingOptions->logFile = logFile;
    globalLogingOptions->mutex.unlock();
}

void MediaTools::messageHandler(QtMsgType type,
                                const QMessageLogContext &context,
                                const QString &msg)
{
    static const QMap<QtMsgType, QString> typeToStr {
        {QtDebugMsg   , "debug"   },
        {QtWarningMsg , "warning" },
        {QtCriticalMsg, "critical"},
        {QtFatalMsg   , "fatal"   },
        {QtInfoMsg    , "info"    },
    };
    QString log;
    QTextStream ss(&log);
    ss << "["
       << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
       << ", "
       << QCoreApplication::applicationName()
       << ", "
       << QThread::currentThreadId()
       << ", "
       << QFileInfo(context.file).fileName()
       << " ("
       << context.line
       << ")] "
       << typeToStr[type]
       << ": "
       << msg
       << Qt::endl;

    globalLogingOptions->mutex.lock();

    if (globalLogingOptions->logFile.isEmpty()) {
        fprintf(stderr, "%s", log.toStdString().c_str());
    } else {
        QFile file(globalLogingOptions->logFile);

        if (file.open(QIODevice::WriteOnly
                      | QIODevice::Text
                      | QIODevice::Append)) {
            file.write(log.toUtf8());
        }
    }

    globalLogingOptions->mutex.unlock();
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

void MediaTools::resetWindowWidth()
{
    this->setWindowWidth(0);
}

void MediaTools::resetWindowHeight()
{
    this->setWindowHeight(0);
}

void MediaTools::loadConfigs()
{
    QSettings config;

    config.beginGroup("GeneralConfigs");
    auto windowSize = config.value("windowSize", QSize(1024, 600)).toSize();
    this->d->m_windowWidth = windowSize.width();
    this->d->m_windowHeight = windowSize.height();
    config.endGroup();
}

void MediaTools::saveConfigs()
{
    QSettings config;
    config.beginGroup("GeneralConfigs");
    config.setValue("windowSize", QSize(this->d->m_windowWidth,
                                        this->d->m_windowHeight));
    config.endGroup();
}

void MediaTools::show()
{
    // @uri Webcamoid
    qmlRegisterType<VideoDisplay>("Webcamoid", 1, 0, "VideoDisplay");
    this->d->m_engine->rootContext()->setContextProperty("mediaTools", this);
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

void MediaTools::restartApp()
{
    qApp->quit();
    auto args = qApp->arguments();

    if (args.size() > 1)
        QProcess::startDetached(args.first(), args.mid(1));
    else
        QProcess::startDetached(args.first(), {});
}

void MediaToolsPrivate::loadLinks()
{
    QSettings config;

    config.beginGroup("PluginsLinks");
    int nlinks = config.beginReadArray("links");
    AkPluginLinks links;

    for (int i = 0; i < nlinks; i++) {
        config.setArrayIndex(i);
        auto from = config.value("from").toString();
        auto to = config.value("to").toString();

        if (!from.isEmpty() && !to.isEmpty())
            links[from] = to;
    }

    akPluginManager->setLinks(links);
    config.endArray();
    config.endGroup();
}

void MediaToolsPrivate::saveLinks(const AkPluginLinks &links)
{
    QSettings config;

    config.beginGroup("PluginsLinks");
    config.beginWriteArray("links");
    int i = 0;

    for (auto it = links.begin(); it != links.end(); it++) {
        config.setArrayIndex(i);
        config.setValue("from", it.key());
        config.setValue("to", it.value());
        i++;
    }

    config.endArray();
    config.endGroup();
}

#include "moc_mediatools.cpp"
