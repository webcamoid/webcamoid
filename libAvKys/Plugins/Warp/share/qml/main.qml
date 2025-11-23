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
    Label {
        id: txtRipples
        text: qsTr("Ripples")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldRipples
        value: Warp.ripples
        stepSize: 0.1
        to: 256
        Layout.fillWidth: true
        Accessible.name: txtRipples.text

        onValueChanged: Warp.ripples = value
    }
    Label {
        id: txtDuration
        text: qsTr("Duration (in seconds)")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldDuration
        value: Warp.duration
        stepSize: 1
        to: 128
        Layout.fillWidth: true
        Accessible.name: txtDuration.text

        onValueChanged: Warp.duration = value
    }
}
