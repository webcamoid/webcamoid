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

#include <akelement.h>

#include "videocaptureglobals.h"

VideoCaptureGlobals::VideoCaptureGlobals(QObject *parent):
    QObject(parent)
{
    this->m_preferredFramework = QStringList {
        "ffmpeg",
        "gstreamer",
        "generic",
    };

    this->m_preferredLibrary = QStringList {
#ifdef Q_OS_WIN32
        "dshow",
        "mediafoundation",
#elif defined(Q_OS_OSX)
        "avfoundation",
#elif defined(Q_OS_ANDROID)
        "androicamera",
        "ndkcamera",
#else
        "v4lutils",
        "v4l2sys",
#endif
        "libuvc",
    };

    this->resetCodecLib();
    this->resetCaptureLib();
}

QString VideoCaptureGlobals::codecLib() const
{
    return this->m_codecLib;
}

QString VideoCaptureGlobals::captureLib() const
{
    return this->m_captureLib;
}

void VideoCaptureGlobals::setCodecLib(const QString &codecLib)
{
    if (this->m_codecLib == codecLib)
        return;

    this->m_codecLib = codecLib;
    emit this->codecLibChanged(codecLib);
}

void VideoCaptureGlobals::setCaptureLib(const QString &captureLib)
{
    if (this->m_captureLib == captureLib)
        return;

    this->m_captureLib = captureLib;
    emit this->captureLibChanged(captureLib);
}

void VideoCaptureGlobals::resetCodecLib()
{
    auto subModules = AkElement::listSubModules("VideoCapture", "convert");

    for (auto &framework: this->m_preferredFramework)
        if (subModules.contains(framework)) {
            this->setCodecLib(framework);

            return;
        }

    if (this->m_codecLib.isEmpty() && !subModules.isEmpty())
        this->setCodecLib(subModules.first());
    else
        this->setCodecLib("");
}

void VideoCaptureGlobals::resetCaptureLib()
{
    auto subModules = AkElement::listSubModules("VideoCapture", "capture");

    for (auto &framework: this->m_preferredLibrary)
        if (subModules.contains(framework)) {
            this->setCaptureLib(framework);

            return;
        }

    if (this->m_codecLib.isEmpty() && !subModules.isEmpty())
        this->setCaptureLib(subModules.first());
    else
        this->setCaptureLib("");
}
