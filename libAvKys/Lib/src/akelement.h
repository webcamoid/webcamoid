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

#ifndef AKELEMENT_H
#define AKELEMENT_H

#include <QObject>

#include "akcommons.h"

#define akSend(packet) { \
    if (packet) \
        emit this->oStream(packet); \
    \
    return packet; \
}

class AkElement;
class AkElementPrivate;
class AkPacket;
class AkAudioPacket;
class AkVideoPacket;
class QDataStream;
class QQmlEngine;
class QQmlContext;

using AkElementPtr = QSharedPointer<AkElement>;

/// Plugin template.
class AKCOMMONS_EXPORT AkElement: public QObject
{
    Q_OBJECT
    Q_ENUMS(ElementState)
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

        AkElement(QObject *parent=nullptr);
        virtual ~AkElement();

        Q_INVOKABLE virtual QString pluginId() const;
        Q_INVOKABLE static QString pluginIdFromPath(const QString &path);
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
        template<typename T>
        static inline QSharedPointer<T> create(const QString &pluginId,
                                               const QString &pluginSub={})
        {
            auto object = AkElement::createPtr(pluginId, pluginSub);

            if (!object)
                return {};

            return QSharedPointer<T>(reinterpret_cast<T *>(object));
        }
        Q_INVOKABLE static AkElementPtr create(const QString &pluginId,
                                               const QString &pluginSub={});
        Q_INVOKABLE static QObject *createPtr(const QString &pluginId,
                                              const QString &pluginSub={});
        Q_INVOKABLE static QStringList listSubModules(const QString &pluginId,
                                                      const QString &type={});
        Q_INVOKABLE QStringList listSubModules(const QStringList &types={});
        Q_INVOKABLE static QStringList listSubModulesPaths(const QString &pluginId);
        Q_INVOKABLE QStringList listSubModulesPaths();
        Q_INVOKABLE static QObject *loadSubModule(const QString &pluginId,
                                                  const QString &subModule);
        Q_INVOKABLE QObject *loadSubModule(const QString &subModule);
        Q_INVOKABLE static bool recursiveSearch();
        Q_INVOKABLE static void setRecursiveSearch(bool enable);
        Q_INVOKABLE static QStringList searchPaths();
        Q_INVOKABLE static void addSearchPath(const QString &path);
        Q_INVOKABLE static void setSearchPaths(const QStringList &searchPaths);
        Q_INVOKABLE static void resetSearchPaths();
        Q_INVOKABLE static QString subModulesPath();
        Q_INVOKABLE static void setSubModulesPath(const QString &subModulesPath);
        Q_INVOKABLE static void resetSubModulesPath();
        Q_INVOKABLE static QStringList listPlugins(const QString &type="");
        Q_INVOKABLE static QStringList listPluginPaths(const QString &searchPath);
        Q_INVOKABLE static QStringList listPluginPaths(bool all=false);
        Q_INVOKABLE static void setPluginPaths(const QStringList &paths);
        Q_INVOKABLE static QStringList pluginsBlackList();
        Q_INVOKABLE static void setPluginsBlackList(const QStringList &blackList);
        Q_INVOKABLE static QString pluginPath(const QString &pluginId);
        Q_INVOKABLE static QVariantMap pluginInfo(const QString &pluginId);
        Q_INVOKABLE static void setPluginInfo(const QString &path,
                                              const QVariantMap &metaData);
        Q_INVOKABLE static void clearCache();

        virtual AkPacket operator ()(const AkPacket &packet);

    private:
        AkElementPrivate *d;

    protected:
        virtual QString controlInterfaceProvide(const QString &controlId) const;
        virtual void controlInterfaceConfigure(QQmlContext *context,
                                               const QString &controlId) const;
        virtual AkPacket iAudioStream(const AkAudioPacket &packet);
        virtual AkPacket iVideoStream(const AkVideoPacket &packet);

    Q_SIGNALS:
        void stateChanged(AkElement::ElementState state);
        void oStream(const AkPacket &packet);

    public Q_SLOTS:
        virtual AkPacket iStream(const AkPacket &packet);
        virtual bool setState(AkElement::ElementState state);
        virtual void resetState();
        static void registerTypes();
};

AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkElement::ElementState &state);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, AkElement::ElementState state);

Q_DECLARE_METATYPE(AkElement::ElementState)

#endif // AKELEMENT_H
