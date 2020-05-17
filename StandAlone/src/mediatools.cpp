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
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
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
#include "videolayer.h"
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
        VideoLayerPtr m_videoLayer;
        AudioLayerPtr m_audioLayer;
        VideoEffectsPtr m_videoEffects;
        RecordingPtr m_recording;
        UpdatesPtr m_updates;
        QSystemTrayIcon *m_trayIcon {nullptr};
        CliOptions m_cliOptions;
        int m_windowWidth {0};
        int m_windowHeight {0};
};

MediaTools::MediaTools(QObject *parent):
    QObject(parent)
{
    this->d = new MediaToolsPrivate;

    // Initialize environment.
    this->d->m_trayIcon = new QSystemTrayIcon(QApplication::windowIcon(), this);
    this->d->m_engine = new QQmlApplicationEngine();
    this->d->m_engine->addImageProvider(QLatin1String("icons"),
                                        new IconsProvider);
    Ak::setQmlEngine(this->d->m_engine);
    this->d->m_pluginConfigs =
            PluginConfigsPtr(new PluginConfigs(this->d->m_cliOptions,
                                               this->d->m_engine));
    this->d->m_videoLayer =
            VideoLayerPtr(new VideoLayer(this->d->m_cliOptions,
                                          this->d->m_engine));
    this->d->m_audioLayer = AudioLayerPtr(new AudioLayer(this->d->m_engine));
    this->d->m_videoEffects =
            VideoEffectsPtr(new VideoEffects(this->d->m_engine));
    this->d->m_recording = RecordingPtr(new Recording(this->d->m_engine));
    this->d->m_updates = UpdatesPtr(new Updates(this->d->m_engine));

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
    QObject::connect(this->d->m_pluginConfigs.data(),
                     &PluginConfigs::pluginsChanged,
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

    for (auto &str: strings)
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

#include "moc_mediatools.cpp"
