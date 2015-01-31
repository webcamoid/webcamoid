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

#include <QDir>
#include <QColor>
#include <QCoreApplication>

#include "qb.h"

QQmlEngine *globalEngine = NULL;
Q_GLOBAL_STATIC(QString, globalQmlPluginPath)
Q_GLOBAL_STATIC(QStringList, globalQmlPluginDefaultPaths)

void Qb::init(QQmlEngine *engine)
{
    qRegisterMetaType<QRgb>("QRgb");
    qRegisterMetaType<QColor>("QColor");

    qRegisterMetaType<QbCaps>("QbCaps");
    qRegisterMetaTypeStreamOperators<QbCaps>("QbCaps");
    qRegisterMetaType<QbElement::ElementState>("QbElement::ElementState");
    qRegisterMetaType<QbElement::ElementState>("ElementState");
    qRegisterMetaTypeStreamOperators<QbElement::ElementState>("QbElement::ElementState");
    qRegisterMetaType<QbFrac>("QbFrac");
    qRegisterMetaTypeStreamOperators<QbFrac>("QbFrac");
    qRegisterMetaType<QbPacket>("QbPacket");
    qRegisterMetaType<QbElementPtr>("QbElementPtr");

    Qb::setQmlEngine(engine);
}

qint64 Qb::id()
{
    static qint64 id = 0;

    return id++;
}

void Qb::setQmlEngine(QQmlEngine *engine)
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

QString Qb::qmlPluginPath()
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

void Qb::setQmlPluginPath(const QString &qmlPluginPath)
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

void Qb::resetQmlPluginPath()
{
#ifdef Q_OS_WIN32
    Qb::setQmlPluginPath(QString("%1%2qml").arg(QCoreApplication::applicationDirPath()).arg(QDir::separator()));
#else
    Qb::setQmlPluginPath(QT_INSTALL_QML);
#endif
}
