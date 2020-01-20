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
import Qt.labs.platform 1.1 as LABS
import QtQuick.Layouts 1.3
import AkControls 1.0 as AK

GridLayout {
    columns: 2

    function optionIndex(cbx, option)
    {
        var index = -1

        for (var i = 0; i < cbx.model.count; i++)
            if (cbx.model.get(i).option === option) {
                index = i
                break
            }

        return index
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

    Label {
        text: qsTr("N° of drops")
    }
    TextField {
        text: Matrix.nDrops
        placeholderText: qsTr("N° of drops")
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.fillWidth: true

        onTextChanged: Matrix.nDrops = Number(text)
    }
    Label {
        text: qsTr("Symbols")
    }
    TextField {
        text: Matrix.charTable
        placeholderText: qsTr("Symbols")
        Layout.fillWidth: true

        onTextChanged: Matrix.charTable = text
    }

    Label {
        text: qsTr("Font")
    }
    RowLayout {
        TextField {
            id: txtTable
            text: Matrix.font.family + " " + Matrix.font.pointSize
            placeholderText: qsTr("Font")
            readOnly: true
            font: Matrix.font
            Layout.fillWidth: true
        }
        Button {
            text: qsTr("Select")
            icon.source: "image://icons/fonts"

            onClicked: fontDialog.open()
        }
    }

    Label {
        text: qsTr("Hinting")
    }
    ComboBox {
        id: cbxHinting
        textRole: "text"
        currentIndex: optionIndex(cbxHinting, Matrix.hintingPreference)
        Layout.fillWidth: true

        model: ListModel {
            ListElement {
                text: qsTr("Default")
                option: "PreferDefaultHinting"
            }
            ListElement {
                text: qsTr("No hinting")
                option: "PreferNoHinting"
            }
            ListElement {
                text: qsTr("Vertical hinting")
                option: "PreferVerticalHinting"
            }
            ListElement {
                text: qsTr("Full hinting")
                option: "PreferFullHinting"
            }
        }

        onCurrentIndexChanged: Matrix.hintingPreference = cbxHinting.model.get(currentIndex).option
    }

    Label {
        //: Different font rendering strategies
        text: qsTr("Style")
    }
    ComboBox {
        id: cbxStyle
        textRole: "text"
        currentIndex: optionIndex(cbxStyle, Matrix.styleStrategy)
        Layout.fillWidth: true

        model: ListModel {
            ListElement {
                text: qsTr("Default")
                option: "PreferDefault"
            }
            ListElement {
                text: qsTr("Bitmap")
                option: "PreferBitmap"
            }
            ListElement {
                text: qsTr("Device")
                option: "PreferDevice"
            }
            ListElement {
                text: qsTr("Outline")
                option: "PreferOutline"
            }
            ListElement {
                text: qsTr("Force outline")
                option: "ForceOutline"
            }
            ListElement {
                text: qsTr("Match")
                option: "PreferMatch"
            }
            ListElement {
                text: qsTr("Quality")
                option: "PreferQuality"
            }
            ListElement {
                text: qsTr("Antialias")
                option: "PreferAntialias"
            }
            ListElement {
                text: qsTr("No antialias")
                option: "NoAntialias"
            }
            ListElement {
                text: qsTr("Compatible with OpenGL")
                option: "OpenGLCompatible"
            }
            ListElement {
                text: qsTr("Force integer metrics")
                option: "ForceIntegerMetrics"
            }
            ListElement {
                text: qsTr("No subpixel antialias")
                option: "NoSubpixelAntialias"
            }
            ListElement {
                text: qsTr("No font merging")
                option: "NoFontMerging"
            }
        }

        onCurrentIndexChanged: Matrix.styleStrategy = cbxStyle.model.get(currentIndex).option
    }

    Label {
        text: qsTr("Cursor color")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        AK.ColorButton {
            currentColor: fromRgba(Matrix.cursorColor)
            title: qsTr("Choose the cursor color")

            onCurrentColorChanged: Matrix.cursorColor = toRgba(currentColor)
        }
    }

    Label {
        text: qsTr("Foreground color")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        AK.ColorButton {
            currentColor: fromRgba(Matrix.foregroundColor)
            title: qsTr("Choose the foreground color")

            onCurrentColorChanged: Matrix.foregroundColor = toRgba(currentColor)
        }
    }

    Label {
        text: qsTr("Background color")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        AK.ColorButton {
            currentColor: fromRgba(Matrix.backgroundColor)
            title: qsTr("Choose the background color")

            onCurrentColorChanged: Matrix.backgroundColor = toRgba(currentColor)
        }
    }

    Label {
        text: qsTr("Min. drop length")
    }
    TextField {
        text: Matrix.minDropLength
        placeholderText: qsTr("Min. drop length")
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.fillWidth: true

        onTextChanged: Matrix.minDropLength = Number(text)
    }
    Label {
        text: qsTr("Max. drop length")
    }
    TextField {
        text: Matrix.maxDropLength
        placeholderText: qsTr("Max. drop length")
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.fillWidth: true

        onTextChanged: Matrix.maxDropLength = Number(text)
    }
    Label {
        text: qsTr("Min. speed")
    }
    TextField {
        text: Matrix.minSpeed
        placeholderText: qsTr("Min. speed")
        validator: RegExpValidator {
            regExp: /\d+\.\d+|\d+\.|\.\d+|\d+/
        }
        Layout.fillWidth: true

        onTextChanged: Matrix.minSpeed = Number(text)
    }
    Label {
        text: qsTr("Max. speed")
    }
    TextField {
        text: Matrix.maxSpeed
        placeholderText: qsTr("Max. speed")
        validator: RegExpValidator {
            regExp: /\d+\.\d+|\d+\.|\.\d+|\d+/
        }
        Layout.fillWidth: true

        onTextChanged: Matrix.maxSpeed = Number(text)
    }

    Label {
        text: qsTr("Show cursor")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        Switch {
            checked: Matrix.showCursor

            onCheckedChanged: Matrix.showCursor = checked
        }
    }

    LABS.FontDialog {
        id: fontDialog
        title: qsTr("Please choose a font")
        font: Matrix.font

        onAccepted: Matrix.font = font
    }
}
