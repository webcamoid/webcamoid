/* Webcamoid, camera capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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
import Ak

GridLayout {
    columns: 2

    Component.onCompleted: {
        var fps = AkFrac.create(DesktopCapture.fps).value;
        var q = Infinity;
        var index = -1;

        for (var i in cbxFps.model) {
            var diff = Math.abs(cbxFps.model[i] - fps);

            if (diff < q) {
                index = i;
                q = diff;
            }
        }

        cbxFps.currentIndex = index;
        DesktopCapture.fps = AkFrac.createVariant(cbxFps.model[index], 1);
    }

    Label {
        id: lblFps
        text: qsTr("Frame rate")
    }
    ComboBox {
        id: cbxFps
        Accessible.description: lblFps.text
        currentIndex: 10
        Layout.fillWidth: true
        model: [300,
                240,
                144,
                120,
                100,
                90,
                72,
                60,
                50,
                48,
                30,
                25,
                24,
                20,
                15,
                10,
                5,
                2,
                1]

        onCurrentIndexChanged: {
            if (currentIndex > -1)
                DesktopCapture.fps = AkFrac.createVariant(model[currentIndex], 1);
        }
    }

    Label {
        id: txtShowCursor
        text: qsTr("Show cursor")
        visible: DesktopCapture.canCaptureCursor
    }
    RowLayout {
        visible: DesktopCapture.canCaptureCursor

        Item {
            Layout.fillWidth: true
        }
        Switch {
            checked: DesktopCapture.showCursor
            Accessible.name: txtShowCursor.text

            onCheckedChanged: DesktopCapture.showCursor = checked
        }
    }

    Label {
        id: txtCursorSize
        text: qsTr("Cursor size")
        visible: DesktopCapture.canChangeCursorSize
    }
    TextField {
        text: DesktopCapture.cursorSize
        visible: DesktopCapture.canChangeCursorSize
        placeholderText: txtCursorSize.text
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtCursorSize.text

        onTextChanged: DesktopCapture.cursorSize = Number(text)
    }
}
