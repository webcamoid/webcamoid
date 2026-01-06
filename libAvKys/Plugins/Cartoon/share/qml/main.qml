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

    function strToSize(str)
    {
        if (str.length < 1)
            return Qt.size()

        var size = str.split("x")

        if (size.length < 2)
            return Qt.size()

        return Qt.size(size[0], size[1])
    }

    Label {
        id: lblNColors
        text: qsTr("Number of colors")
        font.bold: true
        Layout.fillWidth: true
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

    Label {
        id: lblColorDiff
        text: qsTr("Color difference")
        font.bold: true
        Layout.fillWidth: true
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

    Switch {
        id: chkShowEdges
        text: qsTr("Show edges")
        checked: Cartoon.showEdges
        Accessible.name: text
        Layout.fillWidth: true

        onCheckedChanged: Cartoon.showEdges = checked
    }

    // Configure edge thresholds.
    Label {
        id: lblThreshold
        text: qsTr("Threshold")
        enabled: chkShowEdges.checked
        font.bold: true
        Layout.fillWidth: true
    }
    RangeSlider {
        id: sldThreshold
        first.value: Cartoon.thresholdLow
        second.value: Cartoon.thresholdHi
        stepSize: 1
        to: 255
        enabled: chkShowEdges.checked
        Accessible.name: lblThreshold.text
        Layout.fillWidth: true

        first.onValueChanged: Cartoon.thresholdLow = first.value
        second.onValueChanged: Cartoon.thresholdHi = second.value
    }
    AK.ColorButton {
        text: qsTr("Line color")
        currentColor: AkUtils.fromRgba(Cartoon.lineColor)
        title: qsTr("Choose a color")
        enabled: chkShowEdges.checked
        horizontalAlignment: root.rtl? Text.AlignRight: Text.AlignLeft
        Layout.fillWidth: true

        onCurrentColorChanged: Cartoon.lineColor = AkUtils.toRgba(currentColor)
    }

    // Scan block.
    Label {
        id: txtScanBlock
        text: qsTr("Scan block")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        text: Cartoon.scanSize.width + "x" + Cartoon.scanSize.height
        placeholderText: qsTr("Scan block")
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /\d+x\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtScanBlock.text

        onTextChanged: Cartoon.scanSize = strToSize(text)
    }
}
