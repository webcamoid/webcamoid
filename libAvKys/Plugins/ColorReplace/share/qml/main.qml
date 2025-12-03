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
import Ak
import AkControls as AK

ColumnLayout {
    id: root
    layoutDirection: rtl? Qt.RightToLeft: Qt.LeftToRight

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    GridLayout {
        layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight
        columns: 2

        // Color to replace.
        Label {
            id: txtOldColor
            text: qsTr("Old color")
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(ColorReplace.from)
            title: qsTr("Select the color to replace")
            modality: Qt.NonModal
            showAlphaChannel: true
            Accessible.description: txtOldColor.text

            onCurrentColorChanged: ColorReplace.from = AkUtils.toRgba(currentColor)
            onIsOpenChanged: ColorReplace.disable = isOpen
        }

        // Color to replace.
        Label {
            id: txtNewColor
            text: qsTr("New color")
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(ColorReplace.to)
            title: qsTr("Select the new color")
            modality: Qt.NonModal
            showAlphaChannel: true
            Accessible.description: txtNewColor.text

            onCurrentColorChanged: ColorReplace.to = AkUtils.toRgba(currentColor)
            onIsOpenChanged: ColorReplace.disable = isOpen
        }
    }

    // Configure color selection radius.
    Label {
        id: lblRadius
        text: qsTr("Radius")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldRadius
        value: ColorReplace.radius
        stepSize: 1
        to: 256
        Layout.fillWidth: true
        Accessible.name: lblRadius.text

        onValueChanged: ColorReplace.radius = value
    }

    // Enable soft color replacing.
    Switch {
        id: chkSoft
        text: qsTr("Soft")
        checked: ColorReplace.soft
        Accessible.name: text
        Layout.fillWidth: true

        onCheckedChanged: ColorReplace.soft = checked
    }
}
