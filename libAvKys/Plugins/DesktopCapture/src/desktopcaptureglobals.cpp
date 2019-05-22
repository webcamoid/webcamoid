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

class DesktopCaptureGlobalsPrivate
{
    public:
        QString m_captureLib;
        QStringList m_preferredLibrary;

        DesktopCaptureGlobalsPrivate();
};

DesktopCaptureGlobals::DesktopCaptureGlobals(QObject *parent):
    QObject(parent)
{
    this->d = new DesktopCaptureGlobalsPrivate;
    this->resetCaptureLib();
}

DesktopCaptureGlobals::~DesktopCaptureGlobals()
{
    delete this->d;
}

QString DesktopCaptureGlobals::captureLib() const
{
    return this->d->m_captureLib;
}

QStringList DesktopCaptureGlobals::subModules() const
{
    return AkElement::listSubModules("DesktopCapture");
}

void DesktopCaptureGlobals::setCaptureLib(const QString &captureLib)
{
    if (this->d->m_captureLib == captureLib)
        return;

    this->d->m_captureLib = captureLib;
    emit this->captureLibChanged(captureLib);
}

void DesktopCaptureGlobals::resetCaptureLib()
{
    auto subModules = AkElement::listSubModules("DesktopCapture");

    for (auto &framework: this->d->m_preferredLibrary)
        if (subModules.contains(framework)) {
            this->setCaptureLib(framework);

            return;
        }

    if (this->d->m_captureLib.isEmpty() && !subModules.isEmpty())
        this->setCaptureLib(subModules.first());
    else
        this->setCaptureLib("");
}

DesktopCaptureGlobalsPrivate::DesktopCaptureGlobalsPrivate()
{
    this->m_preferredLibrary = QStringList {
        "avfoundation",
        "androidscreen",
        "qtscreen",
    };
}

#include "moc_desktopcaptureglobals.cpp"
