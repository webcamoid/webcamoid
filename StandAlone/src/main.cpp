/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
#include <QCommandLineParser>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QWindow>
#include <QTranslator>
#include <QSettings>
#include <QDir>

#include "audiolayer.h"
#include "mediatools.h"
#include "videodisplay.h"
#include "iconsprovider.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
/*
    QPalette palette = app.palette();
    palette.setColor(QPalette::WindowText, QColor(255, 255, 255));
    app.setPalette(palette);
*/
    QCoreApplication::setApplicationName(COMMONS_APPNAME);
    QCoreApplication::setApplicationVersion(COMMONS_VERSION);
    QCoreApplication::setOrganizationName(COMMONS_APPNAME);
    QCoreApplication::setOrganizationDomain(QString("%1.com").arg(COMMONS_APPNAME));

    // Install translations.
    QTranslator translator;
    translator.load(QLocale::system().name(), ":/Webcamoid/share/ts");
    QCoreApplication::installTranslator(&translator);

    // Install fallback icon theme.
    if (QIcon::themeName().isEmpty())
        QIcon::setThemeName("hicolor");

#ifdef Q_OS_OSX
    QIcon fallbackIcon(":/icons/webcamoid.icns");
#elif defined(Q_OS_WIN32)
    QIcon fallbackIcon(":/icons/hicolor/256x256/webcamoid.ico");
#else
    QIcon fallbackIcon(":/icons/hicolor/scalable/webcamoid.svg");
#endif

    app.setWindowIcon(QIcon::fromTheme("webcamoid", fallbackIcon));

    MediaTools::setApplicationDir(QCoreApplication::applicationDirPath());

    QCommandLineParser cliOptions;
    cliOptions.addHelpOption();
    cliOptions.addVersionOption();
    cliOptions.setApplicationDescription(QObject::tr("Webcam capture application."));

    QCommandLineOption configPathOpt(QStringList() << "c" << "config",
                                     QObject::tr("Load settings from PATH. "
                                                 "If PATH is empty, load configs "
                                                 "from application directory."),
                                     "PATH", "");
    cliOptions.addOption(configPathOpt);

    QCommandLineOption qmlPathOpt(QStringList() << "q" << "qmlpath",
                                  QObject::tr("Path to search the Qml interface."),
                                  "PATH", Ak::qmlPluginPath());
    cliOptions.addOption(qmlPathOpt);

    // Set recursive plugin path search.
    QCommandLineOption recursiveOpt(QStringList() << "r" << "recursive",
                                    QObject::tr("Search in the specified plugins paths recursively."));
    cliOptions.addOption(recursiveOpt);

    QCommandLineOption pluginPathsOpt(QStringList() << "p" << "paths",
                                      QObject::tr("Semi-colon separated list of paths to search for plugins."),
                                      "PATH1;PATH2;PATH3;...");
    cliOptions.addOption(pluginPathsOpt);

    cliOptions.process(app);

    // Set path for loading user settings.
    if (cliOptions.isSet(configPathOpt)) {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QString configPath = cliOptions.value(configPathOpt);

        if (configPath.isEmpty())
            configPath = QCoreApplication::applicationDirPath();

        configPath = MediaTools::convertToAbsolute(configPath);

        QSettings::setPath(QSettings::IniFormat,
                           QSettings::UserScope,
                           configPath);
    }

    QSettings config;

    config.beginGroup("PluginConfigs");

    // Set Qml plugins search path.
    QString qmlPluginPath;

    if (cliOptions.isSet(qmlPathOpt))
        qmlPluginPath = cliOptions.value(qmlPathOpt);
    else if (config.contains("qmlPluginPath"))
        qmlPluginPath = config.value("qmlPluginPath").toString();

    if (!qmlPluginPath.isEmpty()) {
#ifdef Q_OS_WIN32
        qmlPluginPath = MediaTools::convertToAbsolute(qmlPluginPath);
#endif

        Ak::setQmlPluginPath(qmlPluginPath);
    }

    // Set recusive search.
    if (cliOptions.isSet(recursiveOpt))
        AkElement::setRecursiveSearch(true);
    else if (config.contains("recursive"))
        AkElement::setRecursiveSearch(config.value("recursive").toBool());

    // Set alternative paths to search for plugins.
    QStringList searchPaths = AkElement::searchPaths();

    if (cliOptions.isSet(pluginPathsOpt)) {
        QStringList pluginPaths = cliOptions.value(pluginPathsOpt)
                                             .split(';');

        for (QString path: pluginPaths) {
#ifdef Q_OS_WIN32
            path = MediaTools::convertToAbsolute(path);
#endif

            path = QDir::toNativeSeparators(path);

            if (!searchPaths.contains(path))
                searchPaths << path;
        }
    } else {
        // Set the paths for plugins search.
        int size = config.beginReadArray("paths");

        for (int i = 0; i < size; i++) {
            config.setArrayIndex(i);
            QString path = config.value("path").toString();

#ifdef Q_OS_WIN32
            path = MediaTools::convertToAbsolute(path);
#endif

            path = QDir::toNativeSeparators(path);

            if (!searchPaths.contains(path))
                searchPaths << path;
        }

        config.endArray();
    }

    AkElement::setSearchPaths(searchPaths);

    config.endGroup();

    // Load cache
    config.beginGroup("PluginsCache");
    int size = config.beginReadArray("paths");
    QStringList pluginsCache;

    for (int i = 0; i < size; i++) {
        config.setArrayIndex(i);
        QString path = config.value("path").toString();

#ifdef Q_OS_WIN32
        path = MediaTools::convertToAbsolute(path);
#endif

        pluginsCache << path;
    }

    AkElement::setPluginsCache(pluginsCache);
    config.endArray();
    config.endGroup();

    // Initialize environment.
    QQmlApplicationEngine engine;
    engine.addImageProvider(QLatin1String("icons"), new IconsProvider);
    Ak::setQmlEngine(&engine);
    AudioLayer audioLayer(&engine);
    MediaTools mediaTools(&engine, &audioLayer);

    // @uri Webcamoid
    qmlRegisterType<VideoDisplay>("Webcamoid", 1, 0, "VideoDisplay");

    engine.rootContext()->setContextProperty("Webcamoid", &mediaTools);
    engine.load(QUrl(QStringLiteral("qrc:/Webcamoid/share/qml/main.qml")));

    emit mediaTools.interfaceLoaded();

    for (const QObject *obj: engine.rootObjects()) {
        // First, find where to enbed the UI.
        VideoDisplay *videoDisplay = obj->findChild<VideoDisplay *>("videoDisplay");

        if (!videoDisplay)
            continue;

        QObject::connect(&mediaTools,
                         &MediaTools::frameReady,
                         videoDisplay,
                         &VideoDisplay::setFrame);

        break;
    }

    return app.exec();
}
