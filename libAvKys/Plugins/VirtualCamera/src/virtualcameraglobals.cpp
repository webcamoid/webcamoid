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

#include <QDir>
#include <QProcessEnvironment>
#include <akelement.h>

#include "virtualcameraglobals.h"

VirtualCameraGlobals::VirtualCameraGlobals(QObject *parent):
    QObject(parent)
{
    this->m_preferredLibrary = QStringList {
#ifdef Q_OS_WIN32
        "dshow"
#elif defined(Q_OS_OSX)
        "cmio"
#else
        "v4l2out"
#endif
    };

    this->resetOutputLib();
}

QString VirtualCameraGlobals::outputLib() const
{
    return this->m_outputLib;
}

void VirtualCameraGlobals::setOutputLib(const QString &outputLib)
{
    if (this->m_outputLib == outputLib)
        return;

    this->m_outputLib = outputLib;
    emit this->outputLibChanged(outputLib);
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
