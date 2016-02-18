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

#ifdef Q_OS_WIN32
#include <objbase.h>
#endif

#include "ak.h"
#include "akvideocaps.h"

class AkPrivate
{
    public:
        QQmlEngine *m_globalEngine;
        QString m_globalQmlPluginPath;
        QStringList m_globalQmlPluginDefaultPaths;

        AkPrivate()
        {
            this->m_globalEngine = NULL;

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

#ifdef Q_OS_WIN32
            // Initialize the COM library in multithread mode.
            CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif
        }

        ~AkPrivate()
        {
#ifdef Q_OS_WIN32
            // Close COM library.
            CoUninitialize();
#endif
        }
};

Q_GLOBAL_STATIC(AkPrivate, akGlobalStuff)

qint64 Ak::id()
{
    static qint64 id = 0;

    return id++;
}

void Ak::setQmlEngine(QQmlEngine *engine)
{
    if (engine == akGlobalStuff->m_globalEngine)
        return;

    if (akGlobalStuff->m_globalEngine) {
        akGlobalStuff->m_globalEngine->setImportPathList(akGlobalStuff->m_globalQmlPluginDefaultPaths);
        akGlobalStuff->m_globalEngine = NULL;
    }

    if (!engine)
        return;

    akGlobalStuff->m_globalEngine = engine;
    akGlobalStuff->m_globalQmlPluginDefaultPaths = akGlobalStuff->m_globalEngine->importPathList();

    if (!akGlobalStuff->m_globalQmlPluginPath.isEmpty()
        && !akGlobalStuff->m_globalQmlPluginDefaultPaths.contains(akGlobalStuff->m_globalQmlPluginPath))
        akGlobalStuff->m_globalEngine->addImportPath(akGlobalStuff->m_globalQmlPluginPath);
}

QString Ak::qmlPluginPath()
{
#ifdef Q_OS_WIN32
    if (akGlobalStuff->m_globalQmlPluginPath.isEmpty())
        return QString("%1%2qml").arg(QCoreApplication::applicationDirPath()).arg(QDir::separator());
#else
    if (akGlobalStuff->m_globalQmlPluginPath.isEmpty())
        return QString(QT_INSTALL_QML);
#endif

    return akGlobalStuff->m_globalQmlPluginPath;
}

void Ak::setQmlPluginPath(const QString &qmlPluginPath)
{
    if (akGlobalStuff->m_globalQmlPluginPath == qmlPluginPath)
        return;

    akGlobalStuff->m_globalQmlPluginPath = qmlPluginPath;

    if (akGlobalStuff->m_globalEngine) {
        if (akGlobalStuff->m_globalQmlPluginDefaultPaths.isEmpty())
            akGlobalStuff->m_globalQmlPluginDefaultPaths = akGlobalStuff->m_globalEngine->importPathList();
        else
            akGlobalStuff->m_globalEngine->setImportPathList(akGlobalStuff->m_globalQmlPluginDefaultPaths);

        if (!akGlobalStuff->m_globalQmlPluginPath.isEmpty()
            && !akGlobalStuff->m_globalQmlPluginDefaultPaths.contains(akGlobalStuff->m_globalQmlPluginPath))
            akGlobalStuff->m_globalEngine->addImportPath(akGlobalStuff->m_globalQmlPluginPath);
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
