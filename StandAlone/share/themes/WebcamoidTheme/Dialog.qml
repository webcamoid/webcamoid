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

T.Dialog {
    id: control
    implicitWidth:
        Math.max(implicitBackgroundWidth + leftInset + rightInset,
                 contentWidth + leftPadding + rightPadding,
                 implicitHeaderWidth,
                 implicitFooterWidth,
                 AkUnit.create(280 * AkTheme.controlScale, "dp").pixels)
    implicitHeight:
        Math.max(implicitBackgroundHeight + topInset + bottomInset,
                 contentHeight + topPadding + bottomPadding
                 + (implicitHeaderHeight > 0? implicitHeaderHeight + spacing: 0)
                 + (implicitFooterHeight > 0? implicitFooterHeight + spacing: 0),
                 AkUnit.create(182 * AkTheme.controlScale, "dp").pixels)
    leftPadding: AkUnit.create(24 * AkTheme.controlScale, "dp").pixels
    rightPadding: AkUnit.create(24 * AkTheme.controlScale, "dp").pixels
    topPadding:
        title?
            AkUnit.create(8 * AkTheme.controlScale, "dp").pixels:
            AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
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

    header: Item {
        id: rectangle
        clip: true
        visible: control.title
        height: AkUnit.create(64 * AkTheme.controlScale, "dp").pixels

        Label {
            text: control.title
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: control.leftPadding
            elide: Label.ElideRight
            font.bold: true
            font.pointSize: 16
            enabled: control.enabled
        }

        Rectangle {
            color: control.enabled?
                       control.activeDark:
                       control.disabledDark
            height: AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
            anchors.left: rectangle.left
            anchors.right: rectangle.right
            anchors.bottom: rectangle.bottom
        }
    }

    footer: DialogButtonBox {
        id: dialogButtonBox
        visible: count > 0
    }

    background: Rectangle {
        color: control.enabled?
                   control.activeWindow:
                   control.disabledWindow
        border.color: control.modal?
                          "transparent":
                      control.enabled?
                          control.activeDark:
                          control.disabledDark
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
