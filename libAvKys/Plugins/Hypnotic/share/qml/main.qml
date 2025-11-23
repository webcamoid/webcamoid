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
import HypnoticElement

ColumnLayout {
    function modeIndex(mode)
    {
        var index = -1

        for (var i = 0; i < cbxMode.model.count; i++)
            if (cbxMode.model.get(i).mode == mode) {
                index = i
                break
            }

        return index
    }

    // Marker type.
    Label {
        id: txtMode
        text: qsTr("Mode")
        font.bold: true
        Layout.fillWidth: true
    }
    ComboBox {
        id: cbxMode
        textRole: "text"
        currentIndex: modeIndex(Hypnotic.mode)
        Layout.fillWidth: true
        Accessible.description: txtMode.text

        model: ListModel {
            ListElement {
                text: qsTr("Spiral 1")
                mode: HypnoticElement.OpticModeSpiral1
            }
            ListElement {
                text: qsTr("Spiral 2")
                mode: HypnoticElement.OpticModeSpiral2
            }
            ListElement {
                text: qsTr("Parabola")
                mode: HypnoticElement.OpticModeParabola
            }
            ListElement {
                text: qsTr("Horizontal stripe")
                mode: HypnoticElement.OpticModeHorizontalStripe
            }
        }

        onCurrentIndexChanged: Hypnotic.mode = cbxMode.model.get(currentIndex).mode
    }
    Label {
        id: txtSpeedIncrement
        text: qsTr("Speed increment")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        text: Hypnotic.speedInc
        placeholderText: qsTr("Speed increment")
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /-?\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtSpeedIncrement.text

        onTextChanged: Hypnotic.speedInc = Number(text)
    }

    Label {
        id: txtThreshold
        text: qsTr("Threshold")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldThreshold
        value: Hypnotic.threshold
        stepSize: 1
        to: 255
        Layout.fillWidth: true
        Accessible.name: txtThreshold.text

        onValueChanged: Hypnotic.threshold = value
    }
}
