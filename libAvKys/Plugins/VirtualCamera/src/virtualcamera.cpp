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

#include "virtualcamera.h"
#include "virtualcameraelement.h"

QObject *VirtualCamera::create(const QString &key, const QString &specification)
{
    Q_UNUSED(specification)

    if (key == AK_PLUGIN_TYPE_ELEMENT)
        return new VirtualCameraElement();

    return nullptr;
}

QStringList VirtualCamera::keys() const
{
    return QStringList();
}
