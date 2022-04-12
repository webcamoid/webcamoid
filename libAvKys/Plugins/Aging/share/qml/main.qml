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
    columns: 3

    Connections {
        target: Aging

        function onNScratchesChanged(nScratches)
        {
            sldNScratches.value = nScratches
            spbNScratches.value = nScratches
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
        Accessible.name: lblNScratches.text

        onValueChanged: Aging.nScratches = value
    }
    SpinBox {
        id: spbNScratches
        value: Aging.nScratches
        to: sldNScratches.to
        stepSize: sldNScratches.stepSize
        editable: true
        Accessible.name: lblNScratches.text

        onValueChanged: Aging.nScratches = Number(value)
    }

    // Aging mode.
    Label {
        id: txtAddDust
        text: qsTr("Add dust")
    }
    RowLayout {
        Layout.columnSpan: 2
        Layout.fillWidth: true

        Item {
            Layout.fillWidth: true
        }
        Switch {
            id: chkAddDust
            checked: Aging.addDust
            Accessible.name: txtAddDust.text

            onCheckedChanged: Aging.addDust = checked
        }
    }
}
