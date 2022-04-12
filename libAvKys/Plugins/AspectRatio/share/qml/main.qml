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
    columns: 2

    Label {
        id: txtWidth
        text: qsTr("Width")
    }
    TextField {
        text: AspectRatio.width
        placeholderText: qsTr("Aspect ratio width")
        Accessible.name: txtWidth.text
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /[1-9][0-9]*/
        }
        Layout.fillWidth: true

        onTextChanged: AspectRatio.width = Number(text)
    }
    Label {
        id: txtHeight
        text: qsTr("Height")
    }
    TextField {
        text: AspectRatio.height
        placeholderText: qsTr("Aspect ratio height")
        Accessible.name: txtHeight.text
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /[1-9][0-9]*/
        }
        Layout.fillWidth: true

        onTextChanged: AspectRatio.height = Number(text)
    }
}
