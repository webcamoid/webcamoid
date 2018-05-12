/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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

#include "desktopcaptureglobals.h"

DesktopCaptureGlobals::DesktopCaptureGlobals(QObject *parent):
    QObject(parent)
{
    this->m_preferredLibrary = QStringList {
        "avfoundation",
        "qtscreen",
    };

    this->resetCaptureLib();
}

QString DesktopCaptureGlobals::captureLib() const
{
    return this->m_captureLib;
}

void DesktopCaptureGlobals::setCaptureLib(const QString &audioLib)
{
    if (this->m_captureLib == audioLib)
        return;

    this->m_captureLib = audioLib;
    emit this->captureLibChanged(audioLib);
}

void DesktopCaptureGlobals::resetCaptureLib()
{
    auto subModules = AkElement::listSubModules("DesktopCapture");

    for (const QString &framework: this->m_preferredLibrary)
        if (subModules.contains(framework)) {
            this->setCaptureLib(framework);

            return;
        }

    if (this->m_captureLib.isEmpty() && !subModules.isEmpty())
        this->setCaptureLib(subModules.first());
    else
        this->setCaptureLib("");
}
