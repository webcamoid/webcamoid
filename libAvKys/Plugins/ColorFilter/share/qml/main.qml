/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.2

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

    function invert(color) {
        return Qt.rgba(1.0 - color.r, 1.0 - color.g, 1.0 - color.b, 1)
    }

    // Configure strip color.
    Label {
        text: qsTr("Color")
    }
    Button {
        Layout.preferredWidth: 32
        Layout.preferredHeight: 32

        style: ButtonStyle {
            background: Rectangle {
                color: fromRgba(ColorFilter.colorf)
                border.color: invert(color)
                border.width: 1
            }
        }

        onClicked: colorDialog.open()
    }
    Label {
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
        maximumValue: 256

        onValueChanged: ColorFilter.radius = value
    }
    SpinBox {
        id: spbRadius
        value: sldRadius.value
        maximumValue: sldRadius.maximumValue
        stepSize: sldRadius.stepSize

        onValueChanged: sldRadius.value = value
    }

    // Enable soft color replacing.
    Label {
        id: lblSoft
        text: qsTr("Soft")
    }
    CheckBox {
        id: chkSoft
        checked: ColorFilter.soft

        onCheckedChanged: ColorFilter.soft = checked
    }
    Label {
    }

    ColorDialog {
        id: colorDialog
        title: qsTr("Select the color to filter")
        currentColor: fromRgba(ColorFilter.colorf)
        showAlphaChannel: true

        onAccepted: ColorFilter.colorf = toRgba(color)
        onVisibleChanged: ColorFilter.disable = visible
    }
}
