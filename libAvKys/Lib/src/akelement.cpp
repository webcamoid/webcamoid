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

class AkElementPrivate
{
    public:
        QString m_pluginId;
        AkElement::ElementState m_state;
        QStringList m_pluginsSearchPaths;
        bool m_recursiveSearchPaths;
        QStringList m_pluginsCache;
        QDir m_applicationDir;

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
    if (!dstElement)
        return false;

    QList<QMetaMethod> signalList = AkElementPrivate::methodsByName(this, "oStream");
    QList<QMetaMethod> slotList = AkElementPrivate::methodsByName(dstElement, "iStream");

    foreach (QMetaMethod signal, signalList)
        foreach (QMetaMethod slot, slotList)
            if (AkElementPrivate::methodCompat(signal, slot) &&
                signal.methodType() == QMetaMethod::Signal &&
                slot.methodType() == QMetaMethod::Slot)
                QObject::connect(this, signal, dstElement, slot, connectionType);

    return true;
}

bool AkElement::link(const AkElementPtr &dstElement, Qt::ConnectionType connectionType) const
{
    return this->link(static_cast<QObject *>(dstElement.data()), connectionType);
}

bool AkElement::unlink(const QObject *dstElement) const
{
    if (!dstElement)
        return false;

    foreach (QMetaMethod signal, AkElementPrivate::methodsByName(this, "oStream"))
        foreach (QMetaMethod slot, AkElementPrivate::methodsByName(dstElement, "iStream"))
            if (AkElementPrivate::methodCompat(signal, slot) &&
                signal.methodType() == QMetaMethod::Signal &&
                slot.methodType() == QMetaMethod::Slot)
                QObject::disconnect(this, signal, dstElement, slot);

    return true;
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

AkElementPtr AkElement::create(const QString &pluginId,
                               const QString &elementName)
{
    QString filePath = AkElement::pluginPath(pluginId);

    if (filePath.isEmpty())
        return AkElementPtr();

    QPluginLoader pluginLoader(filePath);

    if (!pluginLoader.load()) {
        qDebug() << "Error loading plugin " << pluginId << ":" << pluginLoader.errorString();

        return AkElementPtr();
    }

    AkPlugin *plugin = qobject_cast<AkPlugin *>(pluginLoader.instance());
    AkElement *element = qobject_cast<AkElement *>(plugin->create("", ""));
    delete plugin;

    if (!element)
        return AkElementPtr();

    if (!elementName.isEmpty())
        element->setObjectName(elementName);

    element->d->m_pluginId = pluginId;

    return AkElementPtr(element);
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

    foreach (QString path, defaults)
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

    foreach (QString path, searchPaths)
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

QStringList AkElement::listPlugins(const QString &type)
{
    QStringList plugins;
    QStringList pluginPaths = AkElement::listPluginPaths();

    foreach (QString path, pluginPaths) {
        QPluginLoader pluginLoader(path);
        QJsonObject metaData = pluginLoader.metaData();

#ifdef Q_OS_WIN32
        QString pluginId = QFileInfo(path).baseName();
#else
        QString pluginId = QFileInfo(path).baseName()
                                          .remove(QRegExp("^lib"));
#endif

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

    searchDir = searchDir.replace(QRegExp("((\\\\/?)|(/\\\\?))+"),
                                  QDir::separator());

    while (searchDir.endsWith(QDir::separator()))
        searchDir.resize(searchDir.size() - 1);

    QStringList searchPaths(searchDir);
    QStringList files;

#ifdef Q_OS_WIN32
    QString pattern("*.dll");
#else
    QString pattern("lib*.so");
#endif

    while (!searchPaths.isEmpty()) {
        QString path = searchPaths.takeFirst();

        if (QFileInfo(path).isFile()) {
            QString fileName = QFileInfo(path).fileName();

            if (QRegExp(pattern,
                        Qt::CaseSensitive,
                        QRegExp::Wildcard).exactMatch(fileName)) {
                QPluginLoader pluginLoader(path);

                if (pluginLoader.load()) {
                    pluginLoader.unload();
                    files << path;
                }
            }
        } else {
            QDir dir(path);
            QStringList fileList = dir.entryList(QDir::Files,
                                                 QDir::Name);

            foreach (QString file, fileList)
                if (QRegExp(pattern,
                            Qt::CaseSensitive,
                            QRegExp::Wildcard).exactMatch(file)) {
                    QString pluginPath = QString("%1%2%3").arg(path)
                                                          .arg(QDir::separator())
                                                          .arg(file);
                    QPluginLoader pluginLoader(pluginPath);

                    if (pluginLoader.load()) {
                        pluginLoader.unload();
                        files << pluginPath;
                    }
                }

            if (!akElementGlobalStuff->m_recursiveSearchPaths)
                break;

            QStringList dirList = dir.entryList(QDir::Dirs
                                                | QDir::NoDotAndDotDot,
                                                QDir::Name);

            foreach (QString dir, dirList)
                searchPaths << QString("%1%2%3").arg(path)
                                                .arg(QDir::separator())
                                                .arg(dir);
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

        foreach (QString path, paths)
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

QString AkElement::pluginPath(const QString &pluginId)
{
    QStringList pluginPaths = AkElement::listPluginPaths();

    foreach (QString path, pluginPaths) {
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
