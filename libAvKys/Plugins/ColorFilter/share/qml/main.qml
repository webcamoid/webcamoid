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
import AkControls 1.0 as AK

GridLayout {
    columns: 3

    function fromRgba(rgba)
    {
        var a = ((rgba >> 24) & 0xff) / 255.0
        var r = ((rgba >> 16) & 0xff) / 255.0
        var g = ((rgba >> 8) & 0xff) / 255.0
        var b = (rgba & 0xff) / 255.0

        return Qt.rgba(r, g, b, a)
    }

    function toRgba(color)
    {
        var a = Math.round(255 * color.a) << 24
        var r = Math.round(255 * color.r) << 16
        var g = Math.round(255 * color.g) << 8
        var b = Math.round(255 * color.b)

        return a | r | g | b
    }

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
        text: qsTr("Color")
    }
    RowLayout {
        Layout.columnSpan: 2

        Item {
            Layout.fillWidth: true
        }
        AK.ColorButton {
            currentColor: fromRgba(ColorFilter.colorf)
            title: qsTr("Select the color to filter")
            modality: Qt.NonModal
            showAlphaChannel: true

            onCurrentColorChanged: ColorFilter.colorf = toRgba(currentColor)
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

        onValueChanged: ColorFilter.radius = value
    }
    SpinBox {
        id: spbRadius
        value: ColorFilter.radius
        to: sldRadius.to
        stepSize: sldRadius.stepSize
        editable: true

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

            onCheckedChanged: ColorFilter.soft = checked
        }
    }
}
