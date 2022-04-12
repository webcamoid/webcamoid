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

    function strToSize(str)
    {
        if (str.length < 1)
            return Qt.size()

        var size = str.split("x")

        if (size.length < 2)
            return Qt.size()

        return Qt.size(size[0], size[1])
    }

    Connections {
        target: Cartoon

        function onNcolorsChanged(ncolors)
        {
            sldNColors.value = ncolors
            spbNColors.value = ncolors
        }

        function onColorDiffChanged(colorDiff)
        {
            sldColorDiff.value = colorDiff
            spbColorDiff.value = colorDiff
        }

        function onThresholdLowChanged(thresholdLow)
        {
            sldThreshold.first.value = thresholdLow
            spbThresholdLow.value = thresholdLow
        }

        function onThresholdHiChanged(thresholdHi)
        {
            sldThreshold.second.value = thresholdHi
            spbThresholdHi.value = thresholdHi
        }
    }

    Label {
        id: lblNColors
        text: qsTr("Number of colors")
    }
    Slider {
        id: sldNColors
        value: Cartoon.ncolors
        stepSize: 1
        to: 32
        Layout.fillWidth: true
        Accessible.name: lblNColors.text

        onValueChanged: Cartoon.ncolors = value
    }
    SpinBox {
        id: spbNColors
        value: Cartoon.ncolors
        to: sldNColors.to
        stepSize: sldNColors.stepSize
        editable: true
        Accessible.name: lblNColors.text

        onValueChanged: Cartoon.ncolors = Number(value)
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
        Accessible.name: lblColorDiff.text

        onValueChanged: Cartoon.colorDiff = value
    }
    SpinBox {
        id: spbColorDiff
        value: Cartoon.colorDiff
        to: sldColorDiff.to
        stepSize: sldColorDiff.stepSize
        editable: true
        Accessible.name: lblColorDiff.text

        onValueChanged: Cartoon.colorDiff = Number(value)
    }

    Label {
        id: txtShowEdges
        text: qsTr("Show edges")
    }
    RowLayout {
        Layout.columnSpan: 2

        Item {
            Layout.fillWidth: true
        }
        Switch {
            id: chkShowEdges
            checked: Cartoon.showEdges
            Accessible.name: txtShowEdges.text

            onCheckedChanged: Cartoon.showEdges = checked
        }
    }

    // Configure edge thresholds.
    Label {
        id: lblThreshold
        text: qsTr("Threshold")
        enabled: chkShowEdges.checked
    }
    RowLayout {
        Layout.columnSpan: 2

        SpinBox {
            id: spbThresholdLow
            value: Cartoon.thresholdLow
            to: sldThreshold.to
            stepSize: sldThreshold.stepSize
            enabled: chkShowEdges.checked
            editable: true
            Accessible.name: lblThreshold.text

            onValueChanged: Cartoon.thresholdLow = Number(value)
        }
        RangeSlider {
            id: sldThreshold
            first.value: Cartoon.thresholdLow
            second.value: Cartoon.thresholdHi
            stepSize: 1
            to: 255
            enabled: chkShowEdges.checked
            Layout.fillWidth: true
            Accessible.name: lblThreshold.text

            first.onValueChanged: Cartoon.thresholdLow = first.value
            second.onValueChanged: Cartoon.thresholdHi = second.value
        }
        SpinBox {
            id: spbThresholdHi
            value: Cartoon.thresholdHi
            to: sldThreshold.to
            stepSize: sldThreshold.stepSize
            enabled: chkShowEdges.checked
            editable: true
            Accessible.name: lblThreshold.text

            onValueChanged: Cartoon.thresholdHi = Number(value)
        }
    }

    Label {
        id: txtLineColor
        text: qsTr("Line color")
        enabled: chkShowEdges.checked
    }
    RowLayout {
        Layout.columnSpan: 2

        Item {
            Layout.fillWidth: true
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(Cartoon.lineColor)
            title: qsTr("Choose a color")
            enabled: chkShowEdges.checked
            Accessible.description: txtLineColor.text

            onCurrentColorChanged: Cartoon.lineColor = AkUtils.toRgba(currentColor)
        }
    }

    // Scan block.
    Label {
        id: txtScanBlock
        text: qsTr("Scan block")
    }
    TextField {
        text: Cartoon.scanSize.width + "x" + Cartoon.scanSize.height
        placeholderText: qsTr("Scan block")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /\d+x\d+/
        }
        Layout.fillWidth: true
        Layout.columnSpan: 2
        Accessible.name: txtScanBlock.text

        onTextChanged: Cartoon.scanSize = strToSize(text)
    }
}
