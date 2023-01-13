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
import Ak 1.0
import AkControls 1.0 as AK

GridLayout {
    columns: 2

    function invert(color) {
        return Qt.rgba(1.0 - color.r, 1.0 - color.g, 1.0 - color.b, 1)
    }

    Label {
        id: txtShowLines
        text: qsTr("Show lines")
    }
    TextField {
        text: ScanLines.showSize
        placeholderText: qsTr("Show lines")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtShowLines.text

        onTextChanged: ScanLines.showSize = Number(text)
    }
    Label {
        id: txtHideLines
        text: qsTr("Hide lines")
    }
    TextField {
        text: ScanLines.hideSize
        placeholderText: qsTr("Hide lines")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtHideLines.text

        onTextChanged: ScanLines.hideSize = Number(text)
    }
    Label {
        id: txtHideColor
        text: qsTr("Hide color")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(ScanLines.hideColor)
            title: qsTr("Choose the hide color")
            showAlphaChannel: true
            Accessible.description: txtHideColor.text

            onCurrentColorChanged: ScanLines.hideColor = AkUtils.toRgba(currentColor)
        }
    }
}
