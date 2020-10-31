/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

#include "virtualcameraelementsettings.h"
#include "virtualcameraglobals.h"

Q_GLOBAL_STATIC(VirtualCameraGlobals, globalVirtualCamera)

VirtualCameraElementSettings::VirtualCameraElementSettings(QObject *parent):
    QObject(parent)
{
    QObject::connect(globalVirtualCamera,
                     &VirtualCameraGlobals::outputLibChanged,
                     this,
                     &VirtualCameraElementSettings::outputLibChanged);
    QObject::connect(globalVirtualCamera,
                     &VirtualCameraGlobals::rootMethodChanged,
                     this,
                     &VirtualCameraElementSettings::rootMethodChanged);
}

QString VirtualCameraElementSettings::outputLib() const
{
    return globalVirtualCamera->outputLib();
}

QStringList VirtualCameraElementSettings::outputSubModules() const
{
    return globalVirtualCamera->outputSubModules();
}

QString VirtualCameraElementSettings::rootMethod() const
{
    return globalVirtualCamera->rootMethod();
}

QStringList VirtualCameraElementSettings::availableRootMethods() const
{
    return globalVirtualCamera->availableRootMethods();
}

void VirtualCameraElementSettings::setOutputLib(const QString &outputLib)
{
    globalVirtualCamera->setOutputLib(outputLib);
}

void VirtualCameraElementSettings::setRootMethod(const QString &rootMethod)
{
    return globalVirtualCamera->setRootMethod(rootMethod);
}

void VirtualCameraElementSettings::resetOutputLib()
{
    globalVirtualCamera->resetOutputLib();
}

void VirtualCameraElementSettings::resetRootMethod()
{
    globalVirtualCamera->resetRootMethod();
}

#include "moc_virtualcameraelementsettings.cpp"
