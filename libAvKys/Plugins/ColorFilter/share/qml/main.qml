/* Webcamoid, webcam capture application.
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

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import Ak 1.0
import AkControls 1.0 as AK

GridLayout {
    columns: 3

    Connections {
        target: ColorFilter

        function onRadiusChanged(radius)
        {
            sldRadius.value = radius
            spbRadius.value = radius
        }
    }

    // Configure strip color.
    Label {
        id: txtColor
        text: qsTr("Color")
    }
    RowLayout {
        Layout.columnSpan: 2

        Item {
            Layout.fillWidth: true
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(ColorFilter.colorf)
            title: qsTr("Select the color to filter")
            modality: Qt.NonModal
            showAlphaChannel: true
            Accessible.description: txtColor.text

            onCurrentColorChanged: ColorFilter.colorf = AkUtils.toRgba(currentColor)
            onIsOpenChanged: ColorFilter.disable = isOpen
        }
    }

    // Configure color selection radius.
    Label {
        id: lblRadius
        text: qsTr("Radius")
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
    SpinBox {
        id: spbRadius
        value: ColorFilter.radius
        to: sldRadius.to
        stepSize: sldRadius.stepSize
        editable: true
        Accessible.name: lblRadius.text

        onValueChanged: ColorFilter.radius = Number(value)
    }

    // Enable soft color replacing.
    Label {
        id: lblSoft
        text: qsTr("Soft")
    }
    RowLayout {
        Layout.columnSpan: 2

        Item {
            Layout.fillWidth: true
        }
        Switch {
            id: chkSoft
            checked: ColorFilter.soft
            Accessible.name: lblSoft.text

            onCheckedChanged: ColorFilter.soft = checked
        }
    }
}
