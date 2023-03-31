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

#include <QDebug>
#include <QApplication>
#include <QDirIterator>
#include <QFontDatabase>
#include <QMutex>
#include <QQuickStyle>
#include <QTranslator>

#include "clioptions.h"
#include "mediatools.h"

int main(int argc, char *argv[])
{
    QApplication::setApplicationName(COMMONS_APPNAME);
    QApplication::setApplicationVersion(COMMONS_VERSION);
    QApplication::setOrganizationName(COMMONS_APPNAME);
    QApplication::setOrganizationDomain(QString("%1.com").arg(COMMONS_APPNAME));
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    qInstallMessageHandler(MediaTools::messageHandler);

    QApplication app(argc, argv);
    CliOptions cliOptions;

    if (cliOptions.isSet(cliOptions.logFileOpt())) {
        auto logFile = cliOptions.value(cliOptions.logFileOpt());
        qDebug() << "Sending log to" << logFile;

        if (!logFile.isEmpty())
            MediaTools::setLogFile(logFile);
    }

    // Install translations.

    QTranslator translator;

    auto binDir = QDir(BINDIR).absolutePath();
    auto translationsDir = QDir(TRANSLATIONSDIR).absolutePath();
    auto relTranslationsDir = QDir(binDir).relativeFilePath(translationsDir);
    QDir appDir = QCoreApplication::applicationDirPath();

    if (appDir.cd(relTranslationsDir))
        if (translator.load(QLocale::system().name(), appDir.absolutePath()))
            QCoreApplication::installTranslator(&translator);

    // Set theme.
    QQuickStyle::addStylePath(":/Webcamoid/share/themes");
    QQuickStyle::setStyle("WebcamoidTheme");

    // Set fonts
    QDirIterator fontsDirIterator(":/Webcamoid/share/themes/WebcamoidTheme/fonts",
                                  QStringList() << "*.ttf",
                                  QDir::Files
                                  | QDir::Readable
                                  | QDir::NoDotAndDotDot,
                                  QDirIterator::Subdirectories);

    while (fontsDirIterator.hasNext())
        QFontDatabase::addApplicationFont(fontsDirIterator.next());

#if defined(Q_OS_WIN32) || defined(Q_OS_OSX)
    // NOTE: OpenGL detection in Qt is quite buggy, so use software render by default.
    auto quickBackend = qgetenv("QT_QUICK_BACKEND");

    if (quickBackend.isEmpty())
        qputenv("QT_QUICK_BACKEND", "software");
#elif defined(Q_OS_BSD4)
    // NOTE: Text is not rendered with QQC2 in FreeBSD, use native rendering.
    auto distanceField = qgetenv("QML_DISABLE_DISTANCEFIELD");

    if (distanceField.isEmpty())
        qputenv("QML_DISABLE_DISTANCEFIELD", "1");
#endif

    MediaTools mediaTools;

    if (!mediaTools.init(cliOptions))
        return 0;

    mediaTools.show();

    return QApplication::exec();
}
