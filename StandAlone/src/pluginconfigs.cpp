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

#include <QDir>
#include <QSettings>
#include <ak.h>

#include "pluginconfigs.h"

PluginConfigs::PluginConfigs(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent),
    m_engine(nullptr)
{
    this->setQmlEngine(engine);
}

PluginConfigs::PluginConfigs(const CliOptions &cliOptions,
                             QQmlApplicationEngine *engine,
                             QObject *parent):
    QObject(parent),
    m_engine(nullptr)
{
    this->setQmlEngine(engine);
    this->loadProperties(cliOptions);
}

PluginConfigs::~PluginConfigs()
{
    this->saveProperties();
}

QString PluginConfigs::convertToAbsolute(const QString &path) const
{
    if (!QDir::isRelativePath(path))
        return QDir::cleanPath(path);

    static const QDir applicationDir(QCoreApplication::applicationDirPath());
    QString absPath = applicationDir.absoluteFilePath(path);

    return QDir::cleanPath(absPath).replace('/', QDir::separator());
}

void PluginConfigs::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->m_engine == engine)
        return;

    this->m_engine = engine;

    if (engine)
        engine->rootContext()->setContextProperty("PluginConfigs", this);
}

void PluginConfigs::loadProperties(const CliOptions &cliOptions)
{
    QSettings config;

    // Load the list of plugins to be avoided.
    config.beginGroup("PluginBlackList");
    auto blackList = AkElement::pluginsBlackList();

    if (cliOptions.isSet(cliOptions.blackListOpt())) {
        auto blackListPaths = cliOptions.value(cliOptions.blackListOpt()).split(';');

        for (QString &path: blackListPaths) {
#ifdef Q_OS_WIN32
            path = this->convertToAbsolute(path);
#endif

            path = QDir::toNativeSeparators(path);

            if (!blackList.contains(path))
                blackList << path;
        }
    } else {
        int size = config.beginReadArray("paths");

        for (int i = 0; i < size; i++) {
            config.setArrayIndex(i);
            auto path = config.value("path").toString();

#ifdef Q_OS_WIN32
            path = this->convertToAbsolute(path);
#endif

            path = QDir::toNativeSeparators(path);

            if (!blackList.contains(path))
                blackList << path;
        }

        config.endArray();
    }

    AkElement::setPluginsBlackList(blackList);
    config.endGroup();

    config.beginGroup("PluginConfigs");

    // Set Qml plugins search path.
    QStringList qmlImportPaths;

    if (cliOptions.isSet(cliOptions.qmlPathOpt()))
        qmlImportPaths = cliOptions.value(cliOptions.qmlPathOpt()).split(';');
    else {
        int size = config.beginReadArray("qmlPaths");

        for (int i = 0; i < size; i++) {
            config.setArrayIndex(i);
            auto path = config.value("path").toString();

#ifdef Q_OS_WIN32
            path = this->convertToAbsolute(path);
#endif

            path = QDir::toNativeSeparators(path);

            if (!qmlImportPaths.contains(path))
                qmlImportPaths << path;
        }

        config.endArray();
    }

    if (!qmlImportPaths.isEmpty())
        Ak::setQmlImportPathList(qmlImportPaths);

    // Set recusive search.
    if (cliOptions.isSet(cliOptions.recursiveOpt()))
        AkElement::setRecursiveSearch(true);
    else if (config.contains("recursive"))
        AkElement::setRecursiveSearch(config.value("recursive").toBool());

    // Set alternative paths to search for plugins.
    auto searchPaths = AkElement::searchPaths();

    if (cliOptions.isSet(cliOptions.pluginPathsOpt())) {
        auto pluginPaths = cliOptions.value(cliOptions.pluginPathsOpt()).split(';');

        for (QString &path: pluginPaths) {
#ifdef Q_OS_WIN32
            path = this->convertToAbsolute(path);
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
            path = this->convertToAbsolute(path);
#endif

            path = QDir::toNativeSeparators(path);

            if (!searchPaths.contains(path))
                searchPaths << path;
        }

        config.endArray();
    }

    AkElement::setSearchPaths(searchPaths);

    config.endGroup();

    // Load plugins hashes
    config.beginGroup("PluginsHashes");
    int size = config.beginReadArray("hashes");
    QList<QByteArray> pluginsHashes;

    for (int i = 0; i < size; i++) {
        config.setArrayIndex(i);
        pluginsHashes << config.value("hash").toByteArray();
    }

    AkElement::setPluginsHashes(pluginsHashes);
    config.endArray();
    config.endGroup();

    this->m_plugins = AkElement::listPluginPaths();
}

void PluginConfigs::saveProperties()
{
    QSettings config;

#ifdef Q_OS_WIN32
    static const QDir applicationDir(QCoreApplication::applicationDirPath());
#endif

    config.beginGroup("PluginBlackList");
    config.beginWriteArray("paths");

    int i = 0;

    for (const QString &path: AkElement::pluginsBlackList()) {
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

    config.beginGroup("PluginConfigs");
    config.beginWriteArray("qmlPaths");
    i = 0;

    for (auto &path: Ak::qmlImportPathList()) {
        config.setArrayIndex(i);

#ifdef Q_OS_WIN32
        config.setValue("path", applicationDir.relativeFilePath(path));
#else
        config.setValue("path", path);
#endif

        i++;
    }

    config.endArray();

    config.setValue("recursive", AkElement::recursiveSearch());

    config.beginWriteArray("paths");

    i = 0;

    for (const QString &path: AkElement::searchPaths()) {
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

    // Save plugins hashes
    config.beginGroup("PluginsHashes");
    config.beginWriteArray("hashes");

    i = 0;

    for (auto &hash: AkElement::pluginsHashes()) {
        config.setArrayIndex(i);
        config.setValue("hash", hash);
        i++;
    }

    config.endArray();
    config.endGroup();

    QStringList plugins = AkElement::listPluginPaths();

    if (this->m_plugins != plugins) {
        this->m_plugins = plugins;
        emit this->pluginsChanged(plugins);
    }
}
