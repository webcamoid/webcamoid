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

#include <QApplication>
#include <QTranslator>
#include <QPalette>
#include <QIcon>

#include "mediatools.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

#if 0
    QPalette palette = app.palette();
    palette.setColor(QPalette::WindowText, QColor(255, 255, 255));
    app.setPalette(palette);
#endif

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

// OpenGL detection in Qt is quite buggy, so use software render by default.
#if defined(Q_OS_WIN32) || 0
    auto quickBackend = qgetenv("QT_QUICK_BACKEND");

    if (quickBackend.isEmpty())
        qputenv("QT_QUICK_BACKEND", "software");
#endif

    MediaTools mediaTools;
    mediaTools.show();

    return app.exec();
}
