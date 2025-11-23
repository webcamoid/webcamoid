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
        id: lblSpeed
        text: qsTr("Speed")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldSpeed
        value: Dizzy.speed
        stepSize: 0.01
        from: 0.01
        to: 60
        Layout.fillWidth: true
        Accessible.name: lblSpeed.text

        onValueChanged: Dizzy.speed = value
    }

    Label {
        id: lblZoomRate
        text: qsTr("Zoom rate")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldZoomRate
        value: Dizzy.zoomRate
        stepSize: 0.001
        to: 0.25
        Layout.fillWidth: true
        Accessible.name: lblZoomRate.text

        onValueChanged: Dizzy.zoomRate = value
    }

    Label {
        id: lblStrength
        text: qsTr("Strength")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldStrength
        value: Dizzy.strength
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblStrength.text

        onValueChanged: Dizzy.strength = value
    }
}
