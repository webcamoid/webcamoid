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

#ifndef AKPLUGINMANAGER_H
#define AKPLUGINMANAGER_H

#include <QDebug>
#include <QMap>
#include <QObject>

#include "akcommons.h"

#define akPluginManager AkPluginManager::instance()

class AkPluginManagerPrivate;
class AkPluginInfo;

using AkPluginLinks = QMap<QString, QString>;

class AKCOMMONS_EXPORT AkPluginManager: public QObject
{
    Q_OBJECT
    Q_FLAGS(PluginsFilter)
    Q_PROPERTY(bool recursiveSearch
               READ recursiveSearch
               WRITE setRecursiveSearch
               RESET resetRecursiveSearch
               NOTIFY recursiveSearchChanged)
    Q_PROPERTY(QStringList searchPaths
               READ searchPaths
               WRITE setSearchPaths
               RESET resetSearchPaths
               NOTIFY searchPathsChanged)
    Q_PROPERTY(AkPluginLinks links
               READ links
               WRITE setLinks
               RESET resetLinks
               NOTIFY linksChanged)

    public:
        enum PluginStatus
        {
            Enabled,
            Disabled
        };
        Q_ENUM(PluginStatus)

        enum PluginsFilter
        {
            FilterNone = 0x0,
            FilterEnabled = 0x1,
            FilterDisabled = 0x2,
            FilterAll = FilterEnabled | FilterDisabled,
            FilterBestMatch = 0x4,
        };
        Q_DECLARE_FLAGS(PluginsFilters, PluginsFilter)
        Q_FLAG(PluginsFilters)
        Q_ENUM(PluginsFilter)

        AkPluginManager(QObject *parent=nullptr);
        AkPluginManager(const AkPluginManager &other);
        virtual ~AkPluginManager();

        Q_INVOKABLE AkPluginInfo pluginInfo(const QString &pluginId) const;
        template<typename T>
        inline QSharedPointer<T> create(const QString &pluginId,
                                        const QStringList &implements={}) const
        {
            auto object = AkPluginManager::create(pluginId, implements);

            if (!object)
                return {};

            return QSharedPointer<T>(reinterpret_cast<T *>(object));
        }
        Q_INVOKABLE QObject *create(const QString &pluginId,
                                    const QStringList &implements={}) const;
        Q_INVOKABLE bool recursiveSearch() const;
        Q_INVOKABLE QStringList searchPaths() const;
        Q_INVOKABLE AkPluginLinks links() const;
        Q_INVOKABLE QStringList listPlugins(const QString &pluginId={},
                                            const QStringList &implements={},
                                            AkPluginManager::PluginsFilters filter=FilterAll) const;
        Q_INVOKABLE AkPluginInfo defaultPlugin(const QString &pluginId,
                                               const QStringList &implements={}) const;
        Q_INVOKABLE AkPluginManager::PluginStatus pluginStatus(const QString &pluginId) const;
        Q_INVOKABLE static void registerTypes();
        Q_INVOKABLE static AkPluginManager *instance();

    private:
        AkPluginManagerPrivate *d;

    signals:
        void recursiveSearchChanged(bool recursiveSearch);
        void searchPathsChanged(const QStringList &paths);
        void pluginsChanged(const QStringList &plugins);
        void linksChanged(const AkPluginLinks &links);
        void pluginsStatusChanged(const QString &pluginId,
                                  AkPluginManager::PluginStatus status);

    public slots:
        void setRecursiveSearch(bool recursiveSearch);
        void addSearchPath(const QString &path);
        void setSearchPaths(const QStringList &searchPaths);
        void setLinks(const AkPluginLinks &links);
        void link(const QString &fromPluginId,
                  const QString &toPluginId);
        void resetRecursiveSearch();
        void resetSearchPaths();
        void resetLinks();
        void scanPlugins();
        void setPluginsStatus(const QStringList &plugins,
                              AkPluginManager::PluginStatus status);
        void setPluginStatus(const QString &pluginId,
                             AkPluginManager::PluginStatus status);
        void setCachedPlugins(const QStringList &plugins);
};

Q_DECLARE_METATYPE(AkPluginManager)
Q_DECLARE_METATYPE(AkPluginManager::PluginStatus)
Q_DECLARE_METATYPE(AkPluginManager::PluginsFilter)
Q_DECLARE_METATYPE(AkPluginManager::PluginsFilters)
Q_DECLARE_OPERATORS_FOR_FLAGS(AkPluginManager::PluginsFilters)

#endif // AKPLUGINMANAGER_H
