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

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

GridLayout {
    columns: 3

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
        maximumValue: 1530

        onValueChanged: {
            value = Math.min(value, sldThHi.value)
            Edge.thLow = value
        }
    }
    SpinBox {
        id: spbThLow
        enabled: chkCanny.checked
        value: sldThLow.value
        maximumValue: sldThLow.maximumValue
        stepSize: sldThLow.stepSize

        onValueChanged: sldThLow.value = value
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
        maximumValue: 1530

        onValueChanged: {
            value = Math.max(value, sldThLow.value)
            Edge.thHi = value
        }
    }
    SpinBox {
        id: spbThHi
        enabled: chkCanny.checked
        value: sldThHi.value
        maximumValue: sldThHi.maximumValue
        stepSize: sldThHi.stepSize

        onValueChanged: sldThHi.value = value
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
