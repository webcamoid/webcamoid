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
import QtQuick.Layouts 1.3
import "qrc:/Ak/share/qml/AkQmlControls"

GridLayout {
    columns: 3

    Connections {
        target: Edge

        onThLowChanged: {
            sldThLow.value = thLow
            spbThLow.rvalue = thLow
        }

        onThHiChanged: {
            sldThHi.value = thHi
            spbThHi.rvalue = thHi
        }
    }

    // Canny
    Label {
        text: qsTr("Canny mode")
    }
    CheckBox {
        id: chkCanny
        checked: Edge.canny

        onCheckedChanged: Edge.canny = checked
    }
    Label {
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
    AkSpinBox {
        id: spbThLow
        enabled: chkCanny.checked
        rvalue: Edge.thLow
        maximumValue: sldThLow.to
        step: sldThLow.stepSize

        onRvalueChanged: Edge.thLow = Math.min(rvalue, Edge.thHi)
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
    AkSpinBox {
        id: spbThHi
        enabled: chkCanny.checked
        rvalue: Edge.thHi
        maximumValue: sldThHi.to
        step: sldThHi.stepSize

        onRvalueChanged: Edge.thHi = Math.max(rvalue, Edge.thLow)
    }

    // Equalize
    Label {
        text: qsTr("Equalize")
    }
    CheckBox {
        checked: Edge.equalize

        onCheckedChanged: Edge.equalize = checked
    }
    Label {
    }

    // Invert
    Label {
        text: qsTr("Invert")
    }
    CheckBox {
        checked: Edge.invert

        onCheckedChanged: Edge.invert = checked
    }
    Label {
    }
}
