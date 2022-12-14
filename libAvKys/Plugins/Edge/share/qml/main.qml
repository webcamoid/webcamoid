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

GridLayout {
    columns: 2

    Connections {
        target: Edge

        function onThLowChanged(thLow)
        {
            sldThreshold.first.value = thLow
            spbThresholdLow.value = thLow
        }

        function onThHiChanged(thHi)
        {
            sldThreshold.second.value = thHi
            spbThresholdHi.value = thHi
        }
    }

    // Canny
    Label {
        id: txtCannyMode
        //: https://en.wikipedia.org/wiki/Canny_edge_detector
        text: qsTr("Canny mode")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        Switch {
            id: chkCanny
            checked: Edge.canny
            Accessible.name: txtCannyMode.text

            onCheckedChanged: Edge.canny = checked
        }
    }

    // Threshold
    Label {
        id: txtCannyThreshold
        text: qsTr("Canny threshold")
        enabled: chkCanny.checked
    }
    RowLayout {
        SpinBox {
            id: spbThresholdLow
            value: Edge.thLow
            to: sldThreshold.to
            stepSize: sldThreshold.stepSize
            enabled: chkCanny.checked
            editable: true
            Accessible.name: qsTr("Canny threshold low")

            onValueChanged: Edge.thLow = Number(value)
        }
        RangeSlider {
            id: sldThreshold
            first.value: Edge.thLow
            second.value: Edge.thHi
            stepSize: 1
            to: 1530
            enabled: chkCanny.checked
            Layout.fillWidth: true
            Accessible.name: txtCannyThreshold.text

            first.onValueChanged: Edge.thLow = first.value
            second.onValueChanged: Edge.thHi = second.value
        }
        SpinBox {
            id: spbThresholdHi
            value: Edge.thHi
            to: sldThreshold.to
            stepSize: sldThreshold.stepSize
            enabled: chkCanny.checked
            editable: true
            Accessible.name: qsTr("Canny threshold hi")

            onValueChanged: Edge.thHi = Number(value)
        }
    }

    // Equalize
    Label {
        id: txtEqualize
        //: https://en.wikipedia.org/wiki/Histogram_equalization
        text: qsTr("Equalize")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        Switch {
            checked: Edge.equalize
            Accessible.name: txtEqualize.text

            onCheckedChanged: Edge.equalize = checked
        }
    }

    // Invert
    Label {
        id: txtInvert
        text: qsTr("Invert")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        Switch {
            checked: Edge.invert
            Accessible.name: txtInvert.text

            onCheckedChanged: Edge.invert = checked
        }
    }
}
