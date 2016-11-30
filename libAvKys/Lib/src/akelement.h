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

#ifndef AKELEMENT_H
#define AKELEMENT_H

#include <QStringList>
#include <QQmlEngine>

#include "akaudiopacket.h"
#include "akvideopacket.h"

#define akSend(packet) { \
    if (packet) \
        emit this->oStream(packet); \
    \
    return packet; \
}

class AkElement;
class AkElementPrivate;

typedef QSharedPointer<AkElement> AkElementPtr;

/// Plugin template.
class AkElement: public QObject
{
    Q_OBJECT
    Q_ENUMS(ElementState)
    Q_ENUMS(SearchPaths)
    Q_PROPERTY(QString pluginId
               READ pluginId)
    Q_PROPERTY(QString pluginPath
               READ pluginPath)
    Q_PROPERTY(AkElement::ElementState state
               READ state
               WRITE setState
               RESET resetState
               NOTIFY stateChanged)

    public:
        enum ElementState
        {
            ElementStateNull,
            ElementStatePaused,
            ElementStatePlaying
        };

        enum SearchPaths
        {
            SearchPathsAll,
            SearchPathsDefaults,
            SearchPathsExtras
        };

        explicit AkElement(QObject *parent=NULL);
        virtual ~AkElement();

        Q_INVOKABLE QString pluginId() const;
        Q_INVOKABLE QString pluginPath() const;
        Q_INVOKABLE virtual AkElement::ElementState state() const;
        Q_INVOKABLE virtual QObject *controlInterface(QQmlEngine *engine,
                                                      const QString &controlId) const;

        Q_INVOKABLE virtual bool link(const QObject *dstElement,
                                      Qt::ConnectionType connectionType=Qt::AutoConnection) const;

        Q_INVOKABLE virtual bool link(const AkElementPtr &dstElement,
                                      Qt::ConnectionType connectionType=Qt::AutoConnection) const;

        Q_INVOKABLE virtual bool unlink(const QObject *dstElement) const;
        Q_INVOKABLE virtual bool unlink(const AkElementPtr &dstElement) const;

        Q_INVOKABLE static bool link(const AkElementPtr &srcElement,
                                     const QObject *dstElement,
                                     Qt::ConnectionType connectionType=Qt::AutoConnection);
        Q_INVOKABLE static bool link(const AkElementPtr &srcElement,
                                     const AkElementPtr &dstElement,
                                     Qt::ConnectionType connectionType=Qt::AutoConnection);
        Q_INVOKABLE static bool link(const QObject *srcElement,
                                     const QObject *dstElement,
                                     Qt::ConnectionType connectionType=Qt::AutoConnection);
        Q_INVOKABLE static bool unlink(const AkElementPtr &srcElement,
                                       const QObject *dstElement);
        Q_INVOKABLE static bool unlink(const AkElementPtr &srcElement,
                                       const AkElementPtr &dstElement);
        Q_INVOKABLE static bool unlink(const QObject *srcElement,
                                       const QObject *dstElement);
        Q_INVOKABLE static AkElementPtr create(const QString &pluginId,
                                               const QString &elementName="");
        Q_INVOKABLE static QStringList listSubModules(const QString &pluginId,
                                                      const QString &type="");
        Q_INVOKABLE QStringList listSubModules(const QStringList &types={});
        Q_INVOKABLE static QStringList listSubModulesPaths(const QString &pluginId);
        Q_INVOKABLE QStringList listSubModulesPaths();
        Q_INVOKABLE static QObject *loadSubModule(const QString &pluginId,
                                                  const QString &subModule);
        Q_INVOKABLE QObject *loadSubModule(const QString &subModule);
        Q_INVOKABLE static bool recursiveSearch();
        Q_INVOKABLE static void setRecursiveSearch(bool enable);
        Q_INVOKABLE static QStringList searchPaths(SearchPaths pathType=SearchPathsAll);
        Q_INVOKABLE static void addSearchPath(const QString &path);
        Q_INVOKABLE static void setSearchPaths(const QStringList &searchPaths);
        Q_INVOKABLE static void resetSearchPaths();
        Q_INVOKABLE static QString subModulesPath();
        Q_INVOKABLE static void setSubModulesPath(const QString &subModulesPath);
        Q_INVOKABLE static void resetSubModulesPath();
        Q_INVOKABLE static QStringList listPlugins(const QString &type="");
        Q_INVOKABLE static QStringList listPluginPaths(const QString &searchPath);
        Q_INVOKABLE static QStringList listPluginPaths();
        Q_INVOKABLE static QStringList pluginsCache();
        Q_INVOKABLE static void setPluginsCache(const QStringList &paths);
        Q_INVOKABLE static QStringList pluginsBlackList();
        Q_INVOKABLE static void setPluginsBlackList(const QStringList &blackList);
        Q_INVOKABLE static QString pluginPath(const QString &pluginId);
        Q_INVOKABLE static QVariantMap pluginInfo(const QString &pluginId);
        Q_INVOKABLE static void clearCache();

        virtual AkPacket operator ()(const AkPacket &packet);
        virtual AkPacket operator ()(const AkAudioPacket &packet);
        virtual AkPacket operator ()(const AkVideoPacket &packet);

    private:
        AkElementPrivate *d;

    protected:
        virtual void stateChange(AkElement::ElementState from, AkElement::ElementState to);

    signals:
        void stateChanged(AkElement::ElementState state);
        void oStream(const AkPacket &packet);

    public slots:
        virtual AkPacket iStream(const AkPacket &packet);
        virtual AkPacket iStream(const AkAudioPacket &packet);
        virtual AkPacket iStream(const AkVideoPacket &packet);
        virtual bool setState(AkElement::ElementState state);
        virtual void resetState();
};

QDataStream &operator >>(QDataStream &istream, AkElement::ElementState &state);
QDataStream &operator <<(QDataStream &ostream, AkElement::ElementState state);
Q_DECLARE_METATYPE(AkElement::ElementState)

#endif // AKELEMENT_H
