/* Webcamoid, webcam capture application.
 * Copyright (C) 2021  Gonzalo Exequiel Pedone
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
#include <QDir>
#include <QDirIterator>
#include <QPluginLoader>
#include <QQmlEngine>
#include <QRegExp>
#include <QStringList>

#include "akpluginmanager.h"
#include "akplugin.h"
#include "akplugininfo.h"

using StringSet = QSet<QString>;

class AkPluginManagerPrivate
{
    public:
        AkPluginManager *self;
        QString m_pluginFilePattern;
        StringSet m_defaultPluginsSearchPaths;
        StringSet m_pluginsSearchPaths;
        StringSet m_cachedPlugins;
        StringSet m_disabledPlugins;
        QVector<AkPluginInfo> m_pluginsList;
        AkPluginLinks m_pluginsLinks;
        bool m_recursiveSearchPaths {false};

        AkPluginManagerPrivate(AkPluginManager *self);
        QString bestMatch(const QStringList &plugins) const;
};

Q_GLOBAL_STATIC(AkPluginManager, akPluginManagerGlobal)

AkPluginManager::AkPluginManager(QObject *parent):
    QObject(parent)
{
    this->d = new AkPluginManagerPrivate(this);
    this->scanPlugins();
}

AkPluginManager::AkPluginManager(const AkPluginManager &other):
    QObject()
{
    this->d = new AkPluginManagerPrivate(this);
    this->d->m_pluginFilePattern = other.d->m_pluginFilePattern;
    this->d->m_defaultPluginsSearchPaths = other.d->m_defaultPluginsSearchPaths;
    this->d->m_pluginsSearchPaths = other.d->m_pluginsSearchPaths;
    this->d->m_disabledPlugins = other.d->m_disabledPlugins;
    this->d->m_pluginsList = other.d->m_pluginsList;
    this->d->m_pluginsLinks = other.d->m_pluginsLinks;
    this->d->m_recursiveSearchPaths = other.d->m_recursiveSearchPaths;
}

AkPluginManager::~AkPluginManager()
{
    delete this->d;
}

AkPluginInfo AkPluginManager::pluginInfo(const QString &pluginId) const
{
    for (auto &pluginInfo: this->d->m_pluginsList)
        if (pluginInfo.id() == pluginId)
            return pluginInfo;

    return {};
}

QObject *AkPluginManager::create(const QString &pluginId,
                                 const QStringList &implements) const
{
    AkPluginInfo pluginInfo = this->defaultPlugin(pluginId, implements);

    if (!pluginInfo)
        return nullptr;

    QPluginLoader pluginLoader(pluginInfo.path());

    if (!pluginLoader.load()) {
        qDebug() << "Error loading plugin "
                 << pluginId
                 << ":"
                 << pluginLoader.errorString();

        return nullptr;
    }

    auto plugin = qobject_cast<AkPlugin *>(pluginLoader.instance());

    if (!plugin)
        return nullptr;

    auto object = plugin->create("", "");
    delete plugin;

    return object;
}

bool AkPluginManager::recursiveSearch() const
{
    return this->d->m_recursiveSearchPaths;
}

QStringList AkPluginManager::searchPaths() const
{
    return this->d->m_pluginsSearchPaths.values();
}

AkPluginLinks AkPluginManager::links() const
{
    return this->d->m_pluginsLinks;
}

QStringList AkPluginManager::listPlugins(const QString &pluginId,
                                         const QStringList &implements,
                                         PluginsFilters filter) const
{
    QStringList plugins;
    QRegExp regexp(pluginId, Qt::CaseSensitive, QRegExp::Wildcard);
    StringSet interfaces(implements.begin(), implements.end());

    if ((filter & FilterAll) == FilterNone)
        filter |= FilterAll;

    for (auto &pluginInfo: this->d->m_pluginsList) {
        if (!pluginId.isEmpty() && !regexp.exactMatch(pluginInfo.id()))
            continue;

        auto implements = pluginInfo.implements();
        StringSet pluginInterfaces(implements.begin(), implements.end());

        if (!implements.isEmpty()
            && !pluginInterfaces.contains(interfaces))
            continue;

        bool isDisabled = this->d->m_disabledPlugins.contains(pluginInfo.id());

        if (((filter & FilterEnabled) && !isDisabled)
            || ((filter & FilterDisabled) && isDisabled))
            plugins << pluginInfo.id();
    }

    if (filter & FilterBestMatch)
        plugins = QStringList {this->d->bestMatch(plugins)};

    plugins.sort();

    return plugins;
}

AkPluginInfo AkPluginManager::defaultPlugin(const QString &pluginId,
                                            const QStringList &implements) const
{
    if (pluginId.isEmpty())
        return nullptr;

    AkPluginInfo pluginInfo;

    if (this->d->m_pluginsLinks.contains(pluginId)) {
        auto plugin = this->d->m_pluginsLinks[pluginId];

        if (!this->d->m_disabledPlugins.contains(plugin))
            return this->pluginInfo(plugin);
    }

    if (!pluginInfo) {
        auto plugins = this->listPlugins(pluginId,
                                         implements,
                                         FilterEnabled | FilterBestMatch);

        if (!plugins.isEmpty())
            return this->pluginInfo(plugins.first());
    }

    return {};
}

AkPluginManager::PluginStatus AkPluginManager::pluginStatus(const QString &pluginId) const
{
    return this->d->m_disabledPlugins.contains(pluginId)? Disabled: Enabled;
}

void AkPluginManager::registerTypes()
{
    qRegisterMetaType<AkPluginLinks>("AkPluginLinks");
    qRegisterMetaType<PluginStatus>("PluginStatus");
    qRegisterMetaType<PluginsFilter>("PluginsFilter");
    qRegisterMetaType<PluginsFilters>("PluginsFilters");
    qmlRegisterSingletonInstance<AkPluginManager>("Ak",
                                                  1,
                                                  0,
                                                  "AkPluginManager",
                                                  akPluginManagerGlobal);
}

AkPluginManager *AkPluginManager::instance()
{
    return akPluginManagerGlobal;
}

void AkPluginManager::setRecursiveSearch(bool recursiveSearch)
{
    if (this->d->m_recursiveSearchPaths == recursiveSearch)
        return;

    this->d->m_recursiveSearchPaths = recursiveSearch;
    emit this->recursiveSearchChanged(recursiveSearch);
}

void AkPluginManager::addSearchPath(const QString &path)
{
    if (!this->d->m_pluginsSearchPaths.contains(path)) {
        this->d->m_pluginsSearchPaths << path;
        emit this->searchPathsChanged(this->d->m_pluginsSearchPaths.values());
    }
}

void AkPluginManager::setSearchPaths(const QStringList &searchPaths)
{
    StringSet _searchPaths(searchPaths.begin(), searchPaths.end());

    if (this->d->m_pluginsSearchPaths == _searchPaths)
        return;

    this->d->m_pluginsSearchPaths = _searchPaths;
    emit this->searchPathsChanged(_searchPaths.values());
}

void AkPluginManager::setLinks(const AkPluginLinks &links)
{
    if (this->d->m_pluginsLinks == links)
        return;

    this->d->m_pluginsLinks = links;
    emit this->linksChanged(this->d->m_pluginsLinks);
}

void AkPluginManager::link(const QString &fromPluginId,
                           const QString &toPluginId)
{
    if (fromPluginId.isEmpty())
        return;

    if (toPluginId.isEmpty()) {
        if (this->d->m_pluginsLinks.contains(fromPluginId)) {
            this->d->m_pluginsLinks.remove(fromPluginId);
            emit this->linksChanged(this->d->m_pluginsLinks);
        }
    } else {
        if (!this->d->m_pluginsLinks.contains(fromPluginId)
            || this->d->m_pluginsLinks.value(fromPluginId) != toPluginId) {
            this->d->m_pluginsLinks[fromPluginId] = toPluginId;
            emit this->linksChanged(this->d->m_pluginsLinks);
        }
    }
}

void AkPluginManager::resetRecursiveSearch()
{
    this->setRecursiveSearch(false);
}

void AkPluginManager::resetSearchPaths()
{
    this->setSearchPaths({});
}

void AkPluginManager::resetLinks()
{
    this->setLinks({});
}

void AkPluginManager::scanPlugins()
{
    auto oldPluginsList = this->d->m_pluginsList;
    this->d->m_pluginsList.clear();

    QVector<StringSet *> sPaths {
        &this->d->m_defaultPluginsSearchPaths,
        &this->d->m_pluginsSearchPaths
    };

    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;

    if (this->d->m_recursiveSearchPaths)
        flags |= QDirIterator::Subdirectories;

    for (auto sPath: sPaths)
        for (auto searchDir: qAsConst(*sPath)) {
            searchDir.replace(QRegExp(R"(((\\/?)|(/\\?))+)"),
                              QDir::separator());

            while (searchDir.endsWith(QDir::separator()))
                searchDir.resize(searchDir.size() - 1);

            QDirIterator searchDirIt(searchDir,
                                     {this->d->m_pluginFilePattern},
                                     QDir::Files | QDir::CaseSensitive,
                                     flags);

            while (searchDirIt.hasNext()) {
                auto pluginPath = searchDirIt.next();
                QPluginLoader pluginLoader(pluginPath);
                auto metaData = pluginLoader.metaData();
                auto info = metaData["MetaData"].toObject();
                auto type = info.value("type").toString();

                if (type != "WebcamoidPluginsCollection")
                    continue;

                auto plugins = info.value("plugins").toVariant().toList();

                for (auto &plugin: plugins) {
                    auto pluginInfo = plugin.toMap();
                    auto pluginType = pluginInfo.value("type").toString();

                    if (pluginType != "qtplugin")
                        continue;

                    auto pluginId = pluginInfo.value("id").toString();

                    if (pluginId.isEmpty())
                        continue;

                    auto it = std::find_if(this->d->m_pluginsList.begin(),
                                           this->d->m_pluginsList.end(),
                                           [pluginId] (const AkPluginInfo &info) {
                        return info.id() == pluginId;
                    });

                    if (it != this->d->m_pluginsList.end())
                        continue;

                    if (this->d->m_cachedPlugins.contains(pluginId)) {
                        this->d->m_pluginsList << AkPluginInfo {pluginInfo};
                    } else {
                        QLibrary libLoader(pluginPath);

                        if (libLoader.load()) {
                            pluginInfo["path"] = pluginPath;
                            this->d->m_pluginsList << AkPluginInfo {pluginInfo};
                            libLoader.unload();
                        } else {
                            qDebug() << libLoader.errorString();
                        }
                    }
                }
            }
        }

    if (this->d->m_pluginsList != oldPluginsList) {
        QStringList plugins;

        for (auto &pluginInfo: this->d->m_pluginsList)
            plugins << pluginInfo.id();

        emit this->pluginsChanged(plugins);
    }
}

void AkPluginManager::setPluginsStatus(const QStringList &plugins,
                                       PluginStatus status)
{
    for (auto &plugin: plugins)
        AkPluginManager::setPluginStatus(plugin, status);
}

void AkPluginManager::setPluginStatus(const QString &pluginId,
                                      PluginStatus status)
{
    switch (status) {
    case Enabled:
        if (this->d->m_disabledPlugins.contains(pluginId)) {
            this->d->m_disabledPlugins.remove(pluginId);
            emit this->pluginsStatusChanged(pluginId, status);
        }

        break;

    case Disabled:
        if (!this->d->m_disabledPlugins.contains(pluginId)) {
            this->d->m_disabledPlugins << pluginId;
            emit this->pluginsStatusChanged(pluginId, status);
        }

        break;
    }
}

void AkPluginManager::setCachedPlugins(const QStringList &plugins)
{
    this->d->m_cachedPlugins = StringSet(plugins.begin(), plugins.end());
}

AkPluginManagerPrivate::AkPluginManagerPrivate(AkPluginManager *self):
    self(self)
{
    auto binDir = QDir(BINDIR).absolutePath();
    auto pluginsDir = QDir(PLUGINSDIR).absolutePath();
    auto relPluginsDir = QDir(binDir).relativeFilePath(pluginsDir);
    QDir appDir = QCoreApplication::applicationDirPath();

    if (appDir.cd(relPluginsDir))
        this->m_defaultPluginsSearchPaths << appDir.absolutePath();

    QString platformTargetSuffix;

#ifdef Q_OS_ANDROID
    platformTargetSuffix = "_" PLATFORM_TARGET_SUFFIX;
#endif

    this->m_pluginFilePattern =
            QString("%1*%2").arg(PREFIX_SHLIB, platformTargetSuffix);

    if (!QString(EXTENSION_SHLIB).isEmpty())
        this->m_pluginFilePattern += EXTENSION_SHLIB;
}

QString AkPluginManagerPrivate::bestMatch(const QStringList &plugins) const
{
    if (plugins.isEmpty())
        return {};

    if (plugins.size() < 2)
        return plugins.first();

    return *std::min_element(plugins.begin(),
                             plugins.end(),
                             [this] (const QString &a, const QString &b) {
        return this->self->pluginInfo(a).priority() > this->self->pluginInfo(b).priority();
    });
}

#include "moc_akpluginmanager.cpp"
