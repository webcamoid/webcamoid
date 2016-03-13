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

#include "mediatools.h"
#include "videodisplay.h"

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

#ifdef Q_OS_WIN32
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat,
                       QSettings::UserScope,
                       QCoreApplication::applicationDirPath());
#endif

    QCommandLineParser cliOptions;
    cliOptions.addHelpOption();
    cliOptions.addVersionOption();
    cliOptions.setApplicationDescription(QObject::tr("Webcam capture application."));

    QSettings config;

    config.beginGroup("PluginConfigs");

#ifdef Q_OS_WIN32
    QDir applicationDir(QCoreApplication::applicationDirPath());
#endif

    QString qmlPluginPath = config.value("qmlPluginPath", Ak::qmlPluginPath())
                                  .toString();

    QCommandLineOption qmlPathOpt(QStringList() << "q" << "qmlpath",
                                  QObject::tr("Path to search the Qml interface."),
                                  "PATH", qmlPluginPath);
    cliOptions.addOption(qmlPathOpt);

    // Set recursive plugin path search.
    bool recursive = config.value("recursive", false).toBool();

    QCommandLineOption recursiveOpt(QStringList() << "r" << "recursive",
                                    QObject::tr("Search in the specified plugins paths recursively."));
    cliOptions.addOption(recursiveOpt);

    // Set the paths for plugins search.
    QStringList defaultPluginPaths = AkElement::searchPaths();
    int size = config.beginReadArray("paths");

    for (int i = 0; i < size; i++) {
        config.setArrayIndex(i);
        QString path = config.value("path").toString();

#ifdef Q_OS_WIN32
        if (QDir::isRelativePath(path)) {
            path = applicationDir.absoluteFilePath(path);
            path = QDir::cleanPath(path);
        }
#endif

        path = QDir::toNativeSeparators(path);

        if (!defaultPluginPaths.contains(path))
            AkElement::addSearchPath(path);
    }

    QCommandLineOption pluginPathsOpt(QStringList() << "p" << "paths",
                                      QObject::tr("Semi-colon separated list of paths to search for plugins."),
                                      "PATH1;PATH2;PATH3;...");
    cliOptions.addOption(pluginPathsOpt);

    config.endArray();
    config.endGroup();

    cliOptions.process(app);

    // Set Qml plugins search path.
    if (cliOptions.isSet(qmlPathOpt))
        qmlPluginPath = cliOptions.value(qmlPathOpt);

#ifdef Q_OS_WIN32
    if (QDir::isRelativePath(qmlPluginPath)) {
        qmlPluginPath = applicationDir.absoluteFilePath(qmlPluginPath);
        qmlPluginPath = QDir::cleanPath(qmlPluginPath);
    }
#endif

    Ak::setQmlPluginPath(qmlPluginPath);

    // Set recusive search.
    if (cliOptions.isSet(recursiveOpt))
        recursive = true;

    AkElement::setRecursiveSearch(recursive);

    // Set alternative paths to search for plugins.
    if (cliOptions.isSet(pluginPathsOpt)) {
        QStringList defaultPluginPaths = AkElement::searchPaths();
        QStringList pluginPaths = cliOptions.value(pluginPathsOpt)
                                             .split(';');

        foreach (QString path, pluginPaths) {
#ifdef Q_OS_WIN32
            if (QDir::isRelativePath(path)) {
                path = applicationDir.absoluteFilePath(path);
                path = QDir::cleanPath(path);
            }
#endif

            path = QDir::toNativeSeparators(path);

            if (!defaultPluginPaths.contains(path))
                AkElement::addSearchPath(path);
        }
    }

    // Initialize environment.
    QQmlApplicationEngine engine;
    MediaTools mediaTools(&engine);

    // @uri Webcamoid
    qmlRegisterType<VideoDisplay>("Webcamoid", 1, 0, "VideoDisplay");

    engine.rootContext()->setContextProperty("Webcamoid", &mediaTools);
    engine.load(QUrl(QStringLiteral("qrc:/Webcamoid/share/qml/main.qml")));

    emit mediaTools.interfaceLoaded();

    foreach (QObject *obj, engine.rootObjects()) {
        QWindow *applicationWindow = qobject_cast<QWindow *>(obj);

        // Set window icon.
        applicationWindow->setIcon(QIcon::fromTheme("webcamoid",
                                                    QIcon(":/icons/hicolor/scalable/webcamoid.svg")));

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
