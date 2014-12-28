/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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

    function strToSize(str)
    {
        var size = str.split("x")

        return Qt.size(size[0], size[1])
    }

    Label {
        text: qsTr("Denoise method")
    }
    ComboBox {
        id: cbxMode
        currentIndex: modeIndex(Denoise.mode)

        model: ListModel {
            ListElement {
                text: qsTr("Gauss")
                mode: "gauss"
            }
            ListElement {
                text: qsTr("Select")
                mode: "select"
            }
        }

        onCurrentIndexChanged: Denoise.mode = cbxMode.model.get(currentIndex).mode
    }

    Label {
        text: qsTr("Scan block")
    }
    TextField {
        text: Denoise.scanSize.width + "x" + Denoise.scanSize.height
        validator: RegExpValidator {
            regExp: /\d+x\d+/
        }

        onTextChanged: Denoise.scanSize = strToSize(text)
    }

    Label {
        text: qsTr("Mu")
    }
    TextField {
        text: Denoise.mu
        validator: RegExpValidator {
            regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
        }

        onTextChanged: Denoise.mu = parseFloat(text)
    }

    Label {
        text: qsTr("Sigma")
    }
    TextField {
        text: Denoise.sigma
        validator: RegExpValidator {
            regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
        }

        onTextChanged: Denoise.sigma = parseFloat(text)
    }
}
