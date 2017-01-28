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

#ifndef AK_H
#define AK_H

#include "akplugin.h"
#include "akelement.h"

namespace Ak
{
    AKCOMMONS_EXPORT qint64 id();
    AKCOMMONS_EXPORT void setQmlEngine(QQmlEngine *engine);
    AKCOMMONS_EXPORT QString qmlPluginPath();
    AKCOMMONS_EXPORT void setQmlPluginPath(const QString &qmlPluginPath);
    AKCOMMONS_EXPORT void resetQmlPluginPath();
}

#endif // AK_H
