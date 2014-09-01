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
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include <QApplication>
#include <QDesktopWidget>

#include "mainwidget.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow mainWindow;
    MainWidget *mainWidget = new MainWidget();

    mainWindow.setWindowIcon(mainWidget->windowIcon());
    mainWindow.setWindowTitle(mainWidget->windowTitle());

    QObject::connect(&mainWindow,
                     SIGNAL(windowClosed()),
                     mainWidget,
                     SLOT(cleanAll()));

    QRect geometry = mainWidget->geometry();
    QDesktopWidget desktopWidget;

    geometry.moveCenter(desktopWidget.availableGeometry().center());
    mainWindow.setGeometry(geometry);

    mainWindow.setCentralWidget(mainWidget);
    mainWindow.show();

    return app.exec();
}
