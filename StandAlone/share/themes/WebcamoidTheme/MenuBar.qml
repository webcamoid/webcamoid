/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

T.MenuBar {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)
    spacing: 0
    hoverEnabled: true
    delegate: MenuBarItem { }

    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color disabledHighlight: AkTheme.palette.disabled.highlight

    contentItem: Row {
        spacing: control.spacing

        Repeater {
            model: control.contentModel
        }
    }

    background: Rectangle {
        implicitWidth: AkUnit.create(360 * AkTheme.controlScale, "dp").pixels
        implicitHeight: AkUnit.create(48 * AkTheme.controlScale, "dp").pixels
        color: enabled?
                   control.activeHighlight:
                   control.disabledHighlight
    }
}
