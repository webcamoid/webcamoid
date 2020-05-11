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

T.DialogButtonBox {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    spacing: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
    leftPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    rightPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    alignment: Qt.AlignRight

    readonly property color activeDark: AkTheme.palette.active.dark
    readonly property color disabledDark: AkTheme.palette.disabled.dark

    delegate: Button {
        flat: true
    }

    contentItem: ListView {
        model: control.contentModel
        spacing: control.spacing
        orientation: ListView.Horizontal
        boundsBehavior: Flickable.StopAtBounds
        snapMode: ListView.SnapToItem
    }

    background: Item {
        id: background
        implicitHeight:
            AkUnit.create(52 * AkTheme.controlScale, "dp").pixels
        clip: true

        Rectangle {
            color: control.enabled?
                       control.activeDark:
                       control.disabledDark
            height: AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
            anchors.left: background.left
            anchors.right: background.right
            y: control.position == DialogButtonBox.Footer?
                   0:
                   background.height - height
        }
    }
}
