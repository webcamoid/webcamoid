/* Webcamoid, camera capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root
    layoutDirection: rtl? Qt.RightToLeft: Qt.LeftToRight

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    GridLayout {
        columns: 2
        layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight

        Label {
            id: txtBlockWidth
            text: qsTr("Block width")
        }
        SpinBox {
            id: spinWidth
            from: 1
            to: 4096
            value: Pixelate.blockWidth
            editable: true
            Accessible.name: txtBlockWidth.text

            onValueChanged: Pixelate.blockWidth = Number(value)
        }

        Label {
            id: txtBlockHeight
            text: qsTr("Block height")
        }
        SpinBox {
            id: spinHeight
            from: 1
            to: 4096
            value: Pixelate.blockHeight
            editable: true
            Accessible.name: txtBlockHeight.text

            onValueChanged: Pixelate.blockHeight = Number(value)
        }
    }
}
