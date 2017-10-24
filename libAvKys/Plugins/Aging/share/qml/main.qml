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
        target: Aging

        onNScratchesChanged: {
            sldNScratches.value = nScratches
            spbNScratches.rvalue = nScratches
        }
    }

    // Configure the number of scratches to show.
    Label {
        id: lblNScratches
        text: qsTr("Number of scratches")
    }
    Slider {
        id: sldNScratches
        value: Aging.nScratches
        to: 255
        stepSize: 1
        Layout.fillWidth: true

        onValueChanged: Aging.nScratches = value
    }
    AkSpinBox {
        id: spbNScratches
        rvalue: Aging.nScratches
        maximumValue: sldNScratches.to
        step: sldNScratches.stepSize

        onRvalueChanged: Aging.nScratches = rvalue
    }

    // Aging mode.
    Label {
        text: qsTr("Add dust")
    }
    CheckBox {
        id: chkAddDust
        checked: Aging.addDust

        onCheckedChanged: Aging.addDust = checked
    }
}
