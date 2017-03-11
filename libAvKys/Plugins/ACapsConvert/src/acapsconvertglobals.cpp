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

#include <akelement.h>

#include "acapsconvertglobals.h"

ACapsConvertGlobals::ACapsConvertGlobals(QObject *parent):
    QObject(parent)
{
    this->m_preferredFramework = QStringList {
        "ffmpegsw",
        "ffmpegav",
        "gstreamer"
    };

    this->resetConvertLib();
}

QString ACapsConvertGlobals::convertLib() const
{
    return this->m_convertLib;
}

void ACapsConvertGlobals::setConvertLib(const QString &convertLib)
{
    if (this->m_convertLib == convertLib)
        return;

    this->m_convertLib = convertLib;
    emit this->convertLibChanged(convertLib);
}

void ACapsConvertGlobals::resetConvertLib()
{
    auto subModules = AkElement::listSubModules("ACapsConvert");

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
