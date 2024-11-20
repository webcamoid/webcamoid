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
#include "akcompressedaudiocaps.h"
#include "akcompressedaudiopacket.h"
#include "akcompressedvideocaps.h"
#include "akcompressedvideopacket.h"
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
#include "iak/akelement.h"
#include "qml/akcolorizedimage.h"
#include "qml/akfontsettings.h"
#include "qml/akpalette.h"
#include "qml/akpalettegroup.h"
#include "qml/aktheme.h"
#include "qml/akutils.h"

class AkPrivate
{
    public:
        QQmlEngine *m_globalEngine {nullptr};

        AkPrivate();
#ifdef Q_OS_WIN32
        virtual ~AkPrivate();
#endif

#ifdef Q_OS_ANDROID
        static void jniLog(JNIEnv *env,
                           jobject obj,
                           jobject type,
                           jobject msg);
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
    AkCompressedAudioCaps::registerTypes();
    AkCompressedAudioPacket::registerTypes();
    AkCompressedVideoCaps::registerTypes();
    AkCompressedVideoPacket::registerTypes();
    AkElement::registerTypes();
    AkFontSettings::registerTypes();
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

QString Ak::platform()
{
#ifdef Q_OS_WIN32
    return {"windows"};
#elif defined(Q_OS_OSX)
    return {"macos"};
#elif defined(Q_OS_ANDROID)
    return {"android"};
#elif defined(Q_OS_POSIX)
    return {"posix"};
#else
    return {"unknown"};
#endif
}

void Ak::setQmlEngine(QQmlEngine *engine)
{
    if (engine == akGlobalStuff->m_globalEngine)
        return;

    if (engine && !engine->importPathList().contains(":/Ak/share/qml"))
        engine->addImportPath(":/Ak/share/qml");

    akGlobalStuff->m_globalEngine = engine;
}

bool Ak::isFlatpak()
{
    static const bool isFlatpak = QFile::exists("/.flatpak-info");

    return isFlatpak;
}

bool Ak::hasFlatpakVCam()
{
#ifdef WITH_FLATPAK_VCAM
    return true;
#else
    return false;
#endif
}

#ifdef Q_OS_ANDROID
void Ak::registerJniLogFunc(const QString &className)
{
    static bool jniLogFuncReady = false;

    if (jniLogFuncReady)
        return;

    QJniEnvironment jenv;

    if (auto jclass = jenv.findClass(className.toStdString().c_str())) {
        static const QVector<JNINativeMethod> akMethods {
            {"akLog", "(Ljava/lang/String;Ljava/lang/String;)V", reinterpret_cast<void *>(AkPrivate::jniLog)},
        };

        jenv->RegisterNatives(jclass,
                              akMethods.data(),
                              akMethods.size());
    }

    jniLogFuncReady = true;
}
#endif

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

#ifdef Q_OS_ANDROID
void AkPrivate::jniLog(JNIEnv *env,
                       jobject obj,
                       jobject type,
                       jobject msg)
{
    Q_UNUSED(env)
    Q_UNUSED(obj)

    auto msgType = QJniObject(type).toString();
    auto msgStr = QJniObject(msg).toString();

    if (msgType == "debug")
        qDebug() << msgStr.toStdString().c_str();
    else if (msgType == "warning")
        qWarning() << msgStr.toStdString().c_str();
    else if (msgType == "critical" || msgType == "fatal")
        qCritical() << msgStr.toStdString().c_str();
    else if (msgType == "info")
        qInfo() << msgStr.toStdString().c_str();
}
#endif

#include "moc_ak.cpp"
