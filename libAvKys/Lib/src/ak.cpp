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
#include <QDebug>
#include <QDir>
#include <QQmlEngine>

#ifdef Q_OS_WIN32
#include <objbase.h>
#endif

#include "ak.h"
#include "akaudiocaps.h"
#include "akaudiopacket.h"
#include "akcaps.h"
#include "akelement.h"
#include "akfrac.h"
#include "akpacket.h"
#include "akunit.h"
#include "akvideocaps.h"
#include "akvideopacket.h"
#include "qml/akcolorizedimage.h"
#include "qml/akpalette.h"
#include "qml/akpalettegroup.h"
#include "qml/aktheme.h"

class AkPrivate
{
    public:
        QQmlEngine *m_globalEngine {nullptr};
        QStringList m_qmlImportPathList;
        QStringList m_qmlDefaultImportPathList;
        QDir m_applicationDir;

        AkPrivate();
#ifdef Q_OS_WIN32
        virtual ~AkPrivate();
#endif
        QString convertToAbsolute(const QString &path) const;
        QStringList qmlImportPaths() const;
};

Q_GLOBAL_STATIC(AkPrivate, akGlobalStuff)

Ak::Ak()
{

}

Ak::Ak(const Ak &other):
    QObject()
{
    Q_UNUSED(other)
}

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

    if (!engine->importPathList().contains(":/Ak/share/qml"))
        engine->addImportPath(":/Ak/share/qml");

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

AkPrivate::AkPrivate()
{
    this->m_globalEngine = nullptr;

    qRegisterMetaType<size_t>("size_t");
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
    QMetaType::registerDebugStreamOperator<AkAudioCaps::SampleFormat>();
    qRegisterMetaType<QList<AkAudioCaps::SampleFormat>>("QList<AkAudioCaps::SampleFormat>");
    qRegisterMetaType<QList<AkAudioCaps::SampleFormat>>("QList<SampleFormat>");
    qRegisterMetaType<AkAudioCaps::SampleType>("AkAudioCaps::SampleType");
    qRegisterMetaType<AkAudioCaps::SampleType>("SampleType");
    QMetaType::registerDebugStreamOperator<AkAudioCaps::SampleType>();
    qRegisterMetaType<AkAudioCaps::Position>("AkAudioCaps::Position");
    qRegisterMetaType<AkAudioCaps::Position>("Position");
    QMetaType::registerDebugStreamOperator<AkAudioCaps::Position>();
    qRegisterMetaType<AkAudioCaps::ChannelLayout>("AkAudioCaps::ChannelLayout");
    qRegisterMetaType<AkAudioCaps::ChannelLayout>("ChannelLayout");
    QMetaType::registerDebugStreamOperator<AkAudioCaps::ChannelLayout>();
    qRegisterMetaType<QList<AkAudioCaps::ChannelLayout>>("QList<AkAudioCaps::ChannelLayout>");
    qRegisterMetaType<QList<AkAudioCaps::ChannelLayout>>("QList<ChannelLayout>");
    qRegisterMetaType<AkVideoCaps>("AkVideoCaps");
    qRegisterMetaTypeStreamOperators<AkVideoCaps>("AkVideoCaps");
    qRegisterMetaType<AkVideoCaps::PixelFormat>("AkVideoCaps::PixelFormat");
    qRegisterMetaType<AkVideoCaps::PixelFormat>("PixelFormat");
    QMetaType::registerDebugStreamOperator<AkVideoCaps::PixelFormat>();
    qRegisterMetaType<AkElement::ElementState>("AkElement::ElementState");
    qRegisterMetaType<AkElement::ElementState>("ElementState");
    qRegisterMetaTypeStreamOperators<AkElement::ElementState>("AkElement::ElementState");
    qRegisterMetaType<AkFrac>("AkFrac");
    qRegisterMetaTypeStreamOperators<AkFrac>("AkFrac");
    QMetaType::registerDebugStreamOperator<AkFrac>();
    qRegisterMetaType<AkUnit>("AkUnit");
    qRegisterMetaTypeStreamOperators<AkUnit>("AkUnit");
    QMetaType::registerDebugStreamOperator<AkUnit>();
    qRegisterMetaType<AkUnit::Unit>("AkUnit::Unit");
    qRegisterMetaType<AkUnit::Unit>("Unit");
    qRegisterMetaType<AkPacket>("AkPacket");
    qRegisterMetaType<AkAudioPacket>("AkAudioPacket");
    qRegisterMetaType<AkVideoPacket>("AkVideoPacket");
    qRegisterMetaType<AkElementPtr>("AkElementPtr");
    qmlRegisterSingletonType<AkAudioCaps>("Ak", 1, 0, "Ak",
                                          [] (QQmlEngine *qmlEngine,
                                              QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new Ak();
    });
    qmlRegisterSingletonType<AkAudioCaps>("Ak", 1, 0, "AkAudioCaps",
                                          [] (QQmlEngine *qmlEngine,
                                              QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkAudioCaps();
    });
    qmlRegisterSingletonType<AkAudioPacket>("Ak", 1, 0, "AkAudioPacket",
                                            [] (QQmlEngine *qmlEngine,
                                                QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkAudioPacket();
    });
    qmlRegisterSingletonType<AkCaps>("Ak", 1, 0, "AkCaps",
                                     [] (QQmlEngine *qmlEngine,
                                         QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkCaps();
    });
    qmlRegisterSingletonType<AkElement>("Ak", 1, 0, "AkElement",
                                        [] (QQmlEngine *qmlEngine,
                                            QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkElement();
    });
    qmlRegisterSingletonType<AkFrac>("Ak", 1, 0, "AkFrac",
                                     [] (QQmlEngine *qmlEngine,
                                         QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkFrac();
    });
    qmlRegisterSingletonType<AkPacket>("Ak", 1, 0, "AkPacket",
                                       [] (QQmlEngine *qmlEngine,
                                           QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkPacket();
    });
    qmlRegisterSingletonType<AkUnit>("Ak", 1, 0, "AkUnit",
                                     [] (QQmlEngine *qmlEngine,
                                         QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkUnit();
    });
    qmlRegisterSingletonType<AkVideoCaps>("Ak", 1, 0, "AkVideoCaps",
                                          [] (QQmlEngine *qmlEngine,
                                              QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkVideoCaps();
    });
    qmlRegisterSingletonType<AkVideoPacket>("Ak", 1, 0, "AkVideoPacket",
                                            [] (QQmlEngine *qmlEngine,
                                                QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkVideoPacket();
    });
    qmlRegisterType<AkColorizedImage>("Ak", 1, 0, "AkColorizedImage");
    qRegisterMetaType<AkColorizedImage::FillMode>("AkColorizedImage::FillMode");
    qRegisterMetaType<AkColorizedImage::HorizontalAlignment>("AkColorizedImage::HorizontalAlignment");
    qRegisterMetaType<AkColorizedImage::VerticalAlignment>("AkColorizedImage::VerticalAlignment");
    qRegisterMetaType<AkColorizedImage::Status>("AkColorizedImage::Status");
    qRegisterMetaType<AkColorizedImage::Status>("Status");
    qmlRegisterUncreatableType<AkTheme>("Ak", 1, 0, "AkTheme", "AkTheme is an attached property");
    qRegisterMetaType<AkPalette>("AkPalette");
    qmlRegisterType<AkPalette>("Ak", 1, 0, "AkPalette");
    qRegisterMetaType<AkPaletteGroup>("AkPaletteGroup");
    qmlRegisterType<AkPaletteGroup>("Ak", 1, 0, "AkPaletteGroup");

    this->m_applicationDir.setPath(QCoreApplication::applicationDirPath());

#ifdef Q_OS_WIN32
    // Initialize the COM library in multithread mode.
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif
}

#ifdef Q_OS_WIN32
AkPrivate::~AkPrivate()
{
    // Close COM library.
    CoUninitialize();
}
#endif

QString AkPrivate::convertToAbsolute(const QString &path) const
{
    if (!QDir::isRelativePath(path))
        return QDir::cleanPath(path);

    QString absPath = this->m_applicationDir.absoluteFilePath(path);

    return QDir::cleanPath(absPath);
}

QStringList AkPrivate::qmlImportPaths() const
{
#ifdef Q_OS_ANDROID
    return {};
#else
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
#endif
}

#include "moc_ak.cpp"
