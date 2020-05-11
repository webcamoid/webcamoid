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

T.Drawer {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)
    parent: T.Overlay.overlay

    readonly property color activeDark: AkTheme.palette.active.dark
    readonly property color activeWindow: AkTheme.palette.active.window
    readonly property color disabledDark: AkTheme.palette.disabled.dark
    readonly property color disabledWindow: AkTheme.palette.disabled.window

    background: Rectangle {
        implicitWidth: AkUnit.create(256 * AkTheme.controlScale, "dp").pixels
        implicitHeight: AkUnit.create(256 * AkTheme.controlScale, "dp").pixels
        color: control.enabled?
                   control.activeWindow:
                   control.disabledWindow

        Rectangle {
            width: horizontal? handleWidth: parent.width
            height: horizontal? parent.height: handleWidth
            color: control.enabled?
                       control.activeDark:
                       control.disabledDark
            x: control.edge === Qt.LeftEdge? parent.width - handleWidth: 0
            y: control.edge === Qt.TopEdge? parent.height - handleWidth: 0
            visible: !control.dim

            readonly property bool horizontal:
                control.edge === Qt.LeftEdge || control.edge === Qt.RightEdge
            readonly property real handleWidth:
                AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
        }
    }

    T.Overlay.modal: Rectangle {
        color: control.enabled?
                   AkTheme.shade(control.activeDark, 0, 0.75):
                   AkTheme.shade(control.disabledDark, 0, 0.75)

        Behavior on opacity { NumberAnimation { duration: 150 } }
    }

    T.Overlay.modeless: Rectangle {
        color: control.enabled?
                   AkTheme.shade(control.activeDark, 0, 0.75):
                   AkTheme.shade(control.disabledDark, 0, 0.75)

        Behavior on opacity { NumberAnimation { duration: 150 } }
    }

    enter: Transition { SmoothedAnimation { velocity: 5 } }
    exit: Transition { SmoothedAnimation { velocity: 5 } }
}
