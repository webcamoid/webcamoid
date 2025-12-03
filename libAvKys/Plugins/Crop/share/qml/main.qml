/* Webcamoid, camera capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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
import Ak
import AkControls as AK

ColumnLayout {
    id: root
    layoutDirection: rtl? Qt.RightToLeft: Qt.LeftToRight

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    Button {
        text: qsTr("Edit")
        icon.source: "image://icons/edit"
        Accessible.description: qsTr("Enable edition mode")
        checked: Crop.editMode
        checkable: true

        onToggled: Crop.editMode = checked
    }
    Button {
        text: qsTr("Pixels/%")
        Accessible.description: qsTr("Select cropping unit")
        checked: Crop.relative
        checkable: true

        onToggled: Crop.relative = checked
    }
    Button {
        text: qsTr("Reset")
        icon.source: "image://icons/reset"
        Accessible.description: qsTr("Reset parameters")

        onClicked: Crop.reset()
    }
    Switch {
        text: qsTr("Keep resolution")
        checked: Crop.keepResolution
        Accessible.name: text
        Layout.fillWidth: true

        onCheckedChanged: Crop.keepResolution = checked
    }
    Label {
        id: lblLeft
        text: qsTr("Left")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldLeft
        value: Crop.left
        stepSize: 1
        from: 0.0
        to: Crop.relative? 100.0: Crop.frameWidth
        Layout.fillWidth: true
        Accessible.name: lblLeft.text

        onValueChanged: Crop.left = value
    }
    Label {
        id: lblRight
        text: qsTr("Right")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldRight
        value: Crop.right
        stepSize: 1
        from: 1.0
        to: Crop.relative? 100.0: Crop.frameWidth
        Layout.fillWidth: true
        Accessible.name: lblRight.text

        onValueChanged: Crop.right = value
    }
    Label {
        id: lblTop
        text: qsTr("Top")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldTop
        value: Crop.top
        stepSize: 1
        from: 0.0
        to: Crop.relative? 100.0: Crop.frameHeight
        Layout.fillWidth: true
        Accessible.name: lblTop.text

        onValueChanged: Crop.top = value
    }
    Label {
        id: lblBottom
        text: qsTr("Bottom")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldBottom
        value: Crop.bottom
        stepSize: 1
        from: 1.0
        to: Crop.relative? 100.0: Crop.frameHeight
        Layout.fillWidth: true
        Accessible.name: lblBottom.text

        onValueChanged: Crop.bottom = value
    }
    RowLayout {
        layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight

        Label {
            id: txtFillColor
            text: qsTr("Fill color")
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(Crop.fillColor)
            title: qsTr("Choose the filling color")
            showAlphaChannel: true
            Accessible.description: txtFillColor.text

            onCurrentColorChanged: Crop.fillColor = AkUtils.toRgba(currentColor)
        }
    }
}
