/* Webcamoid, camera capture application.
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

#include <iostream>
#include <QApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QRegularExpression>
#include <QSettings>
#include <QSharedMemory>
#include <QStandardPaths>
#include <QThread>
#include <QtConcurrent>
#include <ak.h>
#include <akcaps.h>
#include <akaudiocaps.h>
#include <akvideocaps.h>
#include <akpluginmanager.h>

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#include <QJniObject>
#include <QtCore/private/qandroidextras_p.h>
#include <android/log.h>

#define FLAG_GRANT_READ_URI_PERMISSION 0x1
#endif

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
#define COMMONS_PROJECT_DONATIONS_URL "https://webcamoid.github.io/donations"
#define COMMONS_PROJECT_DOCUMENTATION_URL "https://github.com/webcamoid/webcamoid/wiki"
#define COMMONS_COPYRIGHT_NOTICE "Copyright (C) 2011-2025  Gonzalo Exequiel Pedone"

#define JNAMESPACE "org/webcamoid/webcamoidutils"
#define JCLASS(jclass) JNAMESPACE "/" #jclass
#define JLCLASS(jclass) "L" JNAMESPACE "/" jclass ";"
#define JCLASS_SUBTYPE(jclass, subtype) JCLASS(jclass) "$" #subtype
#define JLCLASS_SUBTYPE(jclass, subtype) "L" JCLASS_SUBTYPE(jclass, subtype) ";"

#define MAX_STRING_SIZE 8192

struct MediaToolsLogger
{
    MediaTools *m_mediaTools {nullptr};
    char m_fileName [MAX_STRING_SIZE];
    QFile *m_logFile {nullptr};

    MediaToolsLogger();
    void setMediaTools(MediaTools *mediaTools);
    void setFileName(const QString &fileName);
    bool writeLine(const QString &msg);
    void close();

    QString readLog(quint64 lineStart=0) const;
};

static MediaToolsLogger globalMediaToolsLogger;

#if defined(Q_OS_ANDROID) && defined(ENABLE_ANDROID_ADS)
struct AdUnit
{
    MediaTools::AdType type;
    jint jniType;
    QString id;
};
#endif

class MediaToolsPrivate
{
    public:
        MediaTools *self;
        bool m_singleInstance {true};

#if QT_CONFIG(sharedmemory)
        QSharedMemory m_singleInstanceSM {
            QString("%1.%2.%3").arg(QApplication::applicationName(),
                                    QApplication::organizationName(),
                                    QApplication::organizationDomain())
        };
#endif
        QQmlApplicationEngine *m_engine {nullptr};
        AudioLayerPtr m_audioLayer;
        PluginConfigsPtr m_pluginConfigs;
        RecordingPtr m_recording;
        UpdatesPtr m_updates;
        VideoEffectsPtr m_videoEffects;
        VideoLayerPtr m_videoLayer;
        DownloadManagerPtr m_downloadManager;
        QMutex m_logMutex;
        QString m_documentsDirectory;
        int m_adBannerWidth {0};
        int m_adBannerHeight {0};
        QTime m_lastTimeAdShow;
        bool m_hideControlsOnPointerOut {false};

        // Show interstitial ads every 1 minute
        int m_adTimeDiff {1 * 60};

#ifdef Q_OS_ANDROID
        QMutex m_mutex;
        QJniObject m_callbacks;
        QJniObject m_adManager;
        bool m_scanResultReady {false};
        QString m_scanResult;
#endif

        int m_windowWidth {0};
        int m_windowHeight {0};

        explicit MediaToolsPrivate(MediaTools *self);
        void registerTypes() const;
        void registerNatives();
        bool isSecondInstance();
        void hasNewInstance();
        void loadLinks();
        void saveLinks(const AkPluginLinks &links);
        bool setupAds();

#ifdef Q_OS_ANDROID
    #ifdef ENABLE_ANDROID_ADS
        QVector<AdUnit> m_adUnits;
    #endif

        static void adBannerSizeChanged(JNIEnv *env,
                                        jobject obj,
                                        jlong userPtr,
                                        jint width,
                                        jint height);
        static void scanCompleted(JNIEnv *env,
                                  jobject obj,
                                  jlong userPtr,
                                  jobject path,
                                  jobject uri);
        QJniObject getUriForFile(const QString &filePath);
        QString copyUrlToLocal(const QString &urlOrFile) const;
#endif
};

MediaTools::MediaTools(QObject *parent):
    QObject(parent)
{
    this->d = new MediaToolsPrivate(this);
}

MediaTools::~MediaTools()
{
    this->saveConfigs();

    if (this->d->m_engine)
        delete this->d->m_engine;

    globalMediaToolsLogger.close();

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

QString MediaTools::projectDocumentationUrl() const
{
    return {COMMONS_PROJECT_DOCUMENTATION_URL};
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
    static const struct {
        const char *type;
        QStandardPaths::StandardLocation location;
    } mediaToolsStdPaths[] = {
        {"movies"   , QStandardPaths::MoviesLocation   },
        {"pictures" , QStandardPaths::PicturesLocation },
        {"documents", QStandardPaths::DocumentsLocation},
        {"download" , QStandardPaths::DownloadLocation },
        {"temp"     , QStandardPaths::TempLocation     },
        {nullptr    , QStandardPaths::TempLocation     }
    };

    for (auto path = mediaToolsStdPaths; path->type; ++path)
        if (path->type == type)
            return QStandardPaths::standardLocations(path->location);

    return {};
}

QString MediaTools::readFile(const QString &fileName)
{
    QString data;
    QFile file(fileName);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        data = file.readAll();
        file.close();
    }

    return data;
}

bool MediaTools::sendFile(const QString &fileName, const QString &subject)
{
#ifdef Q_OS_ANDROID
    auto uri = this->d->getUriForFile(fileName);

    if (!uri.isValid()) {
        qWarning() << "Can't create a valid file URI.";

        return false;
    }

    auto activity =
        QJniObject(qApp->nativeInterface<QNativeInterface::QAndroidApplication>()->context());

    if (!activity.isValid()){
        qWarning() << "Can't get the application activity.";

        return false;
    }

    // Create SEND action intent
    QJniObject intent("android/content/Intent", "()V");
    auto action = QJniObject::fromString("android.intent.action.SEND");
    intent.callObjectMethod("setAction",
                            "(Ljava/lang/String;)Landroid/content/Intent;",
                            action.object<jstring>());

    // Attach the file as an extra stream
    intent.callObjectMethod("putExtra",
                            "(Ljava/lang/String;Landroid/os/Parcelable;)Landroid/content/Intent;",
                            QJniObject::fromString("android.intent.extra.STREAM").object<jstring>(),
                            uri.object());

    // Set the MIME type (important for filtering compatible apps)
    QMimeDatabase mimeDb;
    auto mimeType = mimeDb.mimeTypeForFile(fileName);
    auto mimeString = mimeType.name();

    if (mimeString.isEmpty())
        mimeString = "*/*";

    intent.callObjectMethod("setType",
                            "(Ljava/lang/String;)Landroid/content/Intent;",
                            QJniObject::fromString(mimeString).object<jstring>());

    // Grant read permission
    intent.callObjectMethod("addFlags",
                            "(I)Landroid/content/Intent;",
                            FLAG_GRANT_READ_URI_PERMISSION);

    // Launch the app selector for sharing
    auto chooserTitle = QJniObject::fromString(subject);
    auto chooser =
            QJniObject::callStaticObjectMethod("android/content/Intent",
                                               "createChooser",
                                               "(Landroid/content/Intent;Ljava/lang/CharSequence;)Landroid/content/Intent;",
                                               intent.object(),
                                               chooserTitle.object<jstring>());

    if (!chooser.isValid())
        return false;

    return activity.callMethod<jboolean>("startActivity",
                                         "(Landroid/content/Intent;)V",
                                         chooser.object());
#else
    Q_UNUSED(fileName)

    return false;
#endif
}

QString MediaTools::urlToLocalFile(const QString &urlOrFile) const
{
#ifdef Q_OS_ANDROID
    if (urlOrFile.startsWith("content://"))
        return this->d->copyUrlToLocal(urlOrFile);
#endif

    auto filePath = QUrl(urlOrFile).toLocalFile();

    return filePath.isEmpty()? urlOrFile: filePath;
}

QString MediaTools::urlToLocalFolder(const QString &urlOrFolder) const
{
    auto filePath = QUrl(urlOrFolder).toLocalFile();

    return filePath.isEmpty()? urlOrFolder: filePath;
}

QString MediaTools::organizeFile(const QString &filePath) const
{
    // Validate input file
    QFile sourceFile(filePath);

    if (!sourceFile.exists()) {
        qWarning() << "Source file does not exist:" << filePath;

        return {};
    }

    // Get file info
    QFileInfo fileInfo(filePath);
    auto fileName = fileInfo.fileName();

    // Determine MIME type
    QMimeDatabase mimeDatabase;
    auto mimeType = mimeDatabase.mimeTypeForFile(filePath);

    // Determine target directory based on MIME type
    QString targetDirPath;

    if (mimeType.name().startsWith("video/"))
        targetDirPath =
                QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    else if (mimeType.name().startsWith("image/"))
        targetDirPath =
                QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    else
        targetDirPath =
                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    // Ensure target directory exists
    QDir targetDir(targetDirPath);

    if (!targetDir.exists())
        if (!targetDir.mkpath(".")) {
            qWarning() << "Failed to create target directory:" << targetDirPath;

            return {};
        }

    // Generate unique target file path (handle duplicates)
    auto targetFilePath = targetDir.filePath(fileName);
    auto baseName = fileInfo.completeBaseName();
    auto suffix = fileInfo.suffix();
    int counter = 1;

    while (QFile::exists(targetFilePath)) {
        auto newFileName = QString("%1 (%2).%3").arg(baseName)
                                                .arg(counter)
                                                .arg(suffix);
        targetFilePath = targetDir.filePath(newFileName);
        counter++;
    }

    // Move the file
    if (!sourceFile.rename(targetFilePath)) {
        qWarning() << "Failed to move file from"
                   << filePath
                   << "to"
                   << targetFilePath;

        return {};
    }

    qDebug() << "Successfully moved"
             << filePath
             << "to"
             << targetFilePath
             << "(MIME type:" << mimeType.name() << ")";

    return targetFilePath;
}

QString MediaTools::convertToAbsolute(const QString &path)
{
    if (!QDir::isRelativePath(path))
        return QDir::cleanPath(path);

    static const QDir applicationDir(QCoreApplication::applicationDirPath());
    auto absPath = applicationDir.absoluteFilePath(path);

    return QDir::cleanPath(absPath).replace('/', QDir::separator());
}

void MediaTools::messageHandler(QtMsgType type,
                                const QMessageLogContext &context,
                                const QString &msg)
{
    // Map the message types

    static const struct
    {
        QtMsgType type;
        const char *str;
#ifdef Q_OS_ANDROID
        int atype;
#endif
    } mediaToolsLogTypeMap [] = {
        {QtWarningMsg,
         "warning"
#ifdef Q_OS_ANDROID
         , ANDROID_LOG_WARN
#endif
        },
        {QtCriticalMsg,
         "critical"
#ifdef Q_OS_ANDROID
         , ANDROID_LOG_ERROR
#endif
        },
        {QtFatalMsg,
         "fatal"
#ifdef Q_OS_ANDROID
         , ANDROID_LOG_FATAL
#endif
        },
        {QtInfoMsg,
         "info"
#ifdef Q_OS_ANDROID
         , ANDROID_LOG_INFO
#endif
        },
        {QtDebugMsg,
         "debug"
#ifdef Q_OS_ANDROID
         , ANDROID_LOG_DEBUG
#endif
        },
    };

    const char *msgTypeStr = "debug";

#ifdef Q_OS_ANDROID
    int aMsgType = ANDROID_LOG_DEBUG;
#endif

    for (auto it = mediaToolsLogTypeMap; it->type != QtDebugMsg; ++it)
        if (it->type == type) {
            msgTypeStr = it->str;

#ifdef Q_OS_ANDROID
            aMsgType = it->atype;
#endif

            break;
        }

    // Calculate date time

    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    auto nowC = std::chrono::system_clock::to_time_t(now);
    auto localTm = *std::localtime(&nowC);

    char dateTime[1024];
    std::snprintf(dateTime,
                  1024,
                  "%04d-%02d-%02d %02d:%02d:%02d.%03lld",
                  1900 + localTm.tm_year,
                  1 + localTm.tm_mon,
                  localTm.tm_mday,
                  localTm.tm_hour,
                  localTm.tm_min,
                  localTm.tm_sec,
                  static_cast<long long>(ms.count()));

    // Only read the file name, not the full path.
    const char *fileName = "";

    if (context.file) {
        auto slash = strrchr(context.file, '/');

#ifdef Q_OS_WIN32
        auto bslash = strrchr(context.file, '\\');
        slash = std::max(slash, bslash);
#endif

        fileName = slash? slash + 1: context.file;
    }

    // Get thread ID

    auto threadId = QThread::currentThreadId();

    // Log formatting

    size_t logSize = msg.size() + 1024;
    auto log = new char [logSize];
    int len = snprintf(log,
                       logSize,
                       "[%s, %s, %p, %s (%d)] %s: %s",
                       dateTime,
                       COMMONS_APPNAME,
                       threadId,
                       fileName,
                       context.line,
                       msgTypeStr,
                       qUtf8Printable(msg));
    auto logStr = QString::fromUtf8(log, len);

    if (globalMediaToolsLogger.m_mediaTools)
        globalMediaToolsLogger.m_mediaTools->d->m_logMutex.lock();

    // Print the log to the terminal

#ifdef Q_OS_ANDROID
    __android_log_print(aMsgType,
                        COMMONS_APPNAME,
                        "[%p, %s (%d)]: %s",
                        threadId,
                        fileName,
                        context.line,
                        qUtf8Printable(msg));
#else
    auto &stream = type == QtInfoMsg? std::cout: std::cerr;
    stream << log << std::endl;
#endif

    delete [] log;

    // Write the log to a file

    globalMediaToolsLogger.writeLine(logStr);

    // Emit the last message

    if (globalMediaToolsLogger.m_mediaTools) {
        globalMediaToolsLogger.m_mediaTools->d->m_logMutex.unlock();

        emit globalMediaToolsLogger.m_mediaTools->logUpdated(msgTypeStr, logStr);
    }
}

QString MediaTools::readLog(quint64 lineStart) const
{
    return globalMediaToolsLogger.readLog(lineStart);
}

QString MediaTools::documentsDirectory() const
{
    return this->d->m_documentsDirectory;
}

int MediaTools::adBannerWidth() const
{
    return this->d->m_adBannerWidth;
}

int MediaTools::adBannerHeight() const
{
    return this->d->m_adBannerHeight;
}

bool MediaTools::singleInstanceAllowed() const
{
#if QT_CONFIG(sharedmemory)
    return true;
#else
    return false;
#endif
}

bool MediaTools::singleInstance() const
{
    return this->d->m_singleInstance;
}

bool MediaTools::hideControlsOnPointerOut() const
{
    return this->d->m_hideControlsOnPointerOut;
}

bool MediaTools::init(const CliOptions &cliOptions)
{
    if (!globalMediaToolsLogger.m_mediaTools) {
        auto logPath =
                QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        auto logFile = QDir(logPath).absoluteFilePath("log.txt");
        globalMediaToolsLogger.setMediaTools(this);
        globalMediaToolsLogger.setFileName(logFile);
    }

    if (this->d->isSecondInstance()
        && this->d->m_singleInstance
        && !cliOptions.isSet(cliOptions.newInstance())) {
        qInfo() << QString("An instance of %1 is already running").arg(QApplication::applicationName());

        return false;
    }

    this->d->registerTypes();
    VideoDisplay::registerTypes();

#ifdef Q_OS_ANDROID
    this->d->registerNatives();

#ifdef ENABLE_ANDROID_ADS
    #define ADTYPE(type) QJniObject::getStaticField<jint>(JCLASS(AdManager), #type)

    this->d->m_adUnits = {
        {MediaTools::AdType_Banner              , ADTYPE(ADTYPE_BANNER)               , ANDROID_AD_UNIT_ID_BANNER                        },
        {MediaTools::AdType_AdaptiveBanner      , ADTYPE(ADTYPE_ADAPTIVE_BANNER)      , ANDROID_AD_UNIT_ID_ADAPTIVE_BANNER               },
        {MediaTools::AdType_Appopen             , ADTYPE(ADTYPE_APPOPEN)              , ANDROID_AD_UNIT_ID_APP_OPEN                      },
        {MediaTools::AdType_Interstitial        , ADTYPE(ADTYPE_INTERSTITIAL)         , ANDROID_AD_UNIT_ID_ADAPTIVE_INTERSTITIAL         },
        {MediaTools::AdType_Rewarded            , ADTYPE(ADTYPE_REWARDED)             , ANDROID_AD_UNIT_ID_ADAPTIVE_REWARDED             },
        {MediaTools::AdType_RewardedInterstitial, ADTYPE(ADTYPE_REWARDED_INTERSTITIAL), ANDROID_AD_UNIT_ID_ADAPTIVE_REWARDED_INTERSTITIAL}
    };

    Ak::registerJniLogFunc(JCLASS(AdManager));
#endif

    jlong userPtr = intptr_t(this);
    this->d->m_callbacks =
            QJniObject(JCLASS(WebcamoidUtils),
                       "(J)V",
                       userPtr);
#endif

    auto documentsPath =
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    this->d->m_documentsDirectory =
            documentsPath.isEmpty()?
                "":
                QDir(documentsPath).filePath(qApp->applicationName());

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
            PluginConfigsPtr(new PluginConfigs(cliOptions, this->d->m_engine));
    this->d->m_videoLayer =
            VideoLayerPtr(new VideoLayer(this->d->m_engine));
    this->d->m_audioLayer = AudioLayerPtr(new AudioLayer(this->d->m_engine));
    this->d->m_videoEffects =
            VideoEffectsPtr(new VideoEffects(this->d->m_engine));
    this->d->m_recording = RecordingPtr(new Recording(this->d->m_engine));
    this->d->m_updates = UpdatesPtr(new Updates(this->d->m_engine));
    this->d->m_downloadManager =
            DownloadManagerPtr(new DownloadManager(this->d->m_engine));

#ifndef DISABLE_UPDATES_CHECK
    this->d->m_updates->watch("Webcamoid",
                              COMMONS_VERSION,
                              "https://api.github.com/repos/webcamoid/webcamoid/releases/latest");
    this->d->m_updates->watch("VirtualCamera",
                              this->d->m_videoLayer->currentVCamVersion(),
                              this->d->m_videoLayer->vcamUpdateUrl());
#endif

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
    auto stream = this->d->m_videoLayer->videoInput();

    if (stream.isEmpty())
        this->d->m_audioLayer->resetInput();
    else
        this->d->m_audioLayer->setInput(stream,
                                        this->d->m_videoLayer->description(stream),
                                        this->d->m_videoLayer->inputAudioCaps());

    this->d->m_videoLayer->setLatestVCamVersion(this->d->m_updates->latestVersion("VirtualCamera"));
    this->d->m_updates->start();

    return true;
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

void MediaTools::setDocumentsDirectory(const QString &documentsDirectory)
{
    if (this->d->m_documentsDirectory == documentsDirectory)
        return;

    this->d->m_documentsDirectory = documentsDirectory;
    emit this->documentsDirectoryChanged(this->d->m_documentsDirectory);
}

void MediaTools::setSingleInstance(bool singleInstance)
{
    if (this->d->m_singleInstance == singleInstance)
        return;

    this->d->m_singleInstance = singleInstance;

    QSettings config;
    config.beginGroup("GeneralConfigs");
    config.setValue("singleInstance",
                    this->d->m_singleInstance);
    config.endGroup();

    emit this->singleInstanceChanged(this->d->m_singleInstance);
}

void MediaTools::setHideControlsOnPointerOut(bool hideControlsOnPointerOut)
{
    if (this->d->m_hideControlsOnPointerOut == hideControlsOnPointerOut)
        return;

    this->d->m_hideControlsOnPointerOut = hideControlsOnPointerOut;

    QSettings config;
    config.beginGroup("GeneralConfigs");
    config.setValue("hideControlsOnPointerOut",
                    this->d->m_hideControlsOnPointerOut);
    config.endGroup();

    emit this->hideControlsOnPointerOutChanged(this->d->m_hideControlsOnPointerOut);
}

void MediaTools::resetWindowWidth()
{
    this->setWindowWidth(0);
}

void MediaTools::resetWindowHeight()
{
    this->setWindowHeight(0);
}

void MediaTools::resetDocumentsDirectory()
{
    auto documentsPath =
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString dir =
            documentsPath.isEmpty()?
                "":
                QDir(documentsPath).filePath(qApp->applicationName());
    this->setDocumentsDirectory(dir);
}

void MediaTools::resetSingleInstance()
{
    this->setSingleInstance(true);
}

void MediaTools::resetHideControlsOnPointerOut()
{
    this->setHideControlsOnPointerOut(false);
}

void MediaTools::loadConfigs()
{
    QSettings config;

    config.beginGroup("GeneralConfigs");
    auto windowSize = config.value("windowSize", QSize(1024, 600)).toSize();
    this->d->m_windowWidth = qMax(windowSize.width(), 640);
    this->d->m_windowHeight = qMax(windowSize.height(), 480);
    this->d->m_hideControlsOnPointerOut =
            config.value("hideControlsOnPointerOut", false).toBool();
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
    this->d->setupAds();
}

bool MediaTools::showAd(AdType adType)
{
#if defined(Q_OS_ANDROID) && defined(ENABLE_ANDROID_ADS)
    auto msTimeDiff = this->d->m_lastTimeAdShow.msecsTo(QTime::currentTime()) / 1000;

    if (this->d->m_lastTimeAdShow.isValid()
        && msTimeDiff < this->d->m_adTimeDiff) {
        return false;
    }

    this->d->m_lastTimeAdShow = QTime::currentTime();

    auto result = QNativeInterface::QAndroidApplication::runOnAndroidMainThread([this, adType] () -> QVariant {
        bool result = false;

        if (this->d->m_adManager.isValid()) {
            auto it = std::find_if(this->d->m_adUnits.begin(),
                                   this->d->m_adUnits.end(),
                                   [adType] (const AdUnit &unit) {
                return unit.type == adType;
            });

            if (it != this->d->m_adUnits.end())
                result = this->d->m_adManager.callMethod<jboolean>("show",
                                                                   "(I)Z",
                                                                   it->jniType);
        }

        return result;
    });
    result.waitForFinished();

    return result.result().toBool();
#else
    Q_UNUSED(adType)

    return false;
#endif
}

void MediaTools::printLog()
{
    qInfo() << "Plugin file pattern: " << akPluginManager->pluginFilePattern();
    qInfo() << "Search paths:";

    for (auto &path: akPluginManager->internalSearchPaths())
        qInfo() << "    " << path;

    for (auto &path: akPluginManager->searchPaths())
        qInfo() << "    " << path;

    qInfo() << "Plugin links:";
    auto links = akPluginManager->links();

    for (auto &key: links.keys())
        qInfo() << "    " << key << "->" << links[key];
}

bool MediaTools::saveLog()
{
    auto len = strnlen(globalMediaToolsLogger.m_fileName, MAX_STRING_SIZE);

    if (len < 1)
        return false;

    auto log = QString::fromUtf8(globalMediaToolsLogger.m_fileName, len);

    if (!QFile::exists(log))
        return false;

#ifdef Q_OS_ANDROID
    return QDesktopServices::openUrl(QUrl::fromLocalFile(log));
#else
    if (!QDir().mkpath(this->d->m_documentsDirectory))
        return false;

    auto currentTime =
            QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss");
    auto fileName =
            tr("%1/log %2.txt")
                .arg(this->d->m_documentsDirectory, currentTime);

    if (QFile::exists(fileName))
        QFile::remove(fileName);

    return QFile::copy(log, fileName);
#endif
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

MediaToolsPrivate::MediaToolsPrivate(MediaTools *self):
    self(self)
{
    QSettings config;
    config.beginGroup("GeneralConfigs");
    this->m_singleInstance = config.value("singleInstance", true).toBool();
    config.endGroup();
}

void MediaToolsPrivate::registerTypes() const
{
    qRegisterMetaType<MediaTools::AdType>("AdType");
    qmlRegisterSingletonType<MediaTools>("Webcamoid", 1, 0, "MediaTools",
                                          [] (QQmlEngine *qmlEngine,
                                              QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new MediaTools();
    });
}

void MediaToolsPrivate::registerNatives()
{
#ifdef Q_OS_ANDROID
    static bool ready = false;

    if (ready)
        return;

    QJniEnvironment jenv;

    if (auto jclass = jenv.findClass(JCLASS(WebcamoidUtils))) {
        static const QVector<JNINativeMethod> mediaToolsMethods {
            {"scanCompleted", "(JLjava/lang/String;Landroid/net/Uri;)V", reinterpret_cast<void *>(MediaToolsPrivate::scanCompleted)},
        };

        jenv->RegisterNatives(jclass,
                              mediaToolsMethods.data(),
                              mediaToolsMethods.size());
    }

#ifdef ENABLE_ANDROID_ADS
    if (auto jclass = jenv.findClass(JCLASS(AdManager))) {
        static const QVector<JNINativeMethod> mediaToolsAdsMethods {
            {"adBannerSizeChanged", "(JII)V", reinterpret_cast<void *>(MediaToolsPrivate::adBannerSizeChanged)},
        };

        jenv->RegisterNatives(jclass,
                              mediaToolsAdsMethods.data(),
                              mediaToolsAdsMethods.size());
    }
#endif

    ready = true;
#endif
}

bool MediaToolsPrivate::isSecondInstance()
{
#if QT_CONFIG(sharedmemory)
    return !this->m_singleInstanceSM.create(1024)
           && this->m_singleInstanceSM.error() == QSharedMemory::AlreadyExists;
#else
    return false;
#endif
}

void MediaToolsPrivate::hasNewInstance()
{
    emit self->newInstanceOpened();
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

bool MediaToolsPrivate::setupAds()
{
    bool result = false;

#if defined(Q_OS_ANDROID) && defined(ENABLE_ANDROID_ADS)
    QJniObject adUnitIDMap("java/util/HashMap", "()V");

    for (auto &unit: this->m_adUnits) {
        auto key = QJniObject::callStaticObjectMethod("java/lang/Integer",
                                                      "valueOf",
                                                      "(I)Ljava/lang/Integer;",
                                                      unit.jniType);
        auto value = QJniObject::fromString(unit.id);
        adUnitIDMap.callObjectMethod("put",
                                     "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;",
                                     key.object(),
                                     value.object());
    }

    jlong userPtr = intptr_t(this);
    auto activity =
        qApp->nativeInterface<QNativeInterface::QAndroidApplication>()->context();
    this->m_adManager =
            QJniObject(JCLASS(AdManager),
                       "(JLandroid/app/Activity;Ljava/util/HashMap;)V",
                       userPtr,
                       activity.object(),
                       adUnitIDMap.object());

    if (this->m_adManager.isValid())
        result = this->m_adManager.callMethod<jboolean>("initialize", "()Z");
#endif

    return result;
}

#ifdef Q_OS_ANDROID
void MediaToolsPrivate::adBannerSizeChanged(JNIEnv *env,
                                            jobject obj,
                                            jlong userPtr,
                                            jint width,
                                            jint height)
{
    Q_UNUSED(env)
    Q_UNUSED(obj)

    auto self = reinterpret_cast<MediaToolsPrivate *>(intptr_t(userPtr));

    if (self->m_adBannerWidth != width) {
        self->m_adBannerWidth = width;
        emit self->self->adBannerWidthChanged(width);
    }

    if (self->m_adBannerHeight != height) {
        self->m_adBannerHeight = height;
        emit self->self->adBannerHeightChanged(height);
    }
}

void MediaToolsPrivate::scanCompleted(JNIEnv *env,
                                      jobject obj,
                                      jlong userPtr,
                                      jobject path,
                                      jobject uri)
{
    Q_UNUSED(env)
    Q_UNUSED(obj)
    Q_UNUSED(path)

    auto self = reinterpret_cast<MediaToolsPrivate *>(intptr_t(userPtr));

    if (uri) {
        QJniObject juri = uri;
        self->m_scanResult =
                juri.callObjectMethod("toString",
                                      "()Ljava/lang/String;").toString();
    }

    self->m_scanResultReady = true;
}

QJniObject MediaToolsPrivate::getUriForFile(const QString &filePath)
{
    if (!QFileInfo(filePath).exists()) {
        qWarning() << "File not found:" << filePath;

        return {};
    }

    auto context = QJniObject(QNativeInterface::QAndroidApplication::context());
    auto jniPath = QJniObject::fromString(filePath);

    // Create Java File object
    QJniObject file("java/io/File",
                    "(Ljava/lang/String;)V",
                    jniPath.object<jstring>());

    // Get the app's package name (for the authority)
    auto packageName = context.callObjectMethod("getPackageName",
                                                "()Ljava/lang/String;");
    auto authority = packageName.toString() + ".qtprovider";
    auto jniAuthority = QJniObject::fromString(authority);

    // Obtain the secure URI of the FileProvider
    return QJniObject::callStaticObjectMethod("androidx/core/content/FileProvider",
                                              "getUriForFile",
                                              "(Landroid/content/Context;Ljava/lang/String;Ljava/io/File;)Landroid/net/Uri;",
                                              context.object(),
                                              jniAuthority.object<jstring>(),
                                              file.object());
}

QString MediaToolsPrivate::copyUrlToLocal(const QString &urlOrFile) const
{
    if (!urlOrFile.startsWith("content://"))
        return {};

    auto urlStr = QJniObject::fromString(urlOrFile);

    // Parse the URI

    auto mediaUri =
        QJniObject::callStaticObjectMethod("android/net/Uri",
                                           "parse",
                                           "(Ljava/lang/String;)Landroid/net/Uri;",
                                           urlStr.object());

    // Get ContentResolver

    auto context = QJniObject(QNativeInterface::QAndroidApplication::context());
    auto contentResolver =
        context.callObjectMethod("getContentResolver",
                                 "()Landroid/content/ContentResolver;");

    // Get the file name and the extension

    auto cursor = contentResolver.callObjectMethod("query",
                                                   "(Landroid/net/Uri;"
                                                   "[Ljava/lang/String;"
                                                   "Ljava/lang/String;"
                                                   "[Ljava/lang/String;"
                                                   "Ljava/lang/String;)"
                                                   "Landroid/database/Cursor;",
                                                   mediaUri.object(),
                                                   nullptr,
                                                   nullptr,
                                                   nullptr,
                                                   nullptr);

    if (!cursor.isValid()
        || !cursor.callMethod<jboolean>("moveToFirst", "()Z")) {
        cursor.callMethod<void>("close", "()V");

        return {};
    }

    auto displayName =
        QJniObject::getStaticObjectField("android/provider/OpenableColumns",
                                         "DISPLAY_NAME",
                                         "Ljava/lang/String;");
    auto nameIndex =
    cursor.callMethod<jint>("getColumnIndex",
                            "(Ljava/lang/String;)I",
                            displayName);
    QString fileName;

    if (nameIndex >= 0) {
        fileName = cursor.callObjectMethod("getString",
                                            "(I)Ljava/lang/String;",
                                            nameIndex).toString();
    }

    cursor.callMethod<void>("close", "()V");

    if (fileName.isEmpty())
        return {};

    // Copy the file to the temporary directory

    auto tempDir =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation)
        + "/ImportedFiles";
    QDir().mkpath(tempDir);
    QString filePath = tempDir + "/" + fileName;

    // Copy file

    auto inputStream =
        contentResolver.callObjectMethod("openInputStream",
                                         "(Landroid/net/Uri;)Ljava/io/InputStream;",
                                         mediaUri.object());

    if (!inputStream.isValid()) {
        qCritical() << "Failed to open the file from the URI:" << urlOrFile;

        return {};
    }

    QJniObject outputStream("java/io/FileOutputStream",
                            "(Ljava/lang/String;)V",
                            QJniObject::fromString(filePath).object());

    QJniEnvironment env;
    auto buffer = env->NewByteArray(8192);

    forever {
        auto bytesRead = inputStream.callMethod<jint>("read", "([B)I", buffer);

        if (bytesRead <= 0)
            break;

        outputStream.callMethod<void>("write", "([BII)V", buffer, 0, bytesRead);
    }

    outputStream.callMethod<void>("close", "()V");
    inputStream.callMethod<void>("close", "()V");
    env->DeleteLocalRef(buffer);

    return filePath;
}
#endif

MediaToolsLogger::MediaToolsLogger()
{

}

void MediaToolsLogger::setMediaTools(MediaTools *mediaTools)
{
    if (!this->m_mediaTools)
        this->m_mediaTools = mediaTools;
}

void MediaToolsLogger::setFileName(const QString &fileName)
{
    snprintf(this->m_fileName,
             MAX_STRING_SIZE,
             "%s",
             fileName.toStdString().c_str());
}

bool MediaToolsLogger::writeLine(const QString &msg)
{
    auto len = strnlen(this->m_fileName, MAX_STRING_SIZE);

    if (len < 1)
        return false;

    if (!this->m_fileName[0])
        return false;

    // If the file is not opened, create and open it
    if (!this->m_logFile) {
        auto fileName = QString::fromUtf8(this->m_fileName, len);
        auto saveDirectory = QFileInfo(fileName).absolutePath();

        if (!QDir().exists(saveDirectory) && !QDir().mkpath(saveDirectory))
            return false;

        this->m_logFile = new QFile(fileName);

        if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
            delete this->m_logFile;
            this->m_logFile = nullptr;

            return false;
        }
    }

    // Write the line to the file
    this->m_logFile->write(msg.toUtf8() + "\n");
    this->m_logFile->flush();

    return true;
}

void MediaToolsLogger::close()
{
    if (this->m_logFile)
        return;

    delete this->m_logFile;
    this->m_logFile = nullptr;
}

QString MediaToolsLogger::readLog(quint64 lineStart) const
{
    auto len = strnlen(this->m_fileName, MAX_STRING_SIZE);

    if (len < 1)
        return {};

    auto fileName = QString::fromUtf8(this->m_fileName, len);

    QByteArray logStr;
    QFile file(fileName);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        for (quint64 line = 0; !file.atEnd(); ++line) {
            auto l = file.readLine();

            if (line >= lineStart)
                logStr += l;
        }

        file.close();
    }

    return QString::fromUtf8(logStr);
}

#include "moc_mediatools.cpp"
