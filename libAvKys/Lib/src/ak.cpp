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
#include "akaudioconverter.h"
#include "akaudiopacket.h"
#include "akcaps.h"
#include "akcolorcomponent.h"
#include "akcolorconvert.h"
#include "akcolorplane.h"
#include "akcompressedvideocaps.h"
#include "akcompressedvideopacket.h"
#include "akelement.h"
#include "akfrac.h"
#include "akpacket.h"
#include "akplugininfo.h"
#include "akpluginmanager.h"
#include "aksubtitlecaps.h"
#include "aksubtitlepacket.h"
#include "akunit.h"
#include "akvideocaps.h"
#include "akvideoconverter.h"
#include "akvideoformatspec.h"
#include "akvideomixer.h"
#include "akvideopacket.h"
#include "qml/akcolorizedimage.h"
#include "qml/akutils.h"
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

Ak::Ak():
    QObject()
{

}

Ak::Ak(const Ak &other):
    QObject()
{
    Q_UNUSED(other)
}

void Ak::registerTypes()
{
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
    AkAudioConverter::registerTypes();
    AkAudioPacket::registerTypes();
    AkCaps::registerTypes();
    AkColorComponent::registerTypes();
    AkColorConvert::registerTypes();
    AkColorPlane::registerTypes();
    AkColorizedImage::registerTypes();
    AkCompressedVideoCaps::registerTypes();
    AkCompressedVideoPacket::registerTypes();
    AkElement::registerTypes();
    AkFrac::registerTypes();
    AkPacket::registerTypes();
    AkPalette::registerTypes();
    AkPaletteGroup::registerTypes();
    AkPluginInfo::registerTypes();
    AkPluginManager::registerTypes();
    AkSubtitleCaps::registerTypes();
    AkSubtitlePacket::registerTypes();
    AkTheme::registerTypes();
    AkUnit::registerTypes();
    AkUtils::registerTypes();
    AkVideoCaps::registerTypes();
    AkVideoConverter::registerTypes();
    AkVideoFormatSpec::registerTypes();
    AkVideoMixer::registerTypes();
    AkVideoPacket::registerTypes();
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
