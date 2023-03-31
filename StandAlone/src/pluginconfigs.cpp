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

#include <QDir>
#include <QSettings>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <ak.h>
#include <akelement.h>
#include <akpluginmanager.h>

#include "pluginconfigs.h"
#include "clioptions.h"

class PluginConfigsPrivate
{
    public:
        QQmlApplicationEngine *m_engine {nullptr};

        QString convertToAbsolute(const QString &path) const;
};

PluginConfigs::PluginConfigs(const CliOptions &cliOptions,
                             QQmlApplicationEngine *engine,
                             QObject *parent):
    QObject(parent)
{
    this->d = new PluginConfigsPrivate;
    this->setQmlEngine(engine);
    this->loadProperties(cliOptions);
}

PluginConfigs::~PluginConfigs()
{
    this->saveProperties();
    delete this->d;
}

QString PluginConfigsPrivate::convertToAbsolute(const QString &path) const
{
    if (!QDir::isRelativePath(path))
        return QDir::cleanPath(path);

    static const QDir applicationDir(QCoreApplication::applicationDirPath());
    auto absPath = applicationDir.absoluteFilePath(path);

    return QDir::cleanPath(absPath).replace('/', QDir::separator());
}

void PluginConfigs::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->d->m_engine == engine)
        return;

    this->d->m_engine = engine;

    if (engine)
        engine->rootContext()->setContextProperty("pluginConfigs", this);
}

void PluginConfigs::loadProperties(const CliOptions &cliOptions)
{
    QSettings config;

    // Load the list of plugins to be avoided.
    config.beginGroup("DisabledPlugins");
    QStringList disabledPlugins;

    if (cliOptions.isSet(cliOptions.blackListOpt())) {
        auto blackListIds = cliOptions.value(cliOptions.blackListOpt()).split(';');
        disabledPlugins << blackListIds;
    } else {
        int size = config.beginReadArray("plugins");

        for (int i = 0; i < size; i++) {
            config.setArrayIndex(i);
            auto id = config.value("id").toString();

            if (!disabledPlugins.contains(id))
                disabledPlugins << id;
        }

        config.endArray();
    }

    akPluginManager->setPluginsStatus(disabledPlugins,
                                      AkPluginManager::Disabled);
    config.endGroup();

    config.beginGroup("PluginConfigs");

    // Set recursive search.
    if (cliOptions.isSet(cliOptions.recursiveOpt()))
        akPluginManager->setRecursiveSearch(true);
    else if (config.contains("recursive"))
        akPluginManager->setRecursiveSearch(config.value("recursive").toBool());

    // Set alternative paths to search for plugins.
    QStringList searchPaths;

    if (cliOptions.isSet(cliOptions.pluginPathsOpt())) {
        auto pluginPaths = cliOptions.value(cliOptions.pluginPathsOpt()).split(';');

        for (auto &path: pluginPaths) {
#ifdef Q_OS_WIN32
            path = this->d->convertToAbsolute(path);
#endif

            path = QDir::toNativeSeparators(path);

            if (!searchPaths.contains(path))
                searchPaths << path;
        }
    } else {
        // Set the paths for plugins search.
        int size = config.beginReadArray("paths");

        for (int i = 0; i < size; i++) {
            config.setArrayIndex(i);
            auto path = config.value("path").toString();

#ifdef Q_OS_WIN32
            path = this->d->convertToAbsolute(path);
#endif

            path = QDir::toNativeSeparators(path);

            if (!searchPaths.contains(path))
                searchPaths << path;
        }

        config.endArray();
    }

    akPluginManager->setSearchPaths(searchPaths);
    config.endGroup();

    // Load cached plugins
    config.beginGroup("CachedPlugins");
    int size = config.beginReadArray("plugins");
    QStringList cachedPlugins;

    for (int i = 0; i < size; i++) {
        config.setArrayIndex(i);
        cachedPlugins << config.value("id").toString();
    }

    akPluginManager->setCachedPlugins(cachedPlugins);
    config.endArray();
    config.endGroup();
}

void PluginConfigs::saveProperties()
{
    QSettings config;
    config.beginGroup("DisabledPlugins");
    config.beginWriteArray("plugins");
    auto disabledPlugins =
            akPluginManager->listPlugins({},
                                         {},
                                         AkPluginManager::FilterDisabled);
    int i = 0;

    for (auto &id: disabledPlugins) {
        config.setArrayIndex(i);
        config.setValue("id", id);
        i++;
    }

    config.endArray();
    config.endGroup();

    config.beginGroup("PluginConfigs");
    config.setValue("recursive", akPluginManager->recursiveSearch());

    config.beginWriteArray("paths");

    i = 0;

#ifdef Q_OS_WIN32
    QDir applicationDir(QCoreApplication::applicationDirPath());
#endif

    for (auto &path: akPluginManager->searchPaths()) {
        config.setArrayIndex(i);

#ifdef Q_OS_WIN32
        config.setValue("path", applicationDir.relativeFilePath(path));
#else
        config.setValue("path", path);
#endif

        i++;
    }

    config.endArray();
    config.endGroup();

    // Save plugins cache
    config.beginGroup("CachedPlugins");
    config.beginWriteArray("plugins");

    auto pluginsPaths = akPluginManager->listPlugins();
    i = 0;

    for (auto &path: pluginsPaths) {
        config.setArrayIndex(i);
        config.setValue("id", path);
        i++;
    }

    config.endArray();
    config.endGroup();
}

#include "moc_pluginconfigs.cpp"
