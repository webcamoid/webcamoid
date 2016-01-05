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

#include <QDir>
#include <QColor>
#include <QCoreApplication>

#include "ak.h"
#include "akvideocaps.h"

QQmlEngine *globalEngine = NULL;
Q_GLOBAL_STATIC(QString, globalQmlPluginPath)
Q_GLOBAL_STATIC(QStringList, globalQmlPluginDefaultPaths)

void Ak::init(QQmlEngine *engine)
{
    qRegisterMetaType<QRgb>("QRgb");
    qRegisterMetaType<QColor>("QColor");

    qRegisterMetaType<AkCaps>("AkCaps");
    qRegisterMetaTypeStreamOperators<AkCaps>("AkCaps");
    qRegisterMetaType<AkVideoCaps>("AkVideoCaps");
    qRegisterMetaTypeStreamOperators<AkVideoCaps>("AkVideoCaps");
    qRegisterMetaType<AkElement::ElementState>("AkElement::ElementState");
    qRegisterMetaType<AkElement::ElementState>("ElementState");
    qRegisterMetaTypeStreamOperators<AkElement::ElementState>("AkElement::ElementState");
    qRegisterMetaType<AkFrac>("AkFrac");
    qRegisterMetaTypeStreamOperators<AkFrac>("AkFrac");
    qRegisterMetaType<AkPacket>("AkPacket");
    qRegisterMetaType<AkElementPtr>("AkElementPtr");

    Ak::setQmlEngine(engine);
}

qint64 Ak::id()
{
    static qint64 id = 0;

    return id++;
}

void Ak::setQmlEngine(QQmlEngine *engine)
{
    if (engine == globalEngine)
        return;

    if (globalEngine) {
        globalEngine->setImportPathList(*globalQmlPluginDefaultPaths);
        globalEngine = NULL;
    }

    if (!engine)
        return;

    globalEngine = engine;
    *globalQmlPluginDefaultPaths = globalEngine->importPathList();

    if (!globalQmlPluginPath->isEmpty()
        && !globalQmlPluginDefaultPaths->contains(*globalQmlPluginPath))
        globalEngine->addImportPath(*globalQmlPluginPath);
}

QString Ak::qmlPluginPath()
{
#ifdef Q_OS_WIN32
    if (globalQmlPluginPath->isEmpty())
        return QString("%1%2qml").arg(QCoreApplication::applicationDirPath()).arg(QDir::separator());
#else
    if (globalQmlPluginPath->isEmpty())
        return QString(QT_INSTALL_QML);
#endif

    return *globalQmlPluginPath;
}

void Ak::setQmlPluginPath(const QString &qmlPluginPath)
{
    if (*globalQmlPluginPath == qmlPluginPath)
        return;

    *globalQmlPluginPath = qmlPluginPath;

    if (globalEngine) {
        if (globalQmlPluginDefaultPaths->isEmpty())
            *globalQmlPluginDefaultPaths = globalEngine->importPathList();
        else
            globalEngine->setImportPathList(*globalQmlPluginDefaultPaths);

        if (!globalQmlPluginPath->isEmpty()
            && !globalQmlPluginDefaultPaths->contains(*globalQmlPluginPath))
            globalEngine->addImportPath(*globalQmlPluginPath);
    }
}

void Ak::resetQmlPluginPath()
{
#ifdef Q_OS_WIN32
    Ak::setQmlPluginPath(QString("%1%2qml").arg(QCoreApplication::applicationDirPath()).arg(QDir::separator()));
#else
    Ak::setQmlPluginPath(QT_INSTALL_QML);
#endif
}
