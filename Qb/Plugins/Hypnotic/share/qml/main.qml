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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

GridLayout {
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

    // Marker type.
    Label {
        text: qsTr("Mode")
    }
    ComboBox {
        id: cbxMode
        currentIndex: modeIndex(Hypnotic.mode)

        model: ListModel {
            ListElement {
                text: qsTr("Spiral 1")
                mode: "spiral1"
            }
            ListElement {
                text: qsTr("Spiral 2")
                mode: "spiral2"
            }
            ListElement {
                text: qsTr("Parabola")
                mode: "parabola"
            }
            ListElement {
                text: qsTr("Horizontal stripe")
                mode: "horizontalStripe"
            }
        }

        onCurrentIndexChanged: Hypnotic.mode = cbxMode.model.get(currentIndex).mode
    }

    Label {
        text: qsTr("Speed increment")
    }
    TextField {
        text: Hypnotic.speedInc
        validator: RegExpValidator {
            regExp: /-?\d+/
        }

        onTextChanged: Hypnotic.speedInc = strToFloat(text)
    }

    Label {
        text: qsTr("Threshold")
    }
    TextField {
        text: Hypnotic.threshold
        validator: RegExpValidator {
            regExp: /-?\d+/
        }

        onTextChanged: Hypnotic.threshold = strToFloat(text)
    }
}
