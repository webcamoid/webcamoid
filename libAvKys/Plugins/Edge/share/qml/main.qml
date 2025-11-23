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

ColumnLayout {
    // Canny
    Switch {
        id: chkCanny
        //: https://en.wikipedia.org/wiki/Canny_edge_detector
        text: qsTr("Canny mode")
        checked: Edge.canny
        Accessible.name: text
        Layout.fillWidth: true

        onCheckedChanged: Edge.canny = checked
    }

    // Threshold
    Label {
        id: txtCannyThreshold
        text: qsTr("Canny threshold")
        enabled: chkCanny.checked
        font.bold: true
        Layout.fillWidth: true
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

    // Equalize
    Switch {
        //: https://en.wikipedia.org/wiki/Histogram_equalization
        text: qsTr("Equalize")
        checked: Edge.equalize
        Accessible.name: text
        Layout.fillWidth: true

        onCheckedChanged: Edge.equalize = checked
    }

    // Invert
    Switch {
        text: qsTr("Invert")
        checked: Edge.invert
        Accessible.name: text
        Layout.fillWidth: true

        onCheckedChanged: Edge.invert = checked
    }
}
