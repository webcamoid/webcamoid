/* Webcamoid, webcam capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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

#include <QLibrary>

#include "plugin.h"
#include "capturepipewire.h"

bool Plugin::canLoad()
{
#ifdef USE_PIPEWIRE_DYNLOAD
    return QLibrary("pipewire-0.3").load();
#else
    return true;
#endif
}

QObject *Plugin::create()
{
    return new CapturePipeWire();
}

#include "moc_plugin.cpp"
