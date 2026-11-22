/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#ifndef DAILYBUILDINSTALLER_H
#define DAILYBUILDINSTALLER_H

#include <QProcessEnvironment>
#include <QString>

/* What to do with the downloaded and verified file, depending on the
 * platform. It must run only after the user confirms.
 */
class DailyBuildInstaller
{
    public:
        enum Kind {
            Unsupported,          // Android without ENABLE_ANDROID_APK_UPDATES or QJniObject
            StandaloneExecutable, // Linux: the AppImage is not installed, it is just run
            RunsInstaller,        // Windows: the installer is run
            BuildsFromSource,     // macOS: the installer compiles Webcamoid, it needs a terminal
            AndroidPackage        // Android: the system installer is asked to install the APK
        };

        static Kind kindForOs(const QString &os);
        static QString downloadDir();
        static bool makeExecutable(const QString &path, QString *error=nullptr);
        // macOS: script that runs the installer in a terminal window. The
        // installer asks questions, and the license prompt of makeself loops
        // forever when there is no terminal to read the answer.
        static QByteArray terminalScript(const QString &installerPath);
        static bool writeTerminalScript(const QString &installerPath,
                                        const QString &scriptPath,
                                        QString *error=nullptr);
        // When Webcamoid runs from an AppImage, the environment has variables
        // that point inside of it (APPDIR, PATH, LD_LIBRARY_PATH...). This
        // removes them, so the process that is started does not inherit them.
        static QProcessEnvironment cleanEnvironment(const QProcessEnvironment &env);
        // Runs the file. After an installer the application should close.
        static bool startInstaller(const QString &path, QString *error=nullptr);
        // Checks the file and remembers it to be started once the application
        // has finished. It cannot be started before, because the single
        // instance lock would make the new process exit.
        static bool schedule(const QString &path, QString *error=nullptr);
        // Starts the scheduled file, if any. It must be called after the
        // single instance lock was released.
        static bool startScheduled();
        // Android only: the user must allow the app to install packages.
        static bool canInstallPackages();
        // Android only: opens the settings screen for that permission.
        static bool openInstallPermissionSettings();
};

#endif // DAILYBUILDINSTALLER_H
