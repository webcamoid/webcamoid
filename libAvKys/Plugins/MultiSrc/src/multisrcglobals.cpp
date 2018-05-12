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

#include "multisrcglobals.h"

MultiSrcGlobals::MultiSrcGlobals(QObject *parent):
    QObject(parent)
{
    this->m_preferredFramework = QStringList {
        "ffmpeg",
        "gstreamer"
    };

    this->resetCodecLib();
}

QString MultiSrcGlobals::codecLib() const
{
    return this->m_codecLib;
}

void MultiSrcGlobals::setCodecLib(const QString &codecLib)
{
    if (this->m_codecLib == codecLib)
        return;

    this->m_codecLib = codecLib;
    emit this->codecLibChanged(codecLib);
}

void MultiSrcGlobals::resetCodecLib()
{
    auto subModules = AkElement::listSubModules("MultiSrc");

    for (const QString &framework: this->m_preferredFramework)
        if (subModules.contains(framework)) {
            this->setCodecLib(framework);

            return;
        }

    if (this->m_codecLib.isEmpty() && !subModules.isEmpty())
        this->setCodecLib(subModules.first());
    else
        this->setCodecLib("");
}
