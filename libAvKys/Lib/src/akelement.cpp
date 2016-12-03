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

#include <QRegExp>
#include <QMetaMethod>
#include <QPluginLoader>
#include <QDirIterator>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDataStream>

#include "ak.h"

#define SUBMODULES_PATH "submodules"

class AkElementPrivate
{
    public:
        QString m_pluginId;
        QString m_pluginPath;
        QString m_pluginFilePattern;
        QStringList m_pluginsSearchPaths;
        QStringList m_pluginsCache;
        QStringList m_pluginsBlackList;
        QString m_subModulesPath;
        QDir m_applicationDir;
        AkElement::ElementState m_state;
        bool m_recursiveSearchPaths;

        AkElementPrivate()
        {
            this->m_recursiveSearchPaths = false;

#ifdef Q_OS_WIN32
            QString defaultPath = QString("%1/../lib/%2")
                                  .arg(QCoreApplication::applicationDirPath())
                                  .arg(COMMONS_TARGET);
#else
            QString defaultPath = QString("%1/%2")
                                  .arg(LIBDIR)
                                  .arg(COMMONS_TARGET);
#endif

            this->m_pluginsSearchPaths << this->convertToAbsolute(defaultPath);
            this->m_applicationDir.setPath(QCoreApplication::applicationDirPath());
            this->m_subModulesPath = SUBMODULES_PATH;

#ifdef Q_OS_OSX
            this->m_pluginFilePattern = "lib*.dylib";
#elif defined(Q_OS_WIN32)
            this->m_pluginFilePattern = "*.dll";
#else
            this->m_pluginFilePattern = "lib*.so";
#endif
        }

        static inline QList<QMetaMethod> methodsByName(const QObject *object,
                                                       const QString &methodName)
        {
            QList<QMetaMethod> methods;
            QStringList methodSignatures;

            for (int i = 0; i < object->metaObject()->methodCount(); i++) {
                QMetaMethod method = object->metaObject()->method(i);
                QString signature(method.methodSignature());

                if (QRegExp(QString("\\s*%1\\s*\\(.*").arg(methodName))
                    .exactMatch(signature))
                    if (!methodSignatures.contains(signature)) {
                        methods << method;
                        methodSignatures << signature;
                    }
            }

            return methods;
        }

        static inline bool methodCompat(const QMetaMethod &method1,
                                        const QMetaMethod &method2)
        {
            if (method1.parameterTypes() == method2.parameterTypes())
                return true;

            return false;
        }

        static inline QString pluginId(const QString &fileName)
        {
            auto pluginId = QFileInfo(fileName).baseName();

#ifdef Q_OS_WIN32
            return pluginId;
                                              ;
#else
            return pluginId.remove(QRegExp("^lib"));
#endif
        }

        inline QString convertToAbsolute(const QString &path) const
        {
            if (!QDir::isRelativePath(path))
                return QDir::cleanPath(path);

            QString absPath = this->m_applicationDir.absoluteFilePath(path);

            return QDir::cleanPath(absPath);
        }
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
    return this->d->m_pluginId;
}

QString AkElement::pluginPath() const
{
    return this->d->m_pluginPath;
}

AkElement::ElementState AkElement::state() const
{
    return this->d->m_state;
}

QObject *AkElement::controlInterface(QQmlEngine *engine,
                                     const QString &controlId) const
{
    Q_UNUSED(engine)
    Q_UNUSED(controlId)

    return NULL;
}

bool AkElement::link(const QObject *dstElement,
                     Qt::ConnectionType connectionType) const
{
    return this->link(this, dstElement, connectionType);
}

bool AkElement::link(const AkElementPtr &dstElement, Qt::ConnectionType connectionType) const
{
    return this->link(static_cast<QObject *>(dstElement.data()), connectionType);
}

bool AkElement::unlink(const QObject *dstElement) const
{
    return this->unlink(this, dstElement);
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

    QList<QMetaMethod> signalList = AkElementPrivate::methodsByName(srcElement, "oStream");
    QList<QMetaMethod> slotList = AkElementPrivate::methodsByName(dstElement, "iStream");

    for (const QMetaMethod &signal: signalList)
        for (const QMetaMethod &slot: slotList)
            if (AkElementPrivate::methodCompat(signal, slot) &&
                signal.methodType() == QMetaMethod::Signal &&
                slot.methodType() == QMetaMethod::Slot)
                QObject::connect(srcElement, signal, dstElement, slot, connectionType);

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

    for (const QMetaMethod &signal: AkElementPrivate::methodsByName(srcElement, "oStream"))
        for (const QMetaMethod &slot: AkElementPrivate::methodsByName(dstElement, "iStream"))
            if (AkElementPrivate::methodCompat(signal, slot) &&
                signal.methodType() == QMetaMethod::Signal &&
                slot.methodType() == QMetaMethod::Slot)
                QObject::disconnect(srcElement, signal, dstElement, slot);

    return true;
}

AkElementPtr AkElement::create(const QString &pluginId,
                               const QString &elementName)
{
    auto element = AkElement::createPtr(pluginId, elementName);

    if (!element)
        return AkElementPtr();

    return AkElementPtr(element);
}

AkElement *AkElement::createPtr(const QString &pluginId, const QString &elementName)
{
    QString filePath = AkElement::pluginPath(pluginId);

    if (filePath.isEmpty())
        return NULL;

    QPluginLoader pluginLoader(filePath);

    if (!pluginLoader.load()) {
        qDebug() << "Error loading plugin " << pluginId << ":" << pluginLoader.errorString();

        return NULL;
    }

    auto plugin = qobject_cast<AkPlugin *>(pluginLoader.instance());

    if (!plugin)
        return NULL;

    auto element = qobject_cast<AkElement *>(plugin->create(AK_PLUGIN_TYPE_ELEMENT, ""));
    delete plugin;

    if (!element)
        return NULL;

    if (!elementName.isEmpty())
        element->setObjectName(elementName);

    element->d->m_pluginId = pluginId;
    element->d->m_pluginPath = filePath;

    return element;
}

QStringList AkElement::listSubModules(const QString &pluginId,
                                      const QString &type)
{
    QStringList subModules;
    auto subModulesPaths = AkElement::listSubModulesPaths(pluginId);

    for (const QString &path: subModulesPaths) {
        QPluginLoader pluginLoader(path);
        QJsonObject metaData = pluginLoader.metaData();
        QString pluginId = AkElementPrivate::pluginId(path);

        if (!type.isEmpty()
            && metaData["MetaData"].toObject().contains("type")
            && metaData["MetaData"].toObject()["type"] == type
            && !subModules.contains(pluginId))
            subModules << pluginId;
        else if (type.isEmpty()
                 && !subModules.contains(pluginId))
            subModules << pluginId;
    }

    subModules.sort();

    return subModules;
}

QStringList AkElement::listSubModules(const QStringList &types)
{
    if (types.isEmpty())
        return AkElement::listSubModules(this->d->m_pluginId);

    QStringList subModules;

    for (const QString &type: types)
        subModules << AkElement::listSubModules(this->d->m_pluginId, type);

    return subModules;
}

QStringList AkElement::listSubModulesPaths(const QString &pluginId)
{
    auto filePath = AkElement::pluginPath(pluginId);

    if (filePath.isEmpty())
        return QStringList();

    auto pluginDir = QFileInfo(filePath).absoluteDir();

    if (!pluginDir.cd(akElementGlobalStuff->m_subModulesPath
                      + QDir::separator()
                      + pluginId))
        return QStringList();

    QStringList subModulesPaths;
    auto plugins = pluginDir.entryList({akElementGlobalStuff->m_pluginFilePattern},
                                       QDir::Files
                                       | QDir::AllDirs
                                       | QDir::NoDotAndDotDot,
                                       QDir::Name);

    for (const QString &pluginFile: plugins) {
        auto pluginPath = pluginDir.absoluteFilePath(pluginFile);
        QPluginLoader pluginLoader(pluginPath);

        if (!pluginLoader.load())
            continue;

        if (auto plugin = qobject_cast<AkPlugin *>(pluginLoader.instance())) {
            auto metaData = pluginLoader.metaData();

            if (metaData["MetaData"].toObject().contains("pluginType")
                && metaData["MetaData"].toObject()["pluginType"] == AK_PLUGIN_TYPE_SUBMODULE) {
                subModulesPaths << pluginPath;
            }

            delete plugin;
        }
    }

    return subModulesPaths;
}

QStringList AkElement::listSubModulesPaths()
{
    return AkElement::listSubModulesPaths(this->d->m_pluginId);
}

QObject *AkElement::loadSubModule(const QString &pluginId,
                                  const QString &subModule)
{
    auto subModulesPaths = AkElement::listSubModulesPaths(pluginId);

    for (const QString &subModulesPath: subModulesPaths) {
        if (AkElementPrivate::pluginId(subModulesPath) == subModule) {
            QPluginLoader pluginLoader(subModulesPath);

            if (!pluginLoader.load()) {
                qDebug() << QString("Error loading submodule '%1' for '%2' plugin: %3")
                                .arg(subModule)
                                .arg(pluginId)
                                .arg(pluginLoader.errorString());

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
    return AkElement::loadSubModule(this->d->m_pluginId, subModule);
}

bool AkElement::recursiveSearch()
{
    return akElementGlobalStuff->m_recursiveSearchPaths;
}

void AkElement::setRecursiveSearch(bool enable)
{
    akElementGlobalStuff->m_recursiveSearchPaths = enable;
}

QStringList AkElement::searchPaths(SearchPaths pathType)
{
    if (pathType == SearchPathsAll)
        return akElementGlobalStuff->m_pluginsSearchPaths;

#ifdef Q_OS_WIN32
    QString defaultPath = QString("%1/../lib/%2")
                          .arg(QCoreApplication::applicationDirPath())
                          .arg(COMMONS_TARGET);
#else
    QString defaultPath = QString("%1/%2")
                          .arg(LIBDIR)
                          .arg(COMMONS_TARGET);
#endif

    QStringList defaults;
    defaults << akElementGlobalStuff->convertToAbsolute(defaultPath);

    if (pathType == SearchPathsDefaults)
        return defaults;

    QStringList extras = akElementGlobalStuff->m_pluginsSearchPaths;

    for (const QString &path: defaults)
        extras.removeAll(path);

    return extras;
}

void AkElement::addSearchPath(const QString &path)
{
    if (!path.isEmpty() && QDir(path).exists())
        akElementGlobalStuff->m_pluginsSearchPaths << path;
}

void AkElement::setSearchPaths(const QStringList &searchPaths)
{
    akElementGlobalStuff->m_pluginsSearchPaths.clear();

    for (const QString &path: searchPaths)
        if (QDir(path).exists())
            akElementGlobalStuff->m_pluginsSearchPaths << path;
}

void AkElement::resetSearchPaths()
{
    akElementGlobalStuff->m_pluginsSearchPaths.clear();

#ifdef Q_OS_WIN32
    QString defaultPath = QString("%1/../lib/%2")
                          .arg(QCoreApplication::applicationDirPath())
                          .arg(COMMONS_TARGET);
#else
    QString defaultPath = QString("%1/%2")
                          .arg(LIBDIR)
                          .arg(COMMONS_TARGET);
#endif

    akElementGlobalStuff->m_pluginsSearchPaths
            << akElementGlobalStuff->convertToAbsolute(defaultPath);
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
    QStringList plugins;
    QStringList pluginPaths = AkElement::listPluginPaths();

    for (const QString &path: pluginPaths) {
        QPluginLoader pluginLoader(path);
        QJsonObject metaData = pluginLoader.metaData();
        QString pluginId = AkElementPrivate::pluginId(path);

        if (!type.isEmpty()
            && metaData["MetaData"].toObject().contains("type")
            && metaData["MetaData"].toObject()["type"] == type
            && !plugins.contains(pluginId))
            plugins << pluginId;
        else if (type.isEmpty()
                 && !plugins.contains(pluginId))
            plugins << pluginId;
    }

    plugins.sort();

    return plugins;
}

QStringList AkElement::listPluginPaths(const QString &searchPath)
{
    QString searchDir(searchPath);

    searchDir.replace(QRegExp("((\\\\/?)|(/\\\\?))+"),
                                  QDir::separator());

    while (searchDir.endsWith(QDir::separator()))
        searchDir.resize(searchDir.size() - 1);

    QStringList searchPaths(searchDir);
    QStringList files;

    while (!searchPaths.isEmpty()) {
        QString path = searchPaths.takeFirst();

        if (akElementGlobalStuff->m_pluginsBlackList.contains(path))
            continue;

        if (QFileInfo(path).isFile()) {
            QString fileName = QFileInfo(path).fileName();

            if (QRegExp(akElementGlobalStuff->m_pluginFilePattern,
                        Qt::CaseSensitive,
                        QRegExp::Wildcard).exactMatch(fileName)) {
                QPluginLoader pluginLoader(path);

                if (pluginLoader.load()) {
                    auto plugin = qobject_cast<AkPlugin *>(pluginLoader.instance());

                    if (plugin) {
                        auto metaData = pluginLoader.metaData();

                        if (metaData["MetaData"].toObject().contains("pluginType")
                            && metaData["MetaData"].toObject()["pluginType"] == AK_PLUGIN_TYPE_ELEMENT) {
                            files << path;
                        }

                        delete plugin;
                    }

                    pluginLoader.unload();
                }
            }
        } else {
            QDir dir(path);
            auto fileList = dir.entryList({akElementGlobalStuff->m_pluginFilePattern},
                                          QDir::Files
                                          | QDir::CaseSensitive,
                                          QDir::Name);

            for (const QString &file: fileList)
                searchPaths << dir.absoluteFilePath(file);

            if (akElementGlobalStuff->m_recursiveSearchPaths) {
                auto dirList = dir.entryList(QDir::Dirs
                                             | QDir::NoDotAndDotDot,
                                             QDir::Name);

                for (const QString &path: dirList)
                    searchPaths << dir.absoluteFilePath(path);
            }
        }
    }

    return files;
}

QStringList AkElement::listPluginPaths()
{
    if (!akElementGlobalStuff->m_pluginsCache.isEmpty())
        return akElementGlobalStuff->m_pluginsCache;

    QStringList searchPaths;

    for (int i = akElementGlobalStuff->m_pluginsSearchPaths.length() - 1; i >= 0; i--) {
        QStringList paths = AkElement::listPluginPaths(akElementGlobalStuff->m_pluginsSearchPaths[i]);

        for (const QString &path: paths)
            if (!searchPaths.contains(path))
                searchPaths << path;
    }

    akElementGlobalStuff->m_pluginsCache = searchPaths;

    return searchPaths;
}

QStringList AkElement::pluginsCache()
{
    return akElementGlobalStuff->m_pluginsCache;
}

void AkElement::setPluginsCache(const QStringList &paths)
{
    akElementGlobalStuff->m_pluginsCache = paths;
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
    QStringList pluginPaths = AkElement::listPluginPaths();

    for (const QString &path: pluginPaths) {
        QString baseName = QFileInfo(path).baseName();

#ifdef Q_OS_WIN32
        if (baseName == pluginId)
            return path;
#else
        if (baseName == QString("lib%1").arg(pluginId))
            return path;
#endif
    }

    return QString();
}

QVariantMap AkElement::pluginInfo(const QString &pluginId)
{
    QString filePath = AkElement::pluginPath(pluginId);

    if (filePath.isEmpty())
        return QVariantMap();

    QPluginLoader pluginLoader(filePath);

    return pluginLoader.metaData().toVariantMap();
}

void AkElement::clearCache()
{
    akElementGlobalStuff->m_pluginsCache.clear();
}

AkPacket AkElement::operator ()(const AkPacket &packet)
{
    return this->iStream(packet);
}

AkPacket AkElement::operator ()(const AkAudioPacket &packet)
{
    return this->iStream(packet);
}

AkPacket AkElement::operator ()(const AkVideoPacket &packet)
{
    return this->iStream(packet);
}

void AkElement::stateChange(AkElement::ElementState from, AkElement::ElementState to)
{
    Q_UNUSED(from)
    Q_UNUSED(to)
}

AkPacket AkElement::iStream(const AkPacket &packet)
{
    if (packet.caps().mimeType() == "audio/x-raw")
        return this->iStream(AkAudioPacket(packet));
    else if (packet.caps().mimeType() == "video/x-raw")
        return this->iStream(AkVideoPacket(packet));

    return AkPacket();
}

AkPacket AkElement::iStream(const AkAudioPacket &packet)
{
    Q_UNUSED(packet)

    return AkPacket();
}

AkPacket AkElement::iStream(const AkVideoPacket &packet)
{
    Q_UNUSED(packet)

    return AkPacket();
}

bool AkElement::setState(AkElement::ElementState state)
{
    if (this->d->m_state == state)
        return false;

    ElementState preState = this->d->m_state;
    this->d->m_state = state;

    switch (preState) {
    case ElementStateNull: {
        switch (state) {
        case ElementStatePaused:
            emit this->stateChanged(state);
            emit this->stateChange(preState, state);

            break;
        case ElementStatePlaying:
            emit this->stateChanged(ElementStatePaused);
            emit this->stateChange(preState, ElementStatePaused);

            emit this->stateChanged(state);
            emit this->stateChange(ElementStatePaused, state);

            break;
        case ElementStateNull:
            break;
        }

        break;
    }
    case ElementStatePaused: {
        switch (state) {
        case ElementStateNull:
        case ElementStatePlaying:
            emit this->stateChanged(state);
            emit this->stateChange(preState, state);

            break;
        case ElementStatePaused:
            break;
        }

        break;
    }
    case ElementStatePlaying: {
        switch (state) {
        case ElementStateNull:
            emit this->stateChanged(ElementStatePaused);
            emit this->stateChange(preState, ElementStatePaused);

            emit this->stateChanged(state);
            emit this->stateChange(ElementStatePaused, state);

            break;
        case ElementStatePaused:
            emit this->stateChanged(state);
            emit this->stateChange(preState, state);

            break;
        case ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return true;
}

void AkElement::resetState()
{
    this->setState(ElementStateNull);
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
