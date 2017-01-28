/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1

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

    Label {
        text: qsTr("Color")
    }
    Button {
        Layout.preferredWidth: 32
        Layout.preferredHeight: 32

        style: ButtonStyle {
            background: Rectangle {
                color: fromRgba(Life.lifeColor)
                border.color: invert(color)
                border.width: 1
            }
        }

        onClicked: colorDialog.open()
    }
    Label {
    }

    Label {
        id: lblThreshold
        text: qsTr("Threshold")
    }
    Slider {
        id: sldThreshold
        value: Life.threshold
        maximumValue: 255
        stepSize: 1

        onValueChanged: Life.threshold = value
    }
    SpinBox {
        id: spbThreshold
        value: sldThreshold.value
        maximumValue: sldThreshold.maximumValue
        stepSize: sldThreshold.stepSize

        onValueChanged: sldThreshold.value = value
    }

    Label {
        id: lblLumaThreshold
        text: qsTr("Luma Threshold")
    }
    Slider {
        id: sldLumaThreshold
        value: Life.lumaThreshold
        maximumValue: 255
        stepSize: 1

        onValueChanged: Life.lumaThreshold = value
    }
    SpinBox {
        id: spbLumaThreshold
        value: sldLumaThreshold.value
        maximumValue: sldLumaThreshold.maximumValue
        stepSize: sldLumaThreshold.stepSize

        onValueChanged: sldLumaThreshold.value = value
    }

    ColorDialog {
        id: colorDialog
        title: qsTr("Choose the automata color")
        currentColor: fromRgba(Life.lifeColor)
        showAlphaChannel: true

        onAccepted: Life.lifeColor = toRgba(color)
    }
}
