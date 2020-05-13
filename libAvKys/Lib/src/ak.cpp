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

        AkPrivate();
#ifdef Q_OS_WIN32
        virtual ~AkPrivate();
#endif
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

    if (engine && !engine->importPathList().contains(":/Ak/share/qml"))
        engine->addImportPath(":/Ak/share/qml");

    akGlobalStuff->m_globalEngine = engine;
}

AkPrivate::AkPrivate()
{
    this->m_globalEngine = nullptr;

    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<QRgb>("QRgb");
    qRegisterMetaType<QColor>("QColor");
    qmlRegisterSingletonType<Ak>("Ak", 1, 0, "Ak",
                                 [] (QQmlEngine *qmlEngine,
                                     QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new Ak();
    });
    AkAudioCaps::registerTypes();
    AkAudioPacket::registerTypes();
    AkCaps::registerTypes();
    AkElement::registerTypes();
    AkFrac::registerTypes();
    AkPacket::registerTypes();
    AkUnit::registerTypes();
    AkVideoCaps::registerTypes();
    AkVideoPacket::registerTypes();
    AkColorizedImage::registerTypes();
    AkTheme::registerTypes();
    AkPaletteGroup::registerTypes();
    AkPalette::registerTypes();

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

#include "moc_ak.cpp"
