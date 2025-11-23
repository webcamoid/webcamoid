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
        id: txtNumberOfFrames
        text: qsTr("Number of frames")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        text: Nervous.nFrames
        placeholderText: qsTr("Number of frames")
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtNumberOfFrames.text

        onTextChanged: Nervous.nFrames = Number(text)
    }

    Switch {
        text: qsTr("Simple")
        checked: Nervous.simple
        Accessible.name: text
        Layout.fillWidth: true

        onCheckedChanged: Nervous.simple = checked
    }
}
