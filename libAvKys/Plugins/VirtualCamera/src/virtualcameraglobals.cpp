/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include <QDir>
#include <QProcessEnvironment>
#include <akelement.h>

#include "virtualcameraglobals.h"

VirtualCameraGlobals::VirtualCameraGlobals(QObject *parent):
    QObject(parent)
{
    this->m_preferredFramework = QStringList {
        "ffmpeg",
        "gstreamer"
    };

    this->m_preferredLibrary = QStringList {
#ifdef Q_OS_WIN32
        "dshow"
#elif defined(Q_OS_OSX)
        "avfoundation"
#else
        "v4l2out"
#endif
    };

    this->m_preferredRootMethod = QStringList {
#if defined(Q_OS_UNIX) && !defined(Q_OS_OSX)
        "kdesu",
        "kdesudo",
        "gksu",
        "gksudo",
        "gtksu",
        "sudo",
        "su"
#endif
    };

    this->resetConvertLib();
    this->resetOutputLib();
    this->resetRootMethod();
}

QString VirtualCameraGlobals::convertLib() const
{
    return this->m_convertLib;
}

QString VirtualCameraGlobals::outputLib() const
{
    return this->m_outputLib;
}

QString VirtualCameraGlobals::rootMethod() const
{
    return this->m_rootMethod;
}

QStringList VirtualCameraGlobals::availableMethods() const
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_OSX)
    auto paths = QProcessEnvironment::systemEnvironment().value("PATH").split(':');

    QStringList sus {
        "gksu",
        "gksudo",
        "gtksu",
        "kdesu",
        "kdesudo",
        "su",
        "sudo"
    };

    QStringList methods;

    for (auto &su: sus)
        for (auto &path: paths)
            if (QDir(path).exists(su)) {
                methods << su;

                break;
            }

    return methods;
#else
    return {};
#endif
}

void VirtualCameraGlobals::setConvertLib(const QString &convertLib)
{
    if (this->m_convertLib == convertLib)
        return;

    this->m_convertLib = convertLib;
    emit this->convertLibChanged(convertLib);
}

void VirtualCameraGlobals::setOutputLib(const QString &outputLib)
{
    if (this->m_outputLib == outputLib)
        return;

    this->m_outputLib = outputLib;
    emit this->outputLibChanged(outputLib);
}

void VirtualCameraGlobals::setRootMethod(const QString &rootMethod)
{
    if (this->m_rootMethod == rootMethod)
        return;

    this->m_rootMethod = rootMethod;
    emit this->rootMethodChanged(rootMethod);
}

void VirtualCameraGlobals::resetConvertLib()
{
    auto subModules = AkElement::listSubModules("VirtualCamera", "convert");

    for (const QString &framework: this->m_preferredFramework)
        if (subModules.contains(framework)) {
            this->setConvertLib(framework);

            return;
        }

    if (this->m_convertLib.isEmpty() && !subModules.isEmpty())
        this->setConvertLib(subModules.first());
    else
        this->setConvertLib("");
}

void VirtualCameraGlobals::resetOutputLib()
{
    auto subModules = AkElement::listSubModules("VirtualCamera", "output");

    for (const QString &framework: this->m_preferredLibrary)
        if (subModules.contains(framework)) {
            this->setOutputLib(framework);

            return;
        }

    if (this->m_outputLib.isEmpty() && !subModules.isEmpty())
        this->setOutputLib(subModules.first());
    else
        this->setOutputLib("");
}

void VirtualCameraGlobals::resetRootMethod()
{
    auto methods = this->availableMethods();

    for (const QString &rootMethod: this->m_preferredRootMethod)
        if (methods.contains(rootMethod)) {
            this->setRootMethod(rootMethod);

            return;
        }

    if (this->m_rootMethod.isEmpty() && !methods.isEmpty())
        this->setRootMethod(methods.first());
    else
        this->setRootMethod("");
}
