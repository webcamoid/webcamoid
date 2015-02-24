/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

GridLayout {
    id: configs
    columns: 2

    function modeIndex(mode)
    {
        var index = -1

        for (var i = 0; i < cbxMode.model.count; i++)
            if (cbxMode.model.get(i).mode === mode) {
                index = i
                break
            }

        return index
    }

    function strToFloat(str)
    {
        return str.length < 1? 0: parseFloat(str)
    }

    Label {
        text: qsTr("Grab mode")
    }
    ComboBox {
        id: cbxMode
        currentIndex: modeIndex(DelayGrab.mode)

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
        validator: RegExpValidator {
            regExp: /\d+/
        }

        onTextChanged: DelayGrab.blockSize = strToFloat(text)
    }

    Label {
        text: qsTr("N° of frames")
    }
    TextField {
        text: DelayGrab.nFrames
        validator: RegExpValidator {
            regExp: /\d+/
        }

        onTextChanged: DelayGrab.nFrames = strToFloat(text)
    }
}
