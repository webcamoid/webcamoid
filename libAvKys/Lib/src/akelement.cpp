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
#include <QCryptographicHash>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QMetaMethod>
#include <QPluginLoader>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QRegExp>
#include <QRegularExpression>
#include <QVector>

#include "akelement.h"
#include "akplugin.h"
#include "akcaps.h"
#include "akpacket.h"
#include "akaudiopacket.h"
#include "akvideopacket.h"

struct AkPluginInfoPrivate
{
    QString id;
    QString path;
    QVariantMap metaData;
    bool used {false};

    AkPluginInfoPrivate(const QString &id,
                        const QString &path,
                        const QVariantMap &metaData,
                        bool used);
};

class AkElementPrivate
{
    public:
        QString m_pluginFilePattern;
        QStringList m_pluginsSearchPaths;
        QStringList m_defaultPluginsSearchPaths;
        QStringList m_pluginsBlackList;
        QVector<AkPluginInfoPrivate> m_pluginsList;
        QDir m_applicationDir;
        AkElement::ElementState m_state {AkElement::ElementStateNull};
        bool m_recursiveSearchPaths {false};
        bool m_pluginsScanned {false};

        AkElementPrivate();
        static QList<QMetaMethod> methodsByName(const QObject *object,
                                                const QString &methodName);
        static bool methodCompat(const QMetaMethod &method1,
                                 const QMetaMethod &method2);
        static QString pluginId(const QString &fileName);
        QString convertToAbsolute(const QString &path) const;
        void listPlugins();
};

Q_GLOBAL_STATIC(AkElementPrivate, akElementGlobalStuff)

AkElement::AkElement(QObject *parent):
    QObject(parent)
{
    this->d = new AkElementPrivate();
}

AkElement::~AkElement()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QString AkElement::pluginId() const
{
    QString className(this->metaObject()->className());
    className.remove(QRegExp("Element$"));

    return className;
}

QString AkElement::pluginIdFromPath(const QString &path)
{
    return akElementGlobalStuff->pluginId(path);
}

QString AkElement::pluginPath() const
{
    return AkElement::pluginPath(this->pluginId());
}

AkElement::ElementState AkElement::state() const
{
    return this->d->m_state;
}

QObject *AkElement::controlInterface(QQmlEngine *engine,
                                     const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return nullptr;

    auto qmlFile = this->controlInterfaceProvide(controlId);

    if (qmlFile.isEmpty())
        return nullptr;

    // Load the UI from the plugin.
    QQmlComponent component(engine, qmlFile);

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return nullptr;
    }

    // Create a context for the plugin.
    auto context = new QQmlContext(engine->rootContext());
    this->controlInterfaceConfigure(context, controlId);

    // Create an item with the plugin context.
    QObject *item = component.create(context);

    if (!item) {
        delete context;

        return nullptr;
    }

    QQmlEngine::setObjectOwnership(item, QQmlEngine::JavaScriptOwnership);
    context->setParent(item);

    return item;
}

bool AkElement::link(const QObject *dstElement,
                     Qt::ConnectionType connectionType) const
{
    return AkElement::link(this, dstElement, connectionType);
}

bool AkElement::link(const AkElementPtr &dstElement,
                     Qt::ConnectionType connectionType) const
{
    return this->link(static_cast<QObject *>(dstElement.data()),
                      connectionType);
}

bool AkElement::unlink(const QObject *dstElement) const
{
    return AkElement::unlink(this, dstElement);
}

bool AkElement::unlink(const AkElementPtr &dstElement) const
{
    return this->unlink(static_cast<QObject *>(dstElement.data()));
}

bool AkElement::link(const AkElementPtr &srcElement,
                     const QObject *dstElement,
                     Qt::ConnectionType connectionType)
{
    return srcElement->link(dstElement, connectionType);
}

bool AkElement::link(const AkElementPtr &srcElement,
                     const AkElementPtr &dstElement,
                     Qt::ConnectionType connectionType)
{
    return srcElement->link(dstElement, connectionType);
}

bool AkElement::link(const QObject *srcElement,
                     const QObject *dstElement,
                     Qt::ConnectionType connectionType)
{
    if (!srcElement || !dstElement)
        return false;

    auto signalList = AkElementPrivate::methodsByName(srcElement, "oStream");
    auto slotList = AkElementPrivate::methodsByName(dstElement, "iStream");

    for (auto &signal: signalList)
        for (auto &slot: slotList)
            if (AkElementPrivate::methodCompat(signal, slot)
                && signal.methodType() == QMetaMethod::Signal
                && slot.methodType() == QMetaMethod::Slot)
                QObject::connect(srcElement,
                                 signal,
                                 dstElement,
                                 slot,
                                 connectionType);

    return true;
}

bool AkElement::unlink(const AkElementPtr &srcElement,
                       const QObject *dstElement)
{
    return srcElement->unlink(dstElement);
}

bool AkElement::unlink(const AkElementPtr &srcElement,
                       const AkElementPtr &dstElement)
{
    return srcElement->unlink(dstElement);
}

bool AkElement::unlink(const QObject *srcElement, const QObject *dstElement)
{
    if (!srcElement || !dstElement)
        return false;

    for (auto &signal: AkElementPrivate::methodsByName(srcElement, "oStream"))
        for (auto &slot: AkElementPrivate::methodsByName(dstElement, "iStream"))
            if (AkElementPrivate::methodCompat(signal, slot) &&
                signal.methodType() == QMetaMethod::Signal &&
                slot.methodType() == QMetaMethod::Slot)
                QObject::disconnect(srcElement, signal, dstElement, slot);

    return true;
}

AkElementPtr AkElement::create(const QString &pluginId,
                               const QString &pluginSub)
{
    return AkElement::create<AkElement>(pluginId, pluginSub);
}

QObject *AkElement::createPtr(const QString &pluginId, const QString &pluginSub)
{
    auto filePath = AkElement::pluginPath(pluginId);

    if (filePath.isEmpty())
        return nullptr;

    QPluginLoader pluginLoader(filePath);

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

    auto object = plugin->create(pluginSub.isEmpty()?
                                     AK_PLUGIN_TYPE_ELEMENT: pluginSub,
                                 "");
    delete plugin;

    return object;
}

QStringList AkElement::listSubModules(const QString &pluginId,
                                      const QString &type)
{
    auto pattern = QString("^%1_(\\w+)$").arg(pluginId);

    if (!akElementGlobalStuff->m_pluginsScanned)
        akElementGlobalStuff->listPlugins();

    QStringList subModules;

    for (auto &pluginInfo: akElementGlobalStuff->m_pluginsList) {
        auto metaData = pluginInfo.metaData["MetaData"].toMap();
        auto submoduleId =
                QRegularExpression(pattern).match(pluginInfo.id).captured(1);

        if (!metaData.contains("pluginType")
            || metaData["pluginType"] != AK_PLUGIN_TYPE_SUBMODULE
            || submoduleId.isEmpty()) {
            continue;
        }

        if (!type.isEmpty()
            && metaData.contains("type")
            && metaData["type"] == type
            && !subModules.contains(pluginInfo.id))
            subModules << submoduleId;
        else if (type.isEmpty() && !subModules.contains(pluginInfo.id))
            subModules << submoduleId;
    }

    subModules.sort();

    return subModules;
}

QStringList AkElement::listSubModules(const QStringList &types)
{
    QString pluginId;

    if (this->pluginId().isEmpty()) {
        pluginId = this->metaObject()->className();
        pluginId.remove(QRegExp("Element$"));
    } else {
        pluginId = this->pluginId();
    }

    if (types.isEmpty())
        return AkElement::listSubModules(pluginId);

    QStringList subModules;

    for (auto &type: types)
        subModules << AkElement::listSubModules(pluginId, type);

    return subModules;
}

QObject *AkElement::loadSubModule(const QString &pluginId,
                                  const QString &subModule)
{
    auto filePath =
            AkElement::pluginPath(QString("%1_%2").arg(pluginId, subModule));

    if (filePath.isEmpty())
        return nullptr;

    QPluginLoader pluginLoader(filePath);

    if (!pluginLoader.load()) {
        qDebug() << QString("Error loading submodule '%1' for '%2' plugin: %3")
                        .arg(subModule,
                             pluginId,
                             pluginLoader.errorString());

        return nullptr;
    }

    auto plugin = qobject_cast<AkPlugin *>(pluginLoader.instance());

    if (!plugin)
        return nullptr;

    auto object = plugin->create(AK_PLUGIN_TYPE_SUBMODULE, "");
    delete plugin;

    return object;
}

QObject *AkElement::loadSubModule(const QString &subModule)
{
    QString pluginId;

    if (this->pluginId().isEmpty()) {
        pluginId = this->metaObject()->className();
        pluginId.remove(QRegExp("Element$"));
    } else {
        pluginId = this->pluginId();
    }

    return AkElement::loadSubModule(pluginId, subModule);
}

bool AkElement::recursiveSearch()
{
    return akElementGlobalStuff->m_recursiveSearchPaths;
}

void AkElement::setRecursiveSearch(bool enable)
{
    akElementGlobalStuff->m_recursiveSearchPaths = enable;
}

QStringList AkElement::searchPaths()
{
    return akElementGlobalStuff->m_pluginsSearchPaths;
}

void AkElement::addSearchPath(const QString &path)
{
    auto absPath = akElementGlobalStuff->convertToAbsolute(path);

    if (!path.isEmpty()
        && QDir(absPath).exists()
        && !akElementGlobalStuff->m_pluginsSearchPaths.contains(absPath))
        akElementGlobalStuff->m_pluginsSearchPaths << absPath;
}

void AkElement::setSearchPaths(const QStringList &searchPaths)
{
    akElementGlobalStuff->m_pluginsSearchPaths.clear();

    for (const QString &path: searchPaths)
        AkElement::addSearchPath(path);
}

void AkElement::resetSearchPaths()
{
    akElementGlobalStuff->m_pluginsSearchPaths.clear();
}

QStringList AkElement::listPlugins(const QString &type)
{
    if (!akElementGlobalStuff->m_pluginsScanned)
        akElementGlobalStuff->listPlugins();

    QStringList plugins;

    for (auto &pluginInfo: akElementGlobalStuff->m_pluginsList) {
        auto metaData = pluginInfo.metaData["MetaData"].toMap();        

        if (!metaData.contains("pluginType")
            || metaData["pluginType"] != AK_PLUGIN_TYPE_ELEMENT) {
            continue;
        }

        if (!type.isEmpty()
            && metaData.contains("type")
            && metaData["type"] == type
            && !plugins.contains(pluginInfo.id))
            plugins << pluginInfo.id;
        else if (type.isEmpty() && !plugins.contains(pluginInfo.id))
            plugins << pluginInfo.id;
    }

    plugins.sort();

    return plugins;
}

QStringList AkElement::listPluginPaths(const QString &searchPath)
{
    if (!akElementGlobalStuff->m_pluginsScanned)
        akElementGlobalStuff->listPlugins();

    QString searchDir(searchPath);
    searchDir.replace(QRegExp(R"(((\\/?)|(/\\?))+)"), QDir::separator());

    QStringList files;

    for (auto &pluginInfo: akElementGlobalStuff->m_pluginsList)
        if (pluginInfo.path.startsWith(searchDir))
            files << pluginInfo.path;

    return files;
}

QStringList AkElement::listPluginPaths(bool all)
{
    if (!akElementGlobalStuff->m_pluginsScanned)
        akElementGlobalStuff->listPlugins();

    QStringList files;

    for (auto &pluginInfo: akElementGlobalStuff->m_pluginsList)
        if (!pluginInfo.path.isEmpty()
            && !pluginInfo.id.isEmpty()
            && (all || pluginInfo.used))
            files << pluginInfo.path;

    return files;
}

void AkElement::setPluginPaths(const QStringList &paths)
{
    for (auto &path: paths) {
        bool contains = false;

        for (auto &pluginInfo: akElementGlobalStuff->m_pluginsList)
            if (pluginInfo.path == path) {
                contains = true;

                break;
            }

        if (!contains)
            akElementGlobalStuff->m_pluginsList
                    << AkPluginInfoPrivate {
                           akElementGlobalStuff->pluginId(path),
                           path,
                           {},
                           false
                       };
    }
}

QStringList AkElement::pluginsBlackList()
{
    return akElementGlobalStuff->m_pluginsBlackList;
}

void AkElement::setPluginsBlackList(const QStringList &blackList)
{
    akElementGlobalStuff->m_pluginsBlackList = blackList;
}

QString AkElement::pluginPath(const QString &pluginId)
{
    auto pluginPaths = AkElement::listPluginPaths();
    QString platformTargetSuffix;

#ifdef Q_OS_ANDROID
    platformTargetSuffix = "_" PLATFORM_TARGET_SUFFIX;
#endif

    for (auto &path: pluginPaths) {
        auto baseName = QFileInfo(path).baseName();

        if (baseName == QString("%1%2%3").arg(PREFIX_SHLIB,
                                              pluginId,
                                              platformTargetSuffix))
            return path;
    }

    return {};
}

QVariantMap AkElement::pluginInfo(const QString &pluginId)
{
    if (!akElementGlobalStuff->m_pluginsScanned)
        akElementGlobalStuff->listPlugins();

    for (auto &pluginInfo: akElementGlobalStuff->m_pluginsList)
        if (pluginInfo.id == pluginId)
            return pluginInfo.metaData;

    return QVariantMap();
}

void AkElement::setPluginInfo(const QString &path,
                              const QVariantMap &metaData)
{
    for (auto &pluginInfo: akElementGlobalStuff->m_pluginsList)
        if (pluginInfo.path == path) {
            pluginInfo.metaData = metaData;

            return;
        }

    akElementGlobalStuff->m_pluginsList
            << AkPluginInfoPrivate {
                   akElementGlobalStuff->pluginId(path),
                   path,
                   metaData,
                   false
               };
}

void AkElement::clearCache()
{
    akElementGlobalStuff->m_pluginsList.clear();
    akElementGlobalStuff->m_pluginsScanned = false;
}

AkPacket AkElement::operator ()(const AkPacket &packet)
{
    return this->iStream(packet);
}

QString AkElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString();
}

void AkElement::controlInterfaceConfigure(QQmlContext *context,
                                          const QString &controlId) const
{
    Q_UNUSED(context)
    Q_UNUSED(controlId)
}

AkPacket AkElement::iAudioStream(const AkAudioPacket &packet)
{
    Q_UNUSED(packet)

    return AkPacket();
}

AkPacket AkElement::iVideoStream(const AkVideoPacket &packet)
{
    Q_UNUSED(packet)

    return AkPacket();
}

AkPacket AkElement::iStream(const AkPacket &packet)
{
    if (packet.caps().mimeType() == "audio/x-raw")
        return this->iAudioStream(packet);

    if (packet.caps().mimeType() == "video/x-raw")
        return this->iVideoStream(packet);

    return AkPacket();
}

bool AkElement::setState(AkElement::ElementState state)
{
    if (this->d->m_state == state)
        return false;

    this->d->m_state = state;
    emit this->stateChanged(state);

    return true;
}

void AkElement::resetState()
{
    this->setState(ElementStateNull);
}

void AkElement::registerTypes()
{
    qRegisterMetaType<AkElementPtr>("AkElementPtr");
    qRegisterMetaType<ElementState>("ElementState");
    qRegisterMetaTypeStreamOperators<ElementState>("ElementState");
    qmlRegisterSingletonType<AkElement>("Ak", 1, 0, "AkElement",
                                        [] (QQmlEngine *qmlEngine,
                                            QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkElement();
    });
}

AkElementPrivate::AkElementPrivate()
{
    auto binDir = QDir(BINDIR).absolutePath();
    auto pluginsDir = QDir(PLUGINSDIR).absolutePath();
    auto relPluginsDir = QDir(binDir).relativeFilePath(pluginsDir);
    QDir appDir = QCoreApplication::applicationDirPath();
    appDir.cd(relPluginsDir);
    this->m_defaultPluginsSearchPaths << appDir.absolutePath();
    this->m_applicationDir.setPath(QCoreApplication::applicationDirPath());
    QString platformTargetSuffix;

#ifdef Q_OS_ANDROID
    platformTargetSuffix = "_" PLATFORM_TARGET_SUFFIX;
#endif

    this->m_pluginFilePattern =
            QString("%1*%2").arg(PREFIX_SHLIB, platformTargetSuffix);

    if (strlen(EXTENSION_SHLIB) > 1)
        this->m_pluginFilePattern += EXTENSION_SHLIB;
}

QList<QMetaMethod> AkElementPrivate::methodsByName(const QObject *object,
                                                   const QString &methodName)
{
    QList<QMetaMethod> methods;
    QStringList methodSignatures;

    for (int i = 0; i < object->metaObject()->methodCount(); i++) {
        QMetaMethod method = object->metaObject()->method(i);
        QString signature(method.methodSignature());

        if (QRegExp(QString(R"(\s*%1\s*\(.*)").arg(methodName))
            .exactMatch(signature))
            if (!methodSignatures.contains(signature)) {
                methods << method;
                methodSignatures << signature;
            }
    }

    return methods;
}

bool AkElementPrivate::methodCompat(const QMetaMethod &method1,
                                    const QMetaMethod &method2)
{
    return method1.parameterTypes() == method2.parameterTypes();
}

QString AkElementPrivate::pluginId(const QString &fileName)
{
    auto pluginId = QFileInfo(fileName).baseName();
    QString platformTargetSuffix;

#ifdef Q_OS_ANDROID
    platformTargetSuffix = "_" PLATFORM_TARGET_SUFFIX;
#endif

    auto pattern =
            QString("^%1(\\w+)%2$").arg(PREFIX_SHLIB, platformTargetSuffix);
    QRegularExpression regex(pattern);

    return regex.match(pluginId).captured(1);
}

QString AkElementPrivate::convertToAbsolute(const QString &path) const
{
    if (!QDir::isRelativePath(path))
        return QDir::cleanPath(path);

    QString absPath = this->m_applicationDir.absoluteFilePath(path);

    return QDir::cleanPath(absPath);
}

void AkElementPrivate::listPlugins()
{
    QVector<QStringList *> sPaths {
        &this->m_pluginsSearchPaths,
        &this->m_defaultPluginsSearchPaths
    };

    for (auto sPath: sPaths)
        for (auto it = sPath->rbegin(); it != sPath->rend(); it++) {
            QString searchDir(*it);

            searchDir.replace(QRegExp(R"(((\\/?)|(/\\?))+)"),
                              QDir::separator());

            while (searchDir.endsWith(QDir::separator()))
                searchDir.resize(searchDir.size() - 1);

            QStringList searchPaths(searchDir);

            while (!searchPaths.isEmpty()) {
                QString path = searchPaths.takeFirst();

                if (this->m_pluginsBlackList.contains(path))
                    continue;

                auto pluginId = this->pluginId(path);
                bool found = false;

                for (auto &pluginInfo: this->m_pluginsList)
                    if (pluginInfo.path == path) {
                        if (pluginInfo.metaData.isEmpty()) {
                            QPluginLoader pluginLoader(path);

                            if (pluginLoader.load()) {
                                if (pluginInfo.id.isEmpty())
                                    pluginInfo.id = pluginId;

                                pluginInfo.metaData =
                                        pluginLoader.metaData().toVariantMap();
                                pluginInfo.used = true;

                                pluginLoader.unload();
                            }
                        } else {
                            if (pluginInfo.id.isEmpty())
                                pluginInfo.id = pluginId;

                            pluginInfo.used = true;
                        }

                        found = true;

                        break;
                    }

                if (found)
                    continue;

                if (QFileInfo(path).isFile()) {
                    QString fileName = QFileInfo(path).fileName();

                    if (QRegExp(this->m_pluginFilePattern,
                                Qt::CaseSensitive,
                                QRegExp::Wildcard).exactMatch(fileName)) {
                        QPluginLoader pluginLoader(path);

                        if (pluginLoader.load()) {
                            auto metaData = pluginLoader.metaData();
                            auto pluginType =
                                    metaData["MetaData"].toObject()
                                                        .value("pluginType")
                                                        .toString();

                            if (pluginType == AK_PLUGIN_TYPE_ELEMENT
                                || pluginType == AK_PLUGIN_TYPE_SUBMODULE) {
                                this->m_pluginsList << AkPluginInfoPrivate {
                                   pluginId,
                                   path,
                                   metaData.toVariantMap(),
                                   true
                                };
                            }

                            pluginLoader.unload();
                        }
                    }
                } else {
                    QDir dir(path);
                    auto fileList = dir.entryList({this->m_pluginFilePattern},
                                                  QDir::Files
                                                  | QDir::CaseSensitive,
                                                  QDir::Name);

                    for (auto &file: fileList)
                        searchPaths << dir.absoluteFilePath(file);

                    if (this->m_recursiveSearchPaths) {
                        auto dirList = dir.entryList(QDir::Dirs
                                                     | QDir::NoDotAndDotDot,
                                                     QDir::Name);

                        for (auto &path: dirList)
                            searchPaths << dir.absoluteFilePath(path);
                    }
                }
            }
        }

    this->m_pluginsScanned = true;
}

AkPluginInfoPrivate::AkPluginInfoPrivate(const QString &id,
                                         const QString &path,
                                         const QVariantMap &metaData,
                                         bool used):
    id(id),
    path(path),
    metaData(metaData),
    used(used)
{

}

QDataStream &operator >>(QDataStream &istream, AkElement::ElementState &state)
{
    int stateInt;
    istream >> stateInt;
    state = static_cast<AkElement::ElementState>(stateInt);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, AkElement::ElementState state)
{
    ostream << static_cast<int>(state);

    return ostream;
}

#include "moc_akelement.cpp"
