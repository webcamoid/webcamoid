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

T.Popup {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding,
                            AkUnit.create(344 * AkTheme.controlScale, "dp").pixels)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding,
                             AkUnit.create(148 * AkTheme.controlScale, "dp").pixels)
    leftPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    rightPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    topPadding: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
    bottomPadding: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels

    readonly property color activeDark: AkTheme.palette.active.dark
    readonly property color activeWindow: AkTheme.palette.active.window
    readonly property color disabledDark: AkTheme.palette.disabled.dark
    readonly property color disabledWindow: AkTheme.palette.disabled.window

    // Fade in
    enter: Transition {
        NumberAnimation { property: "scale"; from: 0.9; to: 1.0; easing.type: Easing.OutQuint; duration: 220 }
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutCubic; duration: 150 }
    }

    // Fade out
    exit: Transition {
        NumberAnimation { property: "scale"; from: 1.0; to: 0.9; easing.type: Easing.OutQuint; duration: 220 }
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutCubic; duration: 150 }
    }

    background: Rectangle {
        color: control.enabled?
                   control.activeWindow:
                   control.disabledWindow
        border.color: control.enabled?
                          control.activeDark:
                          control.disabledDark
        radius: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
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
}
