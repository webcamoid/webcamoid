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
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2
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

    function fromRgba(rgba)
    {
        var a = ((rgba >> 24) & 0xff) / 255.0
        var r = ((rgba >> 16) & 0xff) / 255.0
        var g = ((rgba >> 8) & 0xff) / 255.0
        var b = (rgba & 0xff) / 255.0

        return Qt.rgba(r, g, b, a)
    }

    function toRgba(color)
    {
        var a = Math.round(255 * color.a) << 24
        var r = Math.round(255 * color.r) << 16
        var g = Math.round(255 * color.g) << 8
        var b = Math.round(255 * color.b)

        return a | r | g | b
    }

    function invert(color) {
        return Qt.rgba(1.0 - color.r, 1.0 - color.g, 1.0 - color.b, 1)
    }

    Label {
        text: qsTr("Mode")
    }
    ComboBox {
        id: cbxMode
        currentIndex: modeIndex(Radioactive.mode)

        model: ListModel {
            ListElement {
                text: qsTr("Soft normal")
                mode: "softNormal"
            }
            ListElement {
                text: qsTr("Hard normal")
                mode: "hardNormal"
            }
            ListElement {
                text: qsTr("Soft color")
                mode: "softColor"
            }
            ListElement {
                text: qsTr("Hard color")
                mode: "hardColor"
            }
        }

        onCurrentIndexChanged: Radioactive.mode = cbxMode.model.get(currentIndex).mode
    }

    Label {
        text: qsTr("Blur")
    }
    TextField {
        text: Radioactive.blur
        validator: RegExpValidator {
            regExp: /\d+\.\d+|\d+\.|\.\d+|\d+/
        }

        onTextChanged: Radioactive.blur = strToFloat(text)
    }

    Label {
        text: qsTr("Zoom")
    }
    TextField {
        text: Radioactive.zoom
        validator: RegExpValidator {
            regExp: /\d+\.\d+|\d+\.|\.\d+|\d+/
        }

        onTextChanged: Radioactive.zoom = strToFloat(text)
    }

    Label {
        text: qsTr("Threshold")
    }
    TextField {
        text: Radioactive.threshold
        validator: RegExpValidator {
            regExp: /\d+/
        }

        onTextChanged: Radioactive.threshold = strToFloat(text)
    }

    Label {
        text: qsTr("Luma threshold")
    }
    TextField {
        text: Radioactive.lumaThreshold
        validator: RegExpValidator {
            regExp: /\d+/
        }

        onTextChanged: Radioactive.lumaThreshold = strToFloat(text)
    }

    Label {
        text: qsTr("Alpha differential")
    }
    TextField {
        text: Radioactive.alphaDiff
        validator: RegExpValidator {
            regExp: /-?\d+/
        }

        onTextChanged: Radioactive.alphaDiff = strToFloat(text)
    }

    Label {
        text: qsTr("Radiation color")
    }
    Button {
        Layout.preferredWidth: 32
        Layout.preferredHeight: 32

        style: ButtonStyle {
            background: Rectangle {
                color: fromRgba(Radioactive.radColor)
                border.color: invert(color)
                border.width: 1
            }
        }

        function setColor(color)
        {
             Radioactive.radColor = toRgba(color)
        }

        onClicked: {
            colorDialog.caller = this
            colorDialog.currentColor = fromRgba(Radioactive.radColor)
            colorDialog.open()
        }
    }

    ColorDialog {
        id: colorDialog
        title: qsTr("Choose a color")

        property Item caller: null

        onAccepted: caller.setColor(color)
    }
}
