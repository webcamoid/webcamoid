/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTranslator>
#include <QCommandLineParser>

#include "mainwindow.h"
#include "imageprovider.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName(COMMONS_APPNAME);
    QCoreApplication::setApplicationVersion(COMMONS_VERSION);
    QCoreApplication::setOrganizationName(COMMONS_APPNAME);
    QCoreApplication::setOrganizationDomain(QString("%1.com").arg(COMMONS_APPNAME));

    QTranslator translator;
    translator.load(QLocale::system().name(), "qrc:/Webcamoid/share/ts");
    QCoreApplication::installTranslator(&translator);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Webcam capture application."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption enableTestVersion("next", QCoreApplication::translate("main", "Test next version."));
    parser.addOption(enableTestVersion);

    parser.process(app);

    MediaTools *mediaTools = NULL;
    QQmlApplicationEngine *engine = NULL;
    MainWindow *mainWindow = NULL;

    if (parser.isSet(enableTestVersion)) {
        mediaTools = new MediaTools();
        engine = new QQmlApplicationEngine();
        mediaTools->setAppEngine(engine);

        engine->rootContext()->setContextProperty("Webcamoid", mediaTools);

        ImageProvider *imageProvider = new ImageProvider();
        engine->addImageProvider("stream", imageProvider);

        QObject::connect(mediaTools,
                         &MediaTools::frameReady,
                         imageProvider,
                         &ImageProvider::setFrame);

        engine->load(QUrl(QStringLiteral("qrc:/Webcamoid/share/qml/main.qml")));

        emit mediaTools->interfaceLoaded();
    }
    else {
        mainWindow = new MainWindow();
        mainWindow->show();
    }

    int r = app.exec();

    if (mediaTools)
        delete mediaTools;

    if (engine)
        delete engine;

    if (mainWindow)
        delete mainWindow;

    return r;
}
