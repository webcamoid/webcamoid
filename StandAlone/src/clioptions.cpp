/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#include <QCoreApplication>
#include <QSettings>
#include <QDir>
#include <ak.h>

#include "clioptions.h"

class CliOptionsPrivate
{
    public:
        QCommandLineOption m_configPathOpt {{"c", "config"}};
        QCommandLineOption m_qmlPathOpt {{"q", "qmlpaths"}};
        QCommandLineOption m_recursiveOpt {{"r", "recursive"}};
        QCommandLineOption m_pluginPathsOpt {{"p", "paths"}};
        QCommandLineOption m_blackListOpt {{"b", "no-load"}};
        QCommandLineOption m_vcamPathOpt {"vcam"};

        QString convertToAbsolute(const QString &path) const;
};

CliOptions::CliOptions()
{
    this->d = new CliOptionsPrivate;
    this->addHelpOption();
    this->addVersionOption();
    this->setApplicationDescription(QObject::tr("Webcam capture application."));

    this->d->m_configPathOpt.setDescription(
                QObject::tr("Load settings from PATH. If PATH is empty, load "
                            "configs from application directory."));
    this->d->m_configPathOpt.setValueName(QObject::tr("PATH"));
    this->addOption(this->d->m_configPathOpt);

    this->d->m_qmlPathOpt.setDescription(
                QObject::tr("Semi-colon separated list of paths to search the "
                            "Qml interface."));
    this->d->m_qmlPathOpt.setValueName(QObject::tr("PATH1;PATH2;PATH3;..."));
    this->addOption(this->d->m_qmlPathOpt);

    // Set recursive plugin path search.
    this->d->m_recursiveOpt.setDescription(
                QObject::tr("Search in the specified plugins paths "
                            "recursively."));
    this->addOption(this->d->m_recursiveOpt);

    this->d->m_pluginPathsOpt.setDescription(
                QObject::tr("Semi-colon separated list of paths to search for "
                            "plugins."));
    this->d->m_pluginPathsOpt.setValueName(QObject::tr("PATH1;PATH2;PATH3;..."));
    this->addOption(this->d->m_pluginPathsOpt);

    this->d->m_blackListOpt.setDescription(
                QObject::tr("Semi-colon separated list of paths to avoid "
                            "loading."));
    this->d->m_blackListOpt.setValueName(QObject::tr("PATH1;PATH2;PATH3;..."));
    this->addOption(this->d->m_blackListOpt);

    this->d->m_vcamPathOpt.setDescription(
                QObject::tr("Semi-colon separated list of paths to search for "
                            "virtual camera driver"));
    this->d->m_vcamPathOpt.setValueName(QObject::tr("PATH1;PATH2;PATH3;..."));
    this->addOption(this->d->m_vcamPathOpt);

    this->process(*QCoreApplication::instance());

    // Set path for loading user settings.
    if (this->isSet(this->d->m_configPathOpt)) {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QString configPath = this->value(this->d->m_configPathOpt);

        if (configPath.isEmpty())
            configPath = QCoreApplication::applicationDirPath();

        configPath = this->d->convertToAbsolute(configPath);

        QSettings::setPath(QSettings::IniFormat,
                           QSettings::UserScope,
                           configPath);
    }
}

CliOptions::~CliOptions()
{
    delete this->d;
}

QCommandLineOption CliOptions::configPathOpt() const
{
    return this->d->m_configPathOpt;
}

QCommandLineOption CliOptions::qmlPathOpt() const
{
    return this->d->m_qmlPathOpt;
}

QCommandLineOption CliOptions::recursiveOpt() const
{
    return this->d->m_recursiveOpt;
}

QCommandLineOption CliOptions::pluginPathsOpt() const
{
    return this->d->m_pluginPathsOpt;
}

QCommandLineOption CliOptions::blackListOpt() const
{
    return this->d->m_blackListOpt;
}

QCommandLineOption CliOptions::vcamPathOpt() const
{
    return this->d->m_vcamPathOpt;
}

QString CliOptionsPrivate::convertToAbsolute(const QString &path) const
{
    if (!QDir::isRelativePath(path))
        return QDir::cleanPath(path);

    static const QDir applicationDir(QCoreApplication::applicationDirPath());
    QString absPath = applicationDir.absoluteFilePath(path);

    return QDir::cleanPath(absPath).replace('/', QDir::separator());
}
