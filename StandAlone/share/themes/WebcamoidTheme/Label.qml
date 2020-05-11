/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Templates 2.5 as T
import Ak 1.0

T.Label {
    id: control
    color: enabled?
               activeWindowText:
               disabledWindowText
    linkColor: enabled?
                   activeLink:
                   disabledLink
    elide: Text.ElideRight

    readonly property color activeLink: AkTheme.palette.active.link
    readonly property color activeWindowText: AkTheme.palette.active.windowText
    readonly property color disabledLink: AkTheme.palette.disabled.link
    readonly property color disabledWindowText: AkTheme.palette.disabled.windowText
}
