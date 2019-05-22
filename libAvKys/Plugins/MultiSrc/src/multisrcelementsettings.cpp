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

#include "multisrcelementsettings.h"
#include "multisrcglobals.h"

Q_GLOBAL_STATIC(MultiSrcGlobals, globalMultiSrc)

MultiSrcElementSettings::MultiSrcElementSettings(QObject *parent):
    QObject(parent)
{
    QObject::connect(globalMultiSrc,
                     &MultiSrcGlobals::codecLibChanged,
                     this,
                     &MultiSrcElementSettings::codecLibChanged);
}

QString MultiSrcElementSettings::codecLib() const
{
    return globalMultiSrc->codecLib();
}

QStringList MultiSrcElementSettings::subModules() const
{
    return globalMultiSrc->subModules();
}

void MultiSrcElementSettings::setCodecLib(const QString &codecLib)
{
    globalMultiSrc->setCodecLib(codecLib);
}

void MultiSrcElementSettings::resetCodecLib()
{
    globalMultiSrc->resetCodecLib();
}

#include "moc_multisrcelementsettings.cpp"
