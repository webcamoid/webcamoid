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
    // Configure the number of scratches to show.
    Label {
        id: lblNScratches
        text: qsTr("Number of scratches")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldNScratches
        value: Aging.nScratches
        to: 255
        stepSize: 1
        Accessible.name: lblNScratches.text
        Layout.fillWidth: true

        onValueChanged: Aging.nScratches = value
    }

    // Aging mode.
    Switch {
        id: chkAddDust
        text: qsTr("Add dust")
        checked: Aging.addDust
        Accessible.name: text
        Layout.fillWidth: true

        onCheckedChanged: Aging.addDust = checked
    }
}
