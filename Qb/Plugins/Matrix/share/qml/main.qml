/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1

GridLayout {
    columns: 2

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
        text: qsTr("N° of drops")
    }
    TextField {
        text: Matrix.nDrops
        validator: RegExpValidator {
            regExp: /\d+/
        }

        onTextChanged: Matrix.nDrops = strToFloat(text)
    }

    Label {
        text: qsTr("Symbols")
    }
    TextField {
        text: Matrix.charTable
        onTextChanged: Matrix.charTable = text
    }

    Label {
        text: qsTr("Font")
    }
    RowLayout {
        TextField {
            id: txtTable
            text: Matrix.font.family + " " + Matrix.font.pointSize
            readOnly: true
            font: Matrix.font
            Layout.fillWidth: true
        }
        Button {
            text: qsTr("Select")
            iconName: "preferences-desktop-font"

            onClicked: fontDialog.open()
        }
    }

    Label {
        text: qsTr("Cursor color")
    }
    Button {
        Layout.preferredWidth: 32
        Layout.preferredHeight: 32

        style: ButtonStyle {
            background: Rectangle {
                color: fromRgba(Matrix.cursorColor)
                border.color: invert(color)
                border.width: 1
            }
        }

        function setColor(color)
        {
             Matrix.cursorColor = toRgba(color)
        }

        onClicked: {
            colorDialog.caller = this
            colorDialog.currentColor = fromRgba(Matrix.cursorColor)
            colorDialog.open()
        }
    }

    Label {
        text: qsTr("Foreground color")
    }
    Button {
        Layout.preferredWidth: 32
        Layout.preferredHeight: 32

        style: ButtonStyle {
            background: Rectangle {
                color: fromRgba(Matrix.foregroundColor)
                border.color: invert(color)
                border.width: 1
            }
        }

        function setColor(color)
        {
             Matrix.foregroundColor = toRgba(color)
        }

        onClicked: {
            colorDialog.caller = this
            colorDialog.currentColor = fromRgba(Matrix.foregroundColor)
            colorDialog.open()
        }
    }

    Label {
        text: qsTr("Background color")
    }
    Button {
        Layout.preferredWidth: 32
        Layout.preferredHeight: 32

        style: ButtonStyle {
            background: Rectangle {
                color: fromRgba(Matrix.backgroundColor)
                border.color: invert(color)
                border.width: 1
            }
        }

        function setColor(color)
        {
             Matrix.backgroundColor = toRgba(color)
        }

        onClicked: {
            colorDialog.caller = this
            colorDialog.currentColor = fromRgba(Matrix.backgroundColor)
            colorDialog.open()
        }
    }

    Label {
        text: qsTr("Min. drop length")
    }
    TextField {
        text: Matrix.minDropLength
        validator: RegExpValidator {
            regExp: /\d+/
        }

        onTextChanged: Matrix.minDropLength = strToFloat(text)
    }

    Label {
        text: qsTr("Max. drop length")
    }
    TextField {
        text: Matrix.maxDropLength
        validator: RegExpValidator {
            regExp: /\d+/
        }

        onTextChanged: Matrix.maxDropLength = strToFloat(text)
    }

    Label {
        text: qsTr("Min. speed")
    }
    TextField {
        text: Matrix.minSpeed
        validator: RegExpValidator {
            regExp: /\d+\.\d+|\d+\.|\.\d+|\d+/
        }

        onTextChanged: Matrix.minSpeed = strToFloat(text)
    }

    Label {
        text: qsTr("Max. speed")
    }
    TextField {
        text: Matrix.maxSpeed
        validator: RegExpValidator {
            regExp: /\d+\.\d+|\d+\.|\.\d+|\d+/
        }

        onTextChanged: Matrix.maxSpeed = strToFloat(text)
    }

    CheckBox {
        text: qsTr("Show cursor")
        checked: Matrix.showCursor

        onCheckedChanged: Matrix.showCursor = checked
    }

    FontDialog {
        id: fontDialog
        title: qsTr("Please choose a font")
        font: Matrix.font

        onAccepted: Matrix.font = font
    }

    ColorDialog {
        id: colorDialog
        title: qsTr("Choose a color")

        property Item caller: null

        onAccepted: caller.setColor(color)
    }
}
