/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include <QRegExp>
#include <QMetaMethod>
#include <QPluginLoader>
#include <QDirIterator>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

#include "qb.h"

#ifdef Q_OS_WIN32
Q_GLOBAL_STATIC_WITH_ARGS(QStringList,
                          pluginsSearchPaths,
                          (QString("%1/Qb/Plugins").arg(QCoreApplication::applicationDirPath())))
#else
Q_GLOBAL_STATIC_WITH_ARGS(QStringList,
                          pluginsSearchPaths,
                          (QString("%1/%2").arg(LIBDIR)
                                           .arg(COMMONS_TARGET)))
#endif

Q_GLOBAL_STATIC_WITH_ARGS(bool, recursiveSearchPaths, (false))

Q_GLOBAL_STATIC(QStringList, pluginsCache)

class QbElementPrivate
{
    public:
        QString m_pluginId;
        QbElement::ElementState m_state;

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

QbElement::QbElement(QObject *parent):
    QObject(parent)
{
    this->d = new QbElementPrivate();
    this->d->m_state = ElementStateNull;
}

QbElement::~QbElement()
{
    this->setState(QbElement::ElementStateNull);
    delete this->d;
}

QString QbElement::pluginId() const
{
    return this->d->m_pluginId;
}

QbElement::ElementState QbElement::state() const
{
    return this->d->m_state;
}

QObject *QbElement::controlInterface(QQmlEngine *engine,
                                     const QString &controlId) const
{
    Q_UNUSED(engine)
    Q_UNUSED(controlId)

    return NULL;
}

bool QbElement::link(const QObject *dstElement,
                     Qt::ConnectionType connectionType) const
{
    if (!dstElement)
        return false;

    QList<QMetaMethod> signalList = QbElementPrivate::methodsByName(this, "oStream");
    QList<QMetaMethod> slotList = QbElementPrivate::methodsByName(dstElement, "iStream");

    foreach (QMetaMethod signal, signalList)
        foreach (QMetaMethod slot, slotList)
            if (QbElementPrivate::methodCompat(signal, slot) &&
                signal.methodType() == QMetaMethod::Signal &&
                slot.methodType() == QMetaMethod::Slot)
                QObject::connect(this, signal, dstElement, slot, connectionType);

    return true;
}

bool QbElement::link(const QbElementPtr &dstElement, Qt::ConnectionType connectionType) const
{
    return this->link(static_cast<QObject *>(dstElement.data()), connectionType);
}

bool QbElement::unlink(const QObject *dstElement) const
{
    if (!dstElement)
        return false;

    foreach (QMetaMethod signal, QbElementPrivate::methodsByName(this, "oStream"))
        foreach (QMetaMethod slot, QbElementPrivate::methodsByName(dstElement, "iStream"))
            if (QbElementPrivate::methodCompat(signal, slot) &&
                signal.methodType() == QMetaMethod::Signal &&
                slot.methodType() == QMetaMethod::Slot)
                QObject::disconnect(this, signal, dstElement, slot);

    return true;
}

bool QbElement::unlink(const QbElementPtr &dstElement) const
{
    return this->unlink(static_cast<QObject *>(dstElement.data()));
}

bool QbElement::link(const QbElementPtr &srcElement,
                     const QObject *dstElement,
                     Qt::ConnectionType connectionType)
{
    return srcElement->link(dstElement, connectionType);
}

bool QbElement::link(const QbElementPtr &srcElement,
                     const QbElementPtr &dstElement,
                     Qt::ConnectionType connectionType)
{
    return srcElement->link(dstElement, connectionType);
}

bool QbElement::unlink(const QbElementPtr &srcElement,
                       const QObject *dstElement)
{
    return srcElement->unlink(dstElement);
}

bool QbElement::unlink(const QbElementPtr &srcElement,
                       const QbElementPtr &dstElement)
{
    return srcElement->unlink(dstElement);
}

QbElementPtr QbElement::create(const QString &pluginId,
                               const QString &elementName)
{
    QString filePath = QbElement::pluginPath(pluginId);

    if (filePath.isEmpty())
        return QbElementPtr();

    QPluginLoader pluginLoader(filePath);

    if (!pluginLoader.load()) {
        qDebug() << pluginLoader.errorString();

        return QbElementPtr();
    }

    QbPlugin *plugin = qobject_cast<QbPlugin *>(pluginLoader.instance());
    QbElement *element = qobject_cast<QbElement *>(plugin->create("", ""));
    delete plugin;

    if (!element)
        return QbElementPtr();

    if (!elementName.isEmpty())
        element->setObjectName(elementName);

    element->d->m_pluginId = pluginId;

    return QbElementPtr(element);
}

bool QbElement::recursiveSearch()
{
    return *recursiveSearchPaths;
}

void QbElement::setRecursiveSearch(bool enable)
{
    *recursiveSearchPaths = enable;
}

QStringList QbElement::searchPaths()
{
    return *pluginsSearchPaths;
}

void QbElement::addSearchPath(const QString &path)
{
    pluginsSearchPaths->insert(0, path);
}

void QbElement::setSearchPaths(const QStringList &searchPaths)
{
    *pluginsSearchPaths = searchPaths;
}

void QbElement::resetSearchPaths()
{
    pluginsSearchPaths->clear();

#ifdef Q_OS_WIN32
    *pluginsSearchPaths << QString("%1/Qb/Plugins").arg(QCoreApplication::applicationDirPath());
#else
    *pluginsSearchPaths << QString("%1/%2").arg(LIBDIR)
                           .arg(COMMONS_TARGET);
#endif
}

QStringList QbElement::listPlugins(const QString &type)
{
    QStringList plugins;
    QStringList pluginPaths = QbElement::listPluginPaths();

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

QStringList QbElement::listPluginPaths(const QString &searchPath)
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

QStringList QbElement::listPluginPaths()
{
    if (!pluginsCache->isEmpty())
        return *pluginsCache;

    QStringList searchPaths;

    for (int i = pluginsSearchPaths->length() - 1; i >= 0; i--) {
        QStringList paths = QbElement::listPluginPaths(pluginsSearchPaths->at(i));

        if (!paths.isEmpty())
            searchPaths << paths;
    }

    *pluginsCache = searchPaths;

    return searchPaths;
}

QString QbElement::pluginPath(const QString &pluginId)
{
    QStringList pluginPaths = QbElement::listPluginPaths();

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

QVariantMap QbElement::pluginInfo(const QString &pluginId)
{
    QString filePath = QbElement::pluginPath(pluginId);

    if (filePath.isEmpty())
        return QVariantMap();

    QPluginLoader pluginLoader(filePath);

    return pluginLoader.metaData().toVariantMap();
}

void QbElement::clearCache()
{
    pluginsCache->clear();
}

void QbElement::stateChange(QbElement::ElementState from, QbElement::ElementState to)
{
    Q_UNUSED(from)
    Q_UNUSED(to)
}

QbPacket QbElement::iStream(const QbPacket &packet)
{
    Q_UNUSED(packet)

    return QbPacket();
}

void QbElement::setState(QbElement::ElementState state)
{
    QbElement::ElementState preState = this->d->m_state;

    if (state == ElementStateNull
        && preState != state) {
        if (preState == ElementStatePlaying) {
            this->setState(ElementStatePaused);
            preState = this->d->m_state;
        }

        this->d->m_state = state;
        emit this->stateChanged(state);
        emit this->stateChange(preState, state);
    } else if (state == ElementStatePaused
               && preState != state) {
        this->d->m_state = state;
        emit this->stateChanged(state);
        emit this->stateChange(preState, state);
    } else if (state == ElementStatePlaying
               && preState != state) {
        if (preState == ElementStateNull) {
            this->setState(ElementStatePaused);
            preState = this->d->m_state;
        }

        this->d->m_state = state;
        emit this->stateChanged(state);
        emit this->stateChange(preState, state);
    }
}

void QbElement::resetState()
{
    this->setState(ElementStateNull);
}

QDataStream &operator >>(QDataStream &istream, QbElement::ElementState &state)
{
    int stateInt;
    istream >> stateInt;
    state = static_cast<QbElement::ElementState>(stateInt);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, QbElement::ElementState state)
{
    ostream << static_cast<int>(state);

    return ostream;
}
