/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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
        target: Rotate

        function onAngleChanged(angle)
        {
            sldAngle.value = angle
            spbAngle.value = angle
        }
    }

    // Configure rotation angle.
    Label {
        id: lblAngle
        text: qsTr("Angle")
    }
    Slider {
        id: sldAngle
        value: Rotate.angle
        stepSize: 1
        to: 360
        Layout.fillWidth: true
        Accessible.name: lblAngle.text

        onValueChanged: Rotate.angle = value
    }
    SpinBox {
        id: spbAngle
        value: Rotate.angle
        to: sldAngle.to
        stepSize: sldAngle.stepSize
        editable: true
        Accessible.name: lblAngle.text

        onValueChanged: Rotate.angle = Number(value)
    }

    Label {
        id: txtKeep
        text: qsTr("Keep resolution")
    }
    RowLayout {
        Layout.columnSpan: 2
        Layout.fillWidth: true

        Label {
            Layout.fillWidth: true
        }
        Switch {
            checked: Rotate.keep
            Accessible.name: txtKeep.text

            onCheckedChanged: Rotate.keep = checked
        }
    }
}
