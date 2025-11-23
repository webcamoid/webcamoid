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
        id: txtBrightness
        text: qsTr("Brightness")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldBrightness
        value: Photocopy.brightness
        stepSize: 1
        to: 256
        Layout.fillWidth: true
        Accessible.name: txtBrightness.text

        onValueChanged: Photocopy.brightness = value
    }

    Label {
        id: txtContrast
        text: qsTr("Contrast")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldContrast
        value: Photocopy.contrast
        stepSize: 1
        to: 256
        Layout.fillWidth: true
        Accessible.name: txtContrast.text

        onValueChanged: Photocopy.contrast = value
    }
}
