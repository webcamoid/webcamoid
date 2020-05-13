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

#define SUBMODULES_PATH "submodules"

class AkPluginInfoPrivate
{
    public:
        QString m_id;
        QString m_path;
        QVariantMap m_metaData;
        bool m_used;
};

class AkElementPrivate
{
    public:
        QString m_pluginFilePattern;
        QStringList m_pluginsSearchPaths;
        QStringList m_defaultPluginsSearchPaths;
        QStringList m_pluginsBlackList;
        QList<AkPluginInfoPrivate> m_pluginsList;
        QString m_subModulesPath;
        QDir m_applicationDir;
        AkElement::ElementState m_state;
        bool m_recursiveSearchPaths;
        bool m_pluginsScanned;

        AkElementPrivate();
        static QList<QMetaMethod> methodsByName(const QObject *object,
                                                const QString &methodName);
        static bool methodCompat(const QMetaMethod &method1,
                                 const QMetaMethod &method2);
        static QString pluginId(const QString &fileName);
        static QString submoduleId(const QString &fileName,
                                   const QString &pluginId);
        QString convertToAbsolute(const QString &path) const;
        void listPlugins();
};

Q_GLOBAL_STATIC(AkElementPrivate, akElementGlobalStuff)

AkElement::AkElement(QObject *parent):
    QObject(parent)
{
    this->d = new AkElementPrivate();
    this->d->m_state = ElementStateNull;
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
    QStringList subModules;
    auto subModulesPaths = AkElement::listSubModulesPaths(pluginId);

    for (auto &path: subModulesPaths) {
        QPluginLoader pluginLoader(path);
        auto metaData = pluginLoader.metaData();
        auto submoduleId = AkElementPrivate::submoduleId(path, pluginId);

        if (!type.isEmpty()
            && metaData["MetaData"].toObject().contains("type")
            && metaData["MetaData"].toObject()["type"] == type
            && !subModules.contains(submoduleId))
            subModules << submoduleId;
        else if (type.isEmpty()
                 && !subModules.contains(submoduleId))
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

QStringList AkElement::listSubModulesPaths(const QString &pluginId)
{
    auto filePath = AkElement::pluginPath(pluginId);

    if (filePath.isEmpty())
        return QStringList();

    auto pluginDir = QFileInfo(filePath).absoluteDir();

#ifdef Q_OS_ANDROID
    auto submoduleFilePattern =
            QString("lib%1_%2_%3_lib*%4.so").arg(COMMONS_TARGET,
                                                 SUBMODULES_PATH,
                                                 pluginId,
                                                 PLATFORM_TARGET_SUFFIX);
#else
    if (!pluginDir.cd(akElementGlobalStuff->m_subModulesPath
                      + QDir::separator()
                      + pluginId))
        return QStringList();

    auto submoduleFilePattern = akElementGlobalStuff->m_pluginFilePattern;
#endif

    QStringList subModulesPaths;
    auto plugins = pluginDir.entryList({submoduleFilePattern},
                                       QDir::Files
                                       | QDir::AllDirs
                                       | QDir::NoDotAndDotDot,
                                       QDir::Name);

    for (auto &pluginFile: plugins) {
        auto pluginPath = pluginDir.absoluteFilePath(pluginFile);
        QPluginLoader pluginLoader(pluginPath);

        if (!pluginLoader.load())
            continue;

        auto metaData = pluginLoader.metaData();

        if (metaData["MetaData"].toObject().contains("pluginType")
            && metaData["MetaData"].toObject()["pluginType"] == AK_PLUGIN_TYPE_SUBMODULE) {
            subModulesPaths << pluginPath;
        }
    }

    return subModulesPaths;
}

QStringList AkElement::listSubModulesPaths()
{
    QString pluginId;

    if (this->pluginId().isEmpty()) {
        pluginId = this->metaObject()->className();
        pluginId.remove(QRegExp("Element$"));
    } else {
        pluginId = this->pluginId();
    }

    return AkElement::listSubModulesPaths(pluginId);
}

QObject *AkElement::loadSubModule(const QString &pluginId,
                                  const QString &subModule)
{
    auto subModulesPaths = AkElement::listSubModulesPaths(pluginId);

    for (auto &subModulesPath: subModulesPaths) {
        if (AkElementPrivate::submoduleId(subModulesPath,
                                          pluginId) == subModule) {
            QPluginLoader pluginLoader(subModulesPath);

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

            auto obj = plugin->create(AK_PLUGIN_TYPE_SUBMODULE, "");
            delete plugin;

            return obj;
        }
    }

    return nullptr;
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

QString AkElement::subModulesPath()
{
    return akElementGlobalStuff->m_subModulesPath;
}

void AkElement::setSubModulesPath(const QString &subModulesPath)
{
    akElementGlobalStuff->m_subModulesPath = subModulesPath;
}

void AkElement::resetSubModulesPath()
{
    akElementGlobalStuff->m_subModulesPath = SUBMODULES_PATH;
}

QStringList AkElement::listPlugins(const QString &type)
{
    if (!akElementGlobalStuff->m_pluginsScanned)
        akElementGlobalStuff->listPlugins();

    QStringList plugins;

    for (auto &pluginInfo: akElementGlobalStuff->m_pluginsList) {
        auto metaData = pluginInfo.m_metaData["MetaData"].toMap();

        if (!type.isEmpty()
            && metaData.contains("type")
            && metaData["type"] == type
            && !plugins.contains(pluginInfo.m_id))
            plugins << pluginInfo.m_id;
        else if (type.isEmpty() && !plugins.contains(pluginInfo.m_id))
            plugins << pluginInfo.m_id;
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
        if (pluginInfo.m_path.startsWith(searchDir))
            files << pluginInfo.m_path;

    return files;
}

QStringList AkElement::listPluginPaths(bool all)
{
    if (!akElementGlobalStuff->m_pluginsScanned)
        akElementGlobalStuff->listPlugins();

    QStringList files;

    for (auto &pluginInfo: akElementGlobalStuff->m_pluginsList)
        if (!pluginInfo.m_path.isEmpty()
            && !pluginInfo.m_id.isEmpty()
            && (all || pluginInfo.m_used))
            files << pluginInfo.m_path;

    return files;
}

void AkElement::setPluginPaths(const QStringList &paths)
{
    for (auto &path: paths) {
        bool contains = false;

        for (auto &pluginInfo: akElementGlobalStuff->m_pluginsList)
            if (pluginInfo.m_path == path) {
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

    for (auto &path: pluginPaths) {
        auto baseName = QFileInfo(path).baseName();

#ifdef Q_OS_ANDROID
        if (baseName == QString("lib%1_lib%2%3")
                        .arg(COMMONS_TARGET,
                             pluginId,
                             PLATFORM_TARGET_SUFFIX))
            return path;
#else
        if (baseName == QString("%1%2").arg(PREFIX_SHLIB, pluginId))
            return path;
#endif
    }

    return {};
}

QVariantMap AkElement::pluginInfo(const QString &pluginId)
{
    if (!akElementGlobalStuff->m_pluginsScanned)
        akElementGlobalStuff->listPlugins();

    for (auto &pluginInfo: akElementGlobalStuff->m_pluginsList)
        if (pluginInfo.m_id == pluginId)
            return pluginInfo.m_metaData;

    return QVariantMap();
}

void AkElement::setPluginInfo(const QString &path,
                              const QVariantMap &metaData)
{
    for (auto &pluginInfo: akElementGlobalStuff->m_pluginsList)
        if (pluginInfo.m_path == path) {
            pluginInfo.m_metaData = metaData;

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
    this->m_state = AkElement::ElementStateNull;
    this->m_recursiveSearchPaths = false;
    this->m_pluginsScanned = false;
    this->m_defaultPluginsSearchPaths << INSTALLPLUGINSDIR;
#ifdef Q_OS_ANDROID
    this->m_defaultPluginsSearchPaths << QCoreApplication::applicationDirPath();
#else
    this->m_defaultPluginsSearchPaths
            << this->convertToAbsolute(QString("%1/../%2/%3")
                                       .arg(QCoreApplication::applicationDirPath(),
                                            QString(LIBDIR).remove(EXECPREFIX).mid(1),
                                            COMMONS_TARGET));
#endif

#ifdef Q_OS_OSX
    this->m_defaultPluginsSearchPaths
            << this->convertToAbsolute(QString("%1/../Plugins/%2")
                                       .arg(QCoreApplication::applicationDirPath(),
                                            COMMONS_TARGET));
#endif

    this->m_applicationDir.setPath(QCoreApplication::applicationDirPath());
    this->m_subModulesPath = SUBMODULES_PATH;

#ifdef Q_OS_ANDROID
    this->m_pluginFilePattern =
            QString("lib%1_lib*%2.so").arg(COMMONS_TARGET,
                                           PLATFORM_TARGET_SUFFIX);
#else
    this->m_pluginFilePattern = QString("%1*").arg(PREFIX_SHLIB);

    if (strlen(EXTENSION_SHLIB) > 1)
        this->m_pluginFilePattern += "." EXTENSION_SHLIB;

#endif
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

#ifdef Q_OS_ANDROID
    auto pattern = QString("^lib%1_lib(\\w+)%2$")
                   .arg(COMMONS_TARGET, PLATFORM_TARGET_SUFFIX);
#else
    auto pattern = QString("^%1(\\w+)$").arg(PREFIX_SHLIB);
#endif

    QRegularExpression regex(pattern);
    auto match = regex.match(pluginId);

    return match.captured(1);
}

QString AkElementPrivate::submoduleId(const QString &fileName,
                                      const QString &pluginId)
{
#ifdef Q_OS_ANDROID
    auto submoduleId = QFileInfo(fileName).baseName();
    auto pattern = QString("^lib%1_%2_%3_lib(\\w+)%4$")
                   .arg(COMMONS_TARGET,
                        SUBMODULES_PATH,
                        pluginId,
                        PLATFORM_TARGET_SUFFIX);
    QRegularExpression regex(pattern);
    auto match = regex.match(submoduleId);

    return match.captured(1);
#else
    Q_UNUSED(pluginId)

    return AkElementPrivate::pluginId(fileName);
#endif
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
                    if (pluginInfo.m_path == path) {
                        if (pluginInfo.m_metaData.isEmpty()) {
                            QPluginLoader pluginLoader(path);

                            if (pluginLoader.load()) {
                                if (pluginInfo.m_id.isEmpty())
                                    pluginInfo.m_id = pluginId;

                                pluginInfo.m_metaData =
                                        pluginLoader.metaData().toVariantMap();
                                pluginInfo.m_used = true;

                                pluginLoader.unload();
                            }
                        } else {
                            if (pluginInfo.m_id.isEmpty())
                                pluginInfo.m_id = pluginId;

                            pluginInfo.m_used = true;
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

                            if (metaData["MetaData"].toObject().contains("pluginType")
                                && metaData["MetaData"].toObject()["pluginType"] == AK_PLUGIN_TYPE_ELEMENT) {
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
