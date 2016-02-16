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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
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

inline QStringList initPluginsSearchPaths()
{
    QStringList defaultPath;

#ifdef Q_OS_WIN32
    defaultPath << QCoreApplication::applicationDirPath()
                << COMMONS_TARGET
                << "Plugins";
#else
    defaultPath << LIBDIR
                << COMMONS_TARGET;
#endif

    QStringList pluginsSearchPaths;
    pluginsSearchPaths << defaultPath.join(QDir::separator());

    return pluginsSearchPaths;
}

Q_GLOBAL_STATIC_WITH_ARGS(QStringList,
                          pluginsSearchPaths,
                          (initPluginsSearchPaths()))

Q_GLOBAL_STATIC_WITH_ARGS(bool, recursiveSearchPaths, (false))

Q_GLOBAL_STATIC(QStringList, pluginsCache)

class AkElementPrivate
{
    public:
        QString m_pluginId;
        AkElement::ElementState m_state;

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
};

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
    return *recursiveSearchPaths;
}

void AkElement::setRecursiveSearch(bool enable)
{
    *recursiveSearchPaths = enable;
}

QStringList AkElement::searchPaths(SearchPaths pathType)
{
    if (pathType == SearchPathsAll)
        return *pluginsSearchPaths;

    QStringList defaults;
    QStringList defaultPath;

#ifdef Q_OS_WIN32
    defaultPath << QCoreApplication::applicationDirPath()
                << COMMONS_TARGET
                << "Plugins";
#else
    defaultPath << LIBDIR
                << COMMONS_TARGET;
#endif

    defaults << defaultPath.join(QDir::separator());

    if (pathType == SearchPathsDefaults)
        return defaults;

    QStringList extras = *pluginsSearchPaths;

    foreach (QString path, defaults)
        extras.removeAll(path);

    return extras;
}

void AkElement::addSearchPath(const QString &path)
{
    if (!path.isEmpty() && QDir(path).exists())
        *pluginsSearchPaths << path;
}

void AkElement::setSearchPaths(const QStringList &searchPaths)
{
    pluginsSearchPaths->clear();

    foreach (QString path, searchPaths)
        if (QDir(path).exists())
            *pluginsSearchPaths << path;
}

void AkElement::resetSearchPaths()
{
    pluginsSearchPaths->clear();

    QStringList defaultPath;

#ifdef Q_OS_WIN32
    defaultPath << QCoreApplication::applicationDirPath()
                << COMMONS_TARGET
                << "Plugins";
#else
    defaultPath << LIBDIR
                << COMMONS_TARGET;
#endif

    *pluginsSearchPaths << defaultPath.join(QDir::separator());
}

QStringList AkElement::listPlugins(const QString &type)
{
    QStringList plugins;
    QStringList pluginPaths = AkElement::listPluginPaths();

    foreach (QString path, pluginPaths) {
        QPluginLoader pluginLoader(path);
        QJsonObject metaData = pluginLoader.metaData();

#ifdef Q_OS_WIN32
        QString pluginId = QFileInfo(path).baseName()
                                          .remove(QRegExp(QString(COMMONS_VER_MAJ) + "$"));
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
    QString pattern(QString("*%1.dll").arg(COMMONS_VER_MAJ));
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

            if (!*recursiveSearchPaths)
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
    if (!pluginsCache->isEmpty())
        return *pluginsCache;

    QStringList searchPaths;

    for (int i = pluginsSearchPaths->length() - 1; i >= 0; i--) {
        QStringList paths = AkElement::listPluginPaths(pluginsSearchPaths->at(i));

        if (!paths.isEmpty())
            searchPaths << paths;
    }

    *pluginsCache = searchPaths;

    return searchPaths;
}

QString AkElement::pluginPath(const QString &pluginId)
{
    QStringList pluginPaths = AkElement::listPluginPaths();

    foreach (QString path, pluginPaths) {
        QString baseName = QFileInfo(path).baseName();

#ifdef Q_OS_WIN32
        if (baseName == QString("%1%2").arg(pluginId).arg(COMMONS_VER_MAJ))
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
    pluginsCache->clear();
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
        default:
            break;
        }
    }
    case ElementStatePaused: {
        switch (state) {
        case ElementStateNull:
        case ElementStatePlaying:
            emit this->stateChanged(state);
            emit this->stateChange(preState, state);

            break;
        default:
            break;
        }
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
        default:
            break;
        }
    }
    default:
        break;
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
