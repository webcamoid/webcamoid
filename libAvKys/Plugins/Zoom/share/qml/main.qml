/* Webcamoid, camera capture application.
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    Label {
        id: lblZoom
        text: qsTr("Zoom")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldZoom
        value: Zoom.zoom
        stepSize: 0.1
        from: 1.0
        to: 10.0
        Layout.fillWidth: true
        Accessible.name: lblZoom.text

        onValueChanged: Zoom.zoom = value
    }
}
