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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GridLayout {
    id: configs
    columns: 2

    function modeIndex(mode)
    {
        var index = -1

        for (var i = 0; i < cbxMode.model.count; i++)
            if (cbxMode.model.get(i).mode == mode) {
                index = i
                break
            }

        return index
    }

    Label {
        text: qsTr("Grab mode")
    }
    ComboBox {
        id: cbxMode
        textRole: "text"
        currentIndex: modeIndex(DelayGrab.mode)
        Layout.fillWidth: true

        model: ListModel {
            ListElement {
                text: qsTr("Random square")
                mode: "RandomSquare"
            }
            ListElement {
                text: qsTr("Vertical increase")
                mode: "VerticalIncrease"
            }
            ListElement {
                text: qsTr("Horizontal increase")
                mode: "HorizontalIncrease"
            }
            ListElement {
                text: qsTr("Rings increase")
                mode: "RingsIncrease"
            }
        }

        onCurrentIndexChanged: DelayGrab.mode = cbxMode.model.get(currentIndex).mode
    }

    Label {
        text: qsTr("Block size")
    }
    TextField {
        text: DelayGrab.blockSize
        placeholderText: qsTr("Block size")
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /\d+/
        }
        Layout.fillWidth: true

        onTextChanged: DelayGrab.blockSize = Number(text)
    }

    Label {
        text: qsTr("N° of frames")
    }
    TextField {
        text: DelayGrab.nFrames
        placeholderText: qsTr("N° of frames")
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /\d+/
        }
        Layout.fillWidth: true

        onTextChanged: DelayGrab.nFrames = Number(text)
    }
}
