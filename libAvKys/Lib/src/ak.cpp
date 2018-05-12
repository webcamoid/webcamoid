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

#include <QColor>
#include <QCoreApplication>
#include <QDataStream>
#include <QDir>
#include <QQmlEngine>

#ifdef Q_OS_WIN32
#include <objbase.h>
#endif

#include "ak.h"
#include "akcaps.h"
#include "akelement.h"
#include "akaudiocaps.h"
#include "akvideocaps.h"
#include "akpacket.h"

class AkPrivate
{
    public:
        QQmlEngine *m_globalEngine;
        QStringList m_qmlImportPathList;
        QStringList m_qmlDefaultImportPathList;
        QDir m_applicationDir;

        AkPrivate()
        {
            this->m_globalEngine = nullptr;

            qRegisterMetaType<QRgb>("QRgb");
            qRegisterMetaType<QColor>("QColor");
            qRegisterMetaType<AkCaps>("AkCaps");
            qRegisterMetaTypeStreamOperators<AkCaps>("AkCaps");
            qRegisterMetaType<AkCaps::CapsType>("AkCaps::CapsType");
            qRegisterMetaType<AkCaps::CapsType>("CapsType");
            qRegisterMetaType<AkAudioCaps>("AkAudioCaps");
            qRegisterMetaTypeStreamOperators<AkAudioCaps>("AkAudioCaps");
            qRegisterMetaType<AkAudioCaps::SampleFormat>("AkAudioCaps::SampleFormat");
            qRegisterMetaType<AkAudioCaps::SampleFormat>("SampleFormat");
            qRegisterMetaType<AkAudioCaps::SampleType>("AkAudioCaps::SampleType");
            qRegisterMetaType<AkAudioCaps::SampleType>("SampleType");
            qRegisterMetaType<AkAudioCaps::ChannelLayout>("AkAudioCaps::ChannelLayout");
            qRegisterMetaType<AkAudioCaps::ChannelLayout>("ChannelLayout");
            qRegisterMetaType<AkVideoCaps>("AkVideoCaps");
            qRegisterMetaTypeStreamOperators<AkVideoCaps>("AkVideoCaps");
            qRegisterMetaType<AkVideoCaps::PixelFormat>("AkVideoCaps::PixelFormat");
            qRegisterMetaType<AkVideoCaps::PixelFormat>("PixelFormat");
            qRegisterMetaType<AkElement::ElementState>("AkElement::ElementState");
            qRegisterMetaType<AkElement::ElementState>("ElementState");
            qRegisterMetaTypeStreamOperators<AkElement::ElementState>("AkElement::ElementState");
            qRegisterMetaType<AkFrac>("AkFrac");
            qRegisterMetaTypeStreamOperators<AkFrac>("AkFrac");
            qRegisterMetaType<AkPacket>("AkPacket");
            qRegisterMetaType<AkElementPtr>("AkElementPtr");

            this->m_applicationDir.setPath(QCoreApplication::applicationDirPath());

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

        inline QString convertToAbsolute(const QString &path) const
        {
            if (!QDir::isRelativePath(path))
                return QDir::cleanPath(path);

            QString absPath = this->m_applicationDir.absoluteFilePath(path);

            return QDir::cleanPath(absPath);
        }

        inline QStringList qmlImportPaths() const
        {
            QStringList importPaths {QString(QT_INSTALL_QML)};

        #ifdef Q_OS_WIN32
            QString relativePath =
                    QString("%1/../lib/qt/qml")
                        .arg(QCoreApplication::applicationDirPath());
        #elif defined(Q_OS_OSX)
            QString relativePath =
                    QString("%1/../Resources/qml")
                        .arg(QCoreApplication::applicationDirPath());
        #else
            QString relativePath =
                    QString("%1/../lib/qt/qml")
                        .arg(QCoreApplication::applicationDirPath());
        #endif

            importPaths << this->convertToAbsolute(relativePath);

            return importPaths;
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
        akGlobalStuff->m_globalEngine->setImportPathList(akGlobalStuff->m_qmlDefaultImportPathList);
        akGlobalStuff->m_globalEngine = nullptr;
    }

    if (!engine)
        return;

    akGlobalStuff->m_globalEngine = engine;
    akGlobalStuff->m_qmlDefaultImportPathList = akGlobalStuff->m_globalEngine->importPathList();

    for (auto &path: akGlobalStuff->qmlImportPaths())
        akGlobalStuff->m_globalEngine->addImportPath(path);

    for (auto &path: akGlobalStuff->m_qmlImportPathList)
        akGlobalStuff->m_globalEngine->addImportPath(path);
}

QStringList Ak::qmlImportPathList()
{
    return akGlobalStuff->m_qmlImportPathList;
}

void Ak::addQmlImportPath(const QString &path)
{
    akGlobalStuff->m_qmlImportPathList << path;

    if (akGlobalStuff->m_globalEngine) {
        akGlobalStuff->m_globalEngine->setImportPathList(akGlobalStuff->m_qmlDefaultImportPathList);

        for (auto &path: akGlobalStuff->qmlImportPaths())
            akGlobalStuff->m_globalEngine->addImportPath(path);

        for (auto &path: akGlobalStuff->m_qmlImportPathList)
            akGlobalStuff->m_globalEngine->addImportPath(path);
    }
}

void Ak::setQmlImportPathList(const QStringList &paths)
{
    if (akGlobalStuff->m_qmlImportPathList == paths)
        return;

    akGlobalStuff->m_qmlImportPathList = paths;

    if (akGlobalStuff->m_globalEngine) {
        akGlobalStuff->m_globalEngine->setImportPathList(akGlobalStuff->m_qmlDefaultImportPathList);

        for (auto &path: akGlobalStuff->qmlImportPaths())
            akGlobalStuff->m_globalEngine->addImportPath(path);

        for (auto &path: akGlobalStuff->m_qmlImportPathList)
            akGlobalStuff->m_globalEngine->addImportPath(path);
    }
}

void Ak::resetQmlImportPathList()
{
    Ak::setQmlImportPathList({});
}
