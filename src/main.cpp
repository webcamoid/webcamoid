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
#include <QQuickItem>
#include <QTranslator>

#include "mainwindow.h"
//#include "mediatools.h"

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
/***/
    MediaTools mediaTools;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("Webcamoid", &mediaTools);
    engine.load(QUrl(QStringLiteral("qrc:/Webcamoid/share/qml/main.qml")));

    // Here I'm testing how to embed a control UI from a plugin.

    // First, find where to enbed the UI.
    QQuickItem *item = engine.rootObjects()[0]->findChild<QQuickItem *>("itmMediaControls");

    // Load the UI from the plugin.
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Webcamoid/share/qml/About.qml")));

    // Create a context for the plugin.
    QQmlContext context(engine.rootContext());
    context.setContextProperty("PluginContext", &mediaTools);

    // Create an item with the plugin context.
    QQuickItem *obj = qobject_cast<QQuickItem *>(component.create(&context));

    // Finally, embed the plugin item UI in the desired place.
    obj->setParentItem(item);
//*/
/***
    MainWindow mainWindow;
    mainWindow.show();
//*/
    return app.exec();
}
