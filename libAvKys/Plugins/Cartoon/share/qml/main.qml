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

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3
import AkQmlControls 1.0

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

    Connections {
        target: Cartoon

        onNcolorsChanged: {
            sldNColors.value = ncolors
            spbNColors.rvalue = ncolors
        }
        onColorDiffChanged: {
            sldColorDiff.value = colorDiff
            spbColorDiff.rvalue = colorDiff
        }
        onThresholdLowChanged: {
            sldThresholdLow.value = thresholdLow
            spbThresholdLow.rvalue = thresholdLow
        }
        onThresholdHiChanged: {
            sldThresholdHi.value = thresholdHi
            spbThresholdHi.rvalue = thresholdHi
        }
    }

    Label {
        id: lblNColors
        text: qsTr("N° of colors")
    }
    Slider {
        id: sldNColors
        value: Cartoon.ncolors
        stepSize: 1
        to: 32
        Layout.fillWidth: true

        onValueChanged: Cartoon.ncolors = value
    }
    AkSpinBox {
        id: spbNColors
        rvalue: Cartoon.ncolors
        maximumValue: sldNColors.to
        step: sldNColors.stepSize

        onRvalueChanged: Cartoon.ncolors = rvalue
    }

    Label {
        id: lblColorDiff
        text: qsTr("Color difference")
    }
    Slider {
        id: sldColorDiff
        value: Cartoon.colorDiff
        stepSize: 1
        to: 442
        Layout.fillWidth: true

        onValueChanged: Cartoon.colorDiff = value
    }
    AkSpinBox {
        id: spbColorDiff
        rvalue: Cartoon.colorDiff
        maximumValue: sldColorDiff.to
        step: sldColorDiff.stepSize

        onRvalueChanged: Cartoon.colorDiff = rvalue
    }

    Label {
        text: qsTr("Show edges")
    }
    CheckBox {
        id: chkShowEdges
        checked: Cartoon.showEdges
        Layout.columnSpan: 2

        onCheckedChanged: Cartoon.showEdges = checked
    }

    // Configure edge thresholds.
    Label {
        id: lblThresholdLow
        text: qsTr("Threshold low")
        enabled: chkShowEdges.checked
    }
    Slider {
        id: sldThresholdLow
        value: Cartoon.thresholdLow
        stepSize: 1
        to: 255
        enabled: chkShowEdges.checked
        Layout.fillWidth: true

        onValueChanged: Cartoon.thresholdLow = value
    }
    AkSpinBox {
        id: spbThresholdLow
        rvalue: Cartoon.thresholdLow
        maximumValue: sldThresholdLow.to
        step: sldThresholdLow.stepSize
        enabled: chkShowEdges.checked

        onRvalueChanged: Cartoon.thresholdLow = rvalue
    }

    Label {
        id: lblThresholdHi
        text: qsTr("Threshold high")
        enabled: chkShowEdges.checked
    }
    Slider {
        id: sldThresholdHi
        value: Cartoon.thresholdHi
        stepSize: 1
        to: 255
        enabled: chkShowEdges.checked
        Layout.fillWidth: true

        onValueChanged: Cartoon.thresholdHi = value
    }
    AkSpinBox {
        id: spbThresholdHi
        rvalue: Cartoon.thresholdHi
        maximumValue: sldThresholdHi.to
        step: sldThresholdHi.stepSize
        enabled: chkShowEdges.checked

        onRvalueChanged: Cartoon.thresholdHi = rvalue
    }

    Label {
        text: qsTr("Line color")
        enabled: chkShowEdges.checked
    }
    AkColorButton {
        currentColor: fromRgba(Cartoon.lineColor)
        title: qsTr("Choose a color")

        onCurrentColorChanged: Cartoon.lineColor = toRgba(currentColor)
        enabled: chkShowEdges.checked
    }
    Label {
    }

    // Scan block.
    Label {
        text: qsTr("Scan block")
    }
    TextField {
        text: Cartoon.scanSize.width + "x" + Cartoon.scanSize.height
        Layout.columnSpan: 2
        validator: RegExpValidator {
            regExp: /\d+x\d+/
        }
        Layout.fillWidth: true

        onTextChanged: Cartoon.scanSize = strToSize(text)
    }
}
