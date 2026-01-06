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

    // Configure strip color.
    AK.ColorButton {
        text: qsTr("Color")
        currentColor: AkUtils.fromRgba(ColorFilter.colorf)
        title: qsTr("Select the color to filter")
        modality: Qt.NonModal
        showAlphaChannel: true
        horizontalAlignment: root.rtl? Text.AlignRight: Text.AlignLeft
        Layout.fillWidth: true

        onCurrentColorChanged: ColorFilter.colorf = AkUtils.toRgba(currentColor)
        onIsOpenChanged: ColorFilter.disable = isOpen
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
        value: ColorFilter.radius
        stepSize: 1
        to: 256
        Layout.fillWidth: true
        Accessible.name: lblRadius.text

        onValueChanged: ColorFilter.radius = value
    }

    // Enable soft color replacing.
    Switch {
        id: chkSoft
        text: qsTr("Soft")
        checked: ColorFilter.soft
        Accessible.name: text
        Layout.fillWidth: true

        onCheckedChanged: ColorFilter.soft = checked
    }
}
