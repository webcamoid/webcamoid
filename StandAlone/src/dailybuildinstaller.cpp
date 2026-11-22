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

#if defined(Q_OS_ANDROID) && defined(ENABLE_ANDROID_APK_UPDATES)
#include <QCoreApplication>
#include <QJniEnvironment>
#include <QJniObject>

#define FLAG_GRANT_READ_URI_PERMISSION 0x1
#endif

namespace {
    QString &scheduledPath()
    {
        static QString path;

        return path;
    }
}

DailyBuildInstaller::Kind DailyBuildInstaller::kindForOs(const QString &os)
{
    if (os == "windows")
        return RunsInstaller;

    if (os == "macos")
        return BuildsFromSource;

    if (os == "linux")
        return StandaloneExecutable;

#ifdef ENABLE_ANDROID_APK_UPDATES
    if (os == "android")
        return AndroidPackage;
#endif

    return Unsupported;
}

QString DailyBuildInstaller::downloadDir()
{
#ifdef Q_OS_ANDROID
    // The shared downloads folder is not writable without extra permissions,
    // and the FileProvider of the app can only share its own directories.
    auto dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
               + "/downloads";
    QDir().mkpath(dir);

    return dir;
#else
    auto dir =
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);

    return dir.isEmpty()? QDir::homePath(): dir;
#endif
}

QProcessEnvironment DailyBuildInstaller::cleanEnvironment(const QProcessEnvironment &env)
{
    auto appDir = env.value("APPDIR");

    // Not running from an AppImage.
    if (appDir.isEmpty())
        return env;

    while (appDir.size() > 1 && appDir.endsWith('/'))
        appDir.chop(1);

    auto prefix = appDir + '/';
    auto result = env;

    // Set by the AppImage runtime, the new AppImage sets its own.
    for (auto &key: QStringList {"APPDIR", "APPIMAGE", "ARGV0", "OWD"})
        result.remove(key);

    // The ones set by AppRun to find the libraries and data of the AppImage.
    // They can be lists of paths, so only the entries inside the AppImage are
    // removed, and the variable is removed if nothing else remains.
    for (auto &key: env.keys()) {
        auto value = env.value(key);

        if (!value.contains(appDir))
            continue;

        QStringList kept;
        bool changed = false;

        for (auto &entry: value.split(':')) {
            if (entry == appDir || entry.startsWith(prefix))
                changed = true;
            else
                kept << entry;
        }

        if (!changed)
            continue;

        // Empty entries, that mean the current directory, are dropped too.
        kept.removeAll({});

        if (kept.isEmpty())
            result.remove(key);
        else
            result.insert(key, kept.join(':'));
    }

    return result;
}

QByteArray DailyBuildInstaller::terminalScript(const QString &installerPath)
{
    auto quoted = installerPath;
    quoted.replace('\'', "'\\''");

    // The script deletes itself, and --accept skips the license prompt of the
    // installer: the user already confirmed the installation, and the terminal
    // is only for the questions that Homebrew can ask.
    QString script =
            "#!/bin/sh\n"
            "rm -f \"$0\"\n"
            "echo \"Webcamoid installer\"\n"
            "echo\n"
            "'" + quoted + "' --accept\n"
            "status=$?\n"
            "echo\n"
            "if [ \"$status\" -eq 0 ]; then\n"
            "    echo \"The installation finished.\"\n"
            "else\n"
            "    echo \"The installation failed with code $status.\"\n"
            "fi\n"
            "echo\n"
            "printf \"Press Enter to close this window.\"\n"
            "read -r _\n";

    return script.toUtf8();
}

bool DailyBuildInstaller::writeTerminalScript(const QString &installerPath,
                                              const QString &scriptPath,
                                              QString *error)
{
    QFile file(scriptPath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        auto data = terminalScript(installerPath);
        auto written = file.write(data);
        file.close();

        if (written == data.size()
            && file.setPermissions(QFileDevice::ReadOwner
                                   | QFileDevice::WriteOwner
                                   | QFileDevice::ExeOwner
                                   | QFileDevice::ReadGroup
                                   | QFileDevice::ExeGroup
                                   | QFileDevice::ReadOther
                                   | QFileDevice::ExeOther))
            return true;
    }

    if (error)
        *error = QObject::tr("Cannot create %1").arg(scriptPath);

    return false;
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
#if defined(Q_OS_ANDROID) && defined(ENABLE_ANDROID_APK_UPDATES)
    QJniEnvironment env;
    QJniObject context(QNativeInterface::QAndroidApplication::context());

    if (!context.isValid() || !QFile::exists(path)) {
        if (error)
            *error = QObject::tr("Cannot start the installer %1").arg(path);

        return false;
    }

    // Get a content URI that the system installer can read.
    QJniObject file("java/io/File",
                    "(Ljava/lang/String;)V",
                    QJniObject::fromString(path).object<jstring>());
    auto packageName = context.callObjectMethod("getPackageName",
                                                "()Ljava/lang/String;");
    auto authority =
            QJniObject::fromString(packageName.toString() + ".qtprovider");
    auto uri =
            QJniObject::callStaticObjectMethod("androidx/core/content/FileProvider",
                                               "getUriForFile",
                                               "(Landroid/content/Context;Ljava/lang/String;Ljava/io/File;)Landroid/net/Uri;",
                                               context.object(),
                                               authority.object<jstring>(),
                                               file.object());

    if (env.checkAndClearExceptions() || !uri.isValid()) {
        if (error)
            *error = QObject::tr("Cannot share %1 with the installer").arg(path);

        return false;
    }

    QJniObject intent("android/content/Intent",
                      "(Ljava/lang/String;)V",
                      QJniObject::fromString("android.intent.action.VIEW").object<jstring>());
    intent.callObjectMethod("setDataAndType",
                            "(Landroid/net/Uri;Ljava/lang/String;)Landroid/content/Intent;",
                            uri.object(),
                            QJniObject::fromString("application/vnd.android.package-archive").object<jstring>());
    intent.callObjectMethod("addFlags",
                            "(I)Landroid/content/Intent;",
                            FLAG_GRANT_READ_URI_PERMISSION);
    context.callMethod<void>("startActivity",
                             "(Landroid/content/Intent;)V",
                             intent.object());

    if (!env.checkAndClearExceptions())
        return true;

    if (error)
        *error = QObject::tr("Cannot start the installer %1").arg(path);

    return false;
#elif defined(Q_OS_MACOS)
    if (!makeExecutable(path, error))
        return false;

    auto script = path + ".command";

    if (!writeTerminalScript(path, script, error))
        return false;

    // Terminal runs the .command files.
    if (QProcess::startDetached("/usr/bin/open", {script}))
        return true;

    if (error)
        *error = QObject::tr("Cannot start the installer %1").arg(path);

    return false;
#else
#ifndef Q_OS_WIN
    if (!makeExecutable(path, error))
        return false;
#endif

    QProcess process;
    process.setProgram(path);
    process.setProcessEnvironment(cleanEnvironment(QProcessEnvironment::systemEnvironment()));

    if (process.startDetached())
        return true;

    if (error)
        *error = QObject::tr("Cannot start the installer %1").arg(path);

    return false;
#endif
}

bool DailyBuildInstaller::schedule(const QString &path, QString *error)
{
    if (!QFile::exists(path)) {
        if (error)
            *error = QObject::tr("Cannot find %1").arg(path);

        return false;
    }

#ifndef Q_OS_WIN
    if (!makeExecutable(path, error))
        return false;
#endif

#ifdef Q_OS_MACOS
    // Fail now, not after closing the application.
    if (!writeTerminalScript(path, path + ".command", error))
        return false;
#endif

    scheduledPath() = path;

    return true;
}

bool DailyBuildInstaller::startScheduled()
{
    auto path = scheduledPath();
    scheduledPath().clear();

    if (path.isEmpty())
        return false;

    return startInstaller(path);
}

bool DailyBuildInstaller::canInstallPackages()
{
#if defined(Q_OS_ANDROID) && defined(ENABLE_ANDROID_APK_UPDATES)
    // Before Android 8 it is a global setting that the system installer checks.
    if (QNativeInterface::QAndroidApplication::sdkVersion() < 26)
        return true;

    QJniObject context(QNativeInterface::QAndroidApplication::context());

    if (!context.isValid())
        return false;

    auto packageManager =
            context.callObjectMethod("getPackageManager",
                                     "()Landroid/content/pm/PackageManager;");

    if (!packageManager.isValid())
        return false;

    return packageManager.callMethod<jboolean>("canRequestPackageInstalls",
                                               "()Z");
#else
    return false;
#endif
}

bool DailyBuildInstaller::openInstallPermissionSettings()
{
#if defined(Q_OS_ANDROID) && defined(ENABLE_ANDROID_APK_UPDATES)
    QJniEnvironment env;
    QJniObject context(QNativeInterface::QAndroidApplication::context());

    if (!context.isValid())
        return false;

    auto packageName = context.callObjectMethod("getPackageName",
                                                "()Ljava/lang/String;");
    auto uri =
            QJniObject::callStaticObjectMethod("android/net/Uri",
                                               "parse",
                                               "(Ljava/lang/String;)Landroid/net/Uri;",
                                               QJniObject::fromString("package:" + packageName.toString()).object<jstring>());
    QJniObject intent("android/content/Intent",
                      "(Ljava/lang/String;Landroid/net/Uri;)V",
                      QJniObject::fromString("android.settings.MANAGE_UNKNOWN_APP_SOURCES").object<jstring>(),
                      uri.object());
    context.callMethod<void>("startActivity",
                             "(Landroid/content/Intent;)V",
                             intent.object());

    return !env.checkAndClearExceptions();
#else
    return false;
#endif
}
