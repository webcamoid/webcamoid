/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

CliOptions::CliOptions():
    QCommandLineParser()
{
    this->addHelpOption();
    this->addVersionOption();
    this->setApplicationDescription(QObject::tr("Webcam capture application."));

    this->m_configPathOpt =
            new QCommandLineOption({"c", "config"},
                                   QObject::tr("Load settings from PATH. "
                                               "If PATH is empty, load configs "
                                               "from application directory."),
                                   "PATH", "");
    this->addOption(*this->m_configPathOpt);

    this->m_qmlPathOpt =
            new QCommandLineOption({"q", "qmlpath"},
                                   QObject::tr("Path to search the Qml "
                                               "interface."),
                                   "PATH", Ak::qmlPluginPath());
    this->addOption(*this->m_qmlPathOpt);

    // Set recursive plugin path search.
    this->m_recursiveOpt =
            new QCommandLineOption({"r", "recursive"},
                                   QObject::tr("Search in the specified "
                                               "plugins paths recursively."));
    this->addOption(*this->m_recursiveOpt);

    this->m_pluginPathsOpt =
            new QCommandLineOption({"p", "paths"},
                                   QObject::tr("Semi-colon separated list of "
                                               "paths to search for plugins."),
                                   "PATH1;PATH2;PATH3;...");
    this->addOption(*this->m_pluginPathsOpt);

    this->m_blackListOpt =
            new QCommandLineOption({"b", "no-load"},
                                   QObject::tr("Semi-colon separated list of "
                                               "paths to avoid loading."),
                                   "PATH1;PATH2;PATH3;...");
    this->addOption(*this->m_blackListOpt);

    this->process(*QCoreApplication::instance());

    // Set path for loading user settings.
    if (this->isSet(*this->m_configPathOpt)) {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QString configPath = this->value(*this->m_configPathOpt);

        if (configPath.isEmpty())
            configPath = QCoreApplication::applicationDirPath();

        configPath = this->convertToAbsolute(configPath);

        QSettings::setPath(QSettings::IniFormat,
                           QSettings::UserScope,
                           configPath);
    }
}

CliOptions::~CliOptions()
{
    delete this->m_configPathOpt;
    delete this->m_qmlPathOpt;
    delete this->m_recursiveOpt;
    delete this->m_pluginPathsOpt;
    delete this->m_blackListOpt;
}

QCommandLineOption CliOptions::configPathOpt() const
{
    return *this->m_configPathOpt;
}

QCommandLineOption CliOptions::qmlPathOpt() const
{
    return *this->m_qmlPathOpt;
}

QCommandLineOption CliOptions::recursiveOpt() const
{
    return *this->m_recursiveOpt;
}

QCommandLineOption CliOptions::pluginPathsOpt() const
{
    return *this->m_pluginPathsOpt;
}

QCommandLineOption CliOptions::blackListOpt() const
{
    return *this->m_blackListOpt;
}

QString CliOptions::convertToAbsolute(const QString &path) const
{
    if (!QDir::isRelativePath(path))
        return QDir::cleanPath(path);

    static const QDir applicationDir(QCoreApplication::applicationDirPath());
    QString absPath = applicationDir.absoluteFilePath(path);

    return QDir::cleanPath(absPath).replace('/', QDir::separator());
}
