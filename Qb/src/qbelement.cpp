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
#include <QDir>
#include <QFileInfo>

#include "qb.h"

Q_GLOBAL_STATIC_WITH_ARGS(QStringList,
                          pluginsSearchPaths,
                          (QString("%1/%2").arg(LIBDIR)
                                           .arg(COMMONS_TARGET)))

QbElement::QbElement(QObject *parent): QObject(parent)
{
    this->m_state = ElementStateNull;
}

QbElement::~QbElement()
{
    this->setState(QbElement::ElementStateNull);
}

QString QbElement::pluginId() const
{
    return this->m_pluginId;
}

QbElement::ElementState QbElement::state() const
{
    return this->m_state;
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

    QList<QMetaMethod> signalList = QbElement::methodsByName(this, "oStream");
    QList<QMetaMethod> slotList = QbElement::methodsByName(dstElement, "iStream");

    foreach (QMetaMethod signal, signalList)
        foreach (QMetaMethod slot, slotList)
            if (QbElement::methodCompat(signal, slot) &&
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

    foreach (QMetaMethod signal, QbElement::methodsByName(this, "oStream"))
        foreach (QMetaMethod slot, QbElement::methodsByName(dstElement, "iStream"))
            if (QbElement::methodCompat(signal, slot) &&
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

QbElementPtr QbElement::create(const QString &pluginId, const QString &elementName)
{
    QString fileName;

    for (int i = pluginsSearchPaths->length() - 1; i >= 0; i--) {
        QString path = pluginsSearchPaths->at(i);
        QString filePath = QString("%1%2lib%3.so").arg(path)
                                                  .arg(QDir::separator())
                                                  .arg(pluginId);

        if (QFileInfo(filePath).exists()) {
            fileName = filePath;

            break;
        }
    }

    if (fileName.isEmpty() || !QLibrary::isLibrary(fileName))
        return QbElementPtr();

    QPluginLoader pluginLoader(fileName);

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

    element->m_pluginId = pluginId;

    return QbElementPtr(element);
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

    *pluginsSearchPaths << QString("%1/%2").arg(LIBDIR)
                           .arg(COMMONS_TARGET);
}

QStringList QbElement::listPlugins(const QString &type)
{
    QStringList plugins;

    for (int i = pluginsSearchPaths->length() - 1; i >= 0; i--) {
        QString path = pluginsSearchPaths->at(i);
        QDir devicesDir(path);

        QStringList devices = devicesDir.entryList(QStringList() << "lib*.so",
                                                   QDir::Files
                                                   | QDir::NoSymLinks
                                                   | QDir::NoDotAndDotDot
                                                   | QDir::CaseSensitive,
                                                   QDir::Name);

        foreach (QString devicePath, devices) {
            QString fileName = devicesDir.absoluteFilePath(devicePath);

            if (!QLibrary::isLibrary(fileName))
                continue;

            QPluginLoader pluginLoader(fileName);
            QJsonObject metaData = pluginLoader.metaData();

            QString pluginId = QString(devicePath).remove(QRegExp("^lib"))
                                                  .remove(QRegExp(".so$"));

            if (!type.isEmpty()
                && metaData["MetaData"].toObject().contains("type")
                && metaData["MetaData"].toObject()["type"] == type
                && !plugins.contains(pluginId))
                plugins << pluginId;
            else if (type.isEmpty()
                     && !plugins.contains(pluginId))
                plugins << pluginId;
        }
    }

    plugins.sort();

    return plugins;
}

QVariantMap QbElement::pluginInfo(const QString &pluginId)
{
    QString fileName;

    for (int i = pluginsSearchPaths->length() - 1; i >= 0; i--) {
        QString path = pluginsSearchPaths->at(i);
        QString filePath = QString("%1%2lib%3.so").arg(path)
                                                  .arg(QDir::separator())
                                                  .arg(pluginId);

        if (QFileInfo(filePath).exists()) {
            fileName = filePath;

            break;
        }
    }

    if (fileName.isEmpty() || !QLibrary::isLibrary(fileName))
        return QVariantMap();

    QPluginLoader pluginLoader(fileName);

    return pluginLoader.metaData().toVariantMap();
}

QList<QMetaMethod> QbElement::methodsByName(const QObject *object, const QString &methodName)
{
    QList<QMetaMethod> methods;
    QStringList methodSignatures;

    for (int i = 0; i < object->metaObject()->methodCount(); i++) {
        QMetaMethod method = object->metaObject()->method(i);
        QString signature(method.methodSignature());

        if (QRegExp(QString("\\s*%1\\s*\\(.*").arg(methodName)).exactMatch(signature))
            if (!methodSignatures.contains(signature)) {
                methods << method;
                methodSignatures << signature;
            }
    }

    return methods;
}

bool QbElement::methodCompat(const QMetaMethod &method1, const QMetaMethod &method2)
{
    if (method1.parameterTypes() == method2.parameterTypes())
        return true;

    return false;
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
    QbElement::ElementState preState = this->m_state;

    if (state == ElementStateNull
        && preState != state) {
        if (preState == ElementStatePlaying) {
            this->setState(ElementStatePaused);
            preState = this->m_state;
        }

        this->m_state = state;
        emit this->stateChanged(state);
        emit this->stateChange(preState, state);
    }
    else if (state == ElementStatePaused
             && preState != state) {
        this->m_state = state;
        emit this->stateChanged(state);
        emit this->stateChange(preState, state);
    }
    else if (state == ElementStatePlaying
             && preState != state) {
        if (preState == ElementStateNull) {
            this->setState(ElementStatePaused);
            preState = this->m_state;
        }

        this->m_state = state;
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
