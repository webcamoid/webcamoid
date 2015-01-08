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
    columns: 2

    function strToFloat(str)
    {
        return str.length < 1? 0: parseFloat(str)
    }

    function strToSize(str)
    {
        if (str.length < 1)
            return Qt.size()

        var size = str.split("x")

        if (size.length < 2)
            return Qt.size()

        return Qt.size(size[0], size[1])
    }

    Label {
        text: qsTr("Mode")
    }
    ComboBox {
        currentIndex: Matrix.mode
        model: [0, 1]

        onCurrentIndexChanged: Matrix.mode = currentIndex
    }

    Label {
        text: qsTr("N° of characters")
    }
    TextField {
        text: Matrix.nChars
        validator: RegExpValidator {
            regExp: /\d+/
        }

        onTextChanged: Matrix.nChars = strToFloat(text)
    }

    Label {
        text: qsTr("Font size")
    }
    TextField {
        text: Matrix.fontSize.width + "x" + Matrix.fontSize.height
        validator: RegExpValidator {
            regExp: /\d+x\d+/
        }

        onTextChanged: Matrix.fontSize = strToSize(text)
    }

    Label {
        text: qsTr("Font depth")
    }
    TextField {
        text: Matrix.fontDepth
        validator: RegExpValidator {
            regExp: /\d+/
        }

        onTextChanged: Matrix.fontDepth = strToFloat(text)
    }

    Label {
        text: qsTr("White level")
    }
    TextField {
        text: Matrix.white
        validator: RegExpValidator {
            regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
        }

        onTextChanged: Matrix.white = strToFloat(text)
    }

    CheckBox {
        text: qsTr("pause")
        checked: Matrix.pause

        onCheckedChanged: Matrix.pause = checked
    }
}
