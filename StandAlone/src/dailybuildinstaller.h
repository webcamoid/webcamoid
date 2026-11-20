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

#include <QString>

/* What to do with the downloaded and verified file, depending on the
 * platform. It must run only after the user confirms.
 */
class DailyBuildInstaller
{
    public:
        enum Kind {
            Unsupported,          // Android: installing an APK is not supported yet
            StandaloneExecutable, // Linux: the AppImage is not installed, it is just run
            RunsInstaller         // Windows and macOS: the installer is run
        };

        static Kind kindForOs(const QString &os);
        static QString downloadDir();
        static bool makeExecutable(const QString &path, QString *error=nullptr);
        // Runs the file. After an installer the application should close.
        static bool startInstaller(const QString &path, QString *error=nullptr);
};

#endif // DAILYBUILDINSTALLER_H
