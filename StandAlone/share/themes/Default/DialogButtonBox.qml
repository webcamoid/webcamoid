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
import QtQuick.Controls.impl 2.12
import AkQml 1.0

T.DialogButtonBox {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    spacing: Ak.newUnit(8 * ThemeSettings.constrolScale, "dp").pixels
    leftPadding: Ak.newUnit(16 * ThemeSettings.constrolScale, "dp").pixels
    rightPadding: Ak.newUnit(16 * ThemeSettings.constrolScale, "dp").pixels
    alignment: Qt.AlignRight

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
            Ak.newUnit(52 * ThemeSettings.constrolScale, "dp").pixels
        clip: true

        Rectangle {
            color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            height: Ak.newUnit(1 * ThemeSettings.constrolScale, "dp").pixels
            anchors.left: background.left
            anchors.right: background.right
            y: control.position == DialogButtonBox.Footer?
                   0:
                   background.height - height
        }
    }
}
