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
 * Web-Site: http://webcamoid.github.io/
 */

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1

GridLayout {
    columns: 3

    function strToSize(str)
    {
        if (str.length < 1)
            return Qt.size()

        var size = str.split("x")

        if (size.length < 2)
            return Qt.size()

        return Qt.size(size[0], size[1])
    }

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
        id: lblNColors
        text: qsTr("N° of colors")
    }
    Slider {
        id: sldNColors
        value: Cartoon.ncolors
        stepSize: 1
        maximumValue: 32

        onValueChanged: Cartoon.ncolors = value
    }
    SpinBox {
        id: spbNColors
        value: sldNColors.value
        maximumValue: sldNColors.maximumValue
        stepSize: sldNColors.stepSize

        onValueChanged: sldNColors.value = value
    }

    Label {
        id: lblColorDiff
        text: qsTr("Color difference")
    }
    Slider {
        id: sldColorDiff
        value: Cartoon.colorDiff
        stepSize: 1
        maximumValue: 442

        onValueChanged: Cartoon.colorDiff = value
    }
    SpinBox {
        id: spbColorDiff
        value: sldColorDiff.value
        maximumValue: sldColorDiff.maximumValue
        stepSize: sldColorDiff.stepSize

        onValueChanged: sldColorDiff.value = value
    }

    // Configure edge thresholds.
    Label {
        id: lblThresholdLow
        text: qsTr("Threshold low")
    }
    Slider {
        id: sldThresholdLow
        value: Cartoon.thresholdLow
        stepSize: 1
        maximumValue: 255

        onValueChanged: Cartoon.thresholdLow = value
    }
    SpinBox {
        id: spbThresholdLow
        value: sldThresholdLow.value
        maximumValue: sldThresholdLow.maximumValue
        stepSize: sldThresholdLow.stepSize

        onValueChanged: sldThresholdLow.value = value
    }

    Label {
        id: lblThresholdHi
        text: qsTr("Threshold high")
    }
    Slider {
        id: sldThresholdHi
        value: Cartoon.thresholdHi
        stepSize: 1
        maximumValue: 255

        onValueChanged: Cartoon.thresholdHi = value
    }
    SpinBox {
        id: spbThresholdHi
        value: sldThresholdHi.value
        maximumValue: sldThresholdHi.maximumValue
        stepSize: sldThresholdHi.stepSize

        onValueChanged: sldThresholdHi.value = value
    }

    Label {
        text: qsTr("Line color")
    }
    Button {
        Layout.preferredWidth: 32
        Layout.preferredHeight: 32

        style: ButtonStyle {
            background: Rectangle {
                color: fromRgba(Cartoon.lineColor)
                border.color: invert(color)
                border.width: 1
            }
        }

        function setColor(color)
        {
             Cartoon.lineColor = toRgba(color)
        }

        onClicked: {
            colorDialog.caller = this
            colorDialog.currentColor = fromRgba(Cartoon.lineColor)
            colorDialog.open()
        }
    }
    Label {
    }

    ColorDialog {
        id: colorDialog
        title: qsTr("Choose a color")

        property Item caller: null

        onAccepted: caller.setColor(color)
    }
}
