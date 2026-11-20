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

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>

#include "dailybuildinstaller.h"

DailyBuildInstaller::Kind DailyBuildInstaller::kindForOs(const QString &os)
{
    if (os == "windows" || os == "macos")
        return RunsInstaller;

    if (os == "linux")
        return StandaloneExecutable;

    return Unsupported;
}

QString DailyBuildInstaller::downloadDir()
{
    auto dir =
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);

    return dir.isEmpty()? QDir::homePath(): dir;
}

bool DailyBuildInstaller::makeExecutable(const QString &path, QString *error)
{
    auto perms = QFile::permissions(path)
                 | QFileDevice::ExeOwner
                 | QFileDevice::ExeGroup
                 | QFileDevice::ExeOther;

    if (QFile::setPermissions(path, perms))
        return true;

    if (error)
        *error = QObject::tr("Cannot make %1 executable").arg(path);

    return false;
}

bool DailyBuildInstaller::startInstaller(const QString &path, QString *error)
{
#ifndef Q_OS_WIN
    if (!makeExecutable(path, error))
        return false;
#endif

    if (QProcess::startDetached(path, {}))
        return true;

    if (error)
        *error = QObject::tr("Cannot start the installer %1").arg(path);

    return false;
}
