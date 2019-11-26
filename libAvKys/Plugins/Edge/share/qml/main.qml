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

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

GridLayout {
    columns: 3

    Connections {
        target: Edge

        onThLowChanged: {
            sldThLow.value = thLow
            spbThLow.value = thLow
        }

        onThHiChanged: {
            sldThHi.value = thHi
            spbThHi.value = thHi
        }
    }

    // Canny
    Label {
        //: https://en.wikipedia.org/wiki/Canny_edge_detector
        text: qsTr("Canny mode")
    }
    RowLayout {
        Layout.columnSpan: 2

        Item {
            Layout.fillWidth: true
        }
        Switch {
            id: chkCanny
            checked: Edge.canny

            onCheckedChanged: Edge.canny = checked
        }
    }

    // thLow
    Label {
        text: qsTr("Lower Canny threshold")
        enabled: chkCanny.checked
    }
    Slider {
        id: sldThLow
        enabled: chkCanny.checked
        value: Edge.thLow
        stepSize: 1
        to: 1530
        Layout.fillWidth: true

        onValueChanged: Edge.thLow = Math.min(value, Edge.thHi)
    }
    SpinBox {
        id: spbThLow
        enabled: chkCanny.checked
        value: Edge.thLow
        to: sldThLow.to
        stepSize: sldThLow.stepSize
        editable: true

        onValueChanged: Edge.thLow = Math.min(value, Edge.thHi)
    }

    // thHi
    Label {
        text: qsTr("Higger Canny threshold")
        enabled: chkCanny.checked
    }
    Slider {
        id: sldThHi
        enabled: chkCanny.checked
        value: Edge.thHi
        stepSize: 1
        to: 1530
        Layout.fillWidth: true

        onValueChanged: Edge.thHi = Math.max(value, Edge.thLow)
    }
    SpinBox {
        id: spbThHi
        enabled: chkCanny.checked
        value: Edge.thHi
        to: sldThHi.to
        stepSize: sldThHi.stepSize
        editable: true

        onValueChanged: Edge.thHi = Math.max(value, Edge.thLow)
    }

    // Equalize
    Label {
        //: https://en.wikipedia.org/wiki/Histogram_equalization
        text: qsTr("Equalize")
    }
    RowLayout {
        Layout.columnSpan: 2

        Item {
            Layout.fillWidth: true
        }
        Switch {
            checked: Edge.equalize

            onCheckedChanged: Edge.equalize = checked
        }
    }

    // Invert
    Label {
        text: qsTr("Invert")
    }
    RowLayout {
        Layout.columnSpan: 2

        Item {
            Layout.fillWidth: true
        }
        Switch {
            checked: Edge.invert

            onCheckedChanged: Edge.invert = checked
        }
    }
}
