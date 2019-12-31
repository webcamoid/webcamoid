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

import QtQuick 2.0
import QtQuick.Controls 2.5
import QtQuick.Templates 2.5 as T
import QtGraphicalEffects 1.0
import QtQuick.Controls.impl 2.12
import AkQml 1.0

T.Dialog {
    id: control
    implicitWidth:
        Math.max(implicitBackgroundWidth + leftInset + rightInset,
                 contentWidth + leftPadding + rightPadding,
                 implicitHeaderWidth,
                 implicitFooterWidth,
                 Ak.newUnit(280 * ThemeSettings.constrolScale, "dp").pixels)
    implicitHeight:
        Math.max(implicitBackgroundHeight + topInset + bottomInset,
                 contentHeight + topPadding + bottomPadding
                 + (implicitHeaderHeight > 0? implicitHeaderHeight + spacing: 0)
                 + (implicitFooterHeight > 0? implicitFooterHeight + spacing: 0),
                 Ak.newUnit(182 * ThemeSettings.constrolScale, "dp").pixels)
    leftPadding: Ak.newUnit(24 * ThemeSettings.constrolScale, "dp").pixels
    rightPadding: Ak.newUnit(24 * ThemeSettings.constrolScale, "dp").pixels
    topPadding:
        title?
            Ak.newUnit(8 * ThemeSettings.constrolScale, "dp").pixels:
            Ak.newUnit(16 * ThemeSettings.constrolScale, "dp").pixels
    bottomPadding: Ak.newUnit(8 * ThemeSettings.constrolScale, "dp").pixels

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

    header: Rectangle {
        id: rectangle
        color:
            control.modal?
                "transparent":
                ThemeSettings.colorPrimary
        clip: true
        visible: control.title
        height: Ak.newUnit(64 * ThemeSettings.constrolScale, "dp").pixels

        Label {
            text: control.title
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: control.leftPadding
            elide: Label.ElideRight
            font.bold: true
            font.pointSize: 16
        }

        Rectangle {
            color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            height: Ak.newUnit(1 * ThemeSettings.constrolScale, "dp").pixels
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
        color: ThemeSettings.colorBack
        radius:
            control.modal?
                Ak.newUnit(8 * ThemeSettings.constrolScale, "dp").pixels:
                0
        layer.enabled: control.modal
        layer.effect: DropShadow {
            cached: true
            horizontalOffset: radius / 2
            verticalOffset: radius / 2
            radius: Ak.newUnit(8 * ThemeSettings.constrolScale, "dp").pixels
            samples: 2 * radius + 1
            color: ThemeSettings.constShade(ThemeSettings.colorBack, -0.9)
        }
    }

    T.Overlay.modal: Rectangle {
        color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5, 0.5)

        Behavior on opacity { NumberAnimation { duration: 150 } }
    }

    T.Overlay.modeless: Rectangle {
        color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5, 0.5)

        Behavior on opacity { NumberAnimation { duration: 150 } }
    }
}
