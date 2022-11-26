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
import Ak 1.0
import AkControls 1.0 as AK

GridLayout {
    columns: 2

    function optionIndex(cbx, option)
    {
        var index = -1

        for (var i = 0; i < cbx.model.count; i++)
            if (cbx.model.get(i).option == option) {
                index = i
                break
            }

        return index
    }

    Label {
        id: txtNumberOfDrops
        text: qsTr("Number of drops")
    }
    TextField {
        text: Matrix.nDrops
        placeholderText: qsTr("Number of drops")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtNumberOfDrops.text

        onTextChanged: Matrix.nDrops = Number(text)
    }
    Label {
        id: txtSymbols
        text: qsTr("Symbols")
    }
    TextField {
        text: Matrix.charTable
        placeholderText: qsTr("Symbols")
        selectByMouse: true
        Layout.fillWidth: true
        Accessible.name: txtSymbols.text

        onTextChanged: Matrix.charTable = text
    }

    Label {
        id: txtFont
        text: qsTr("Font")
    }
    RowLayout {
        TextField {
            id: txtTable
            text: Matrix.font.family + " " + Matrix.font.pointSize
            placeholderText: qsTr("Font")
            selectByMouse: true
            readOnly: true
            font: Matrix.font
            Layout.fillWidth: true
            Accessible.name: txtFont.text
        }
        Button {
            text: qsTr("Select")
            icon.source: "image://icons/fonts"
            Accessible.description: qsTr("Select font")

            onClicked: fontDialog.open()
        }
    }

    Label {
        id: txtHinting
        text: qsTr("Hinting")
    }
    ComboBox {
        id: cbxHinting
        textRole: "text"
        currentIndex: optionIndex(cbxHinting, Matrix.hintingPreference)
        Layout.fillWidth: true
        Accessible.description: txtHinting.text

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
        id: txtStyle
        //: Different font rendering strategies
        text: qsTr("Style")
    }
    ComboBox {
        id: cbxStyle
        textRole: "text"
        currentIndex: optionIndex(cbxStyle, Matrix.styleStrategy)
        Layout.fillWidth: true
        Accessible.description: txtStyle.text

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
        id: txtCursorColor
        text: qsTr("Cursor color")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(Matrix.cursorColor)
            title: qsTr("Choose the cursor color")
            Accessible.description: txtCursorColor.text

            onCurrentColorChanged: Matrix.cursorColor = AkUtils.toRgba(currentColor)
        }
    }

    Label {
        id: txtForegroundColor
        text: qsTr("Foreground color")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(Matrix.foregroundColor)
            title: qsTr("Choose the foreground color")
            Accessible.description: txtForegroundColor.text

            onCurrentColorChanged: Matrix.foregroundColor = AkUtils.toRgba(currentColor)
        }
    }

    Label {
        id: txtBackgroundColor
        text: qsTr("Background color")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(Matrix.backgroundColor)
            title: qsTr("Choose the background color")
            Accessible.description: txtBackgroundColor.text

            onCurrentColorChanged: Matrix.backgroundColor = AkUtils.toRgba(currentColor)
        }
    }

    Label {
        id: txtMinimumDropLength
        text: qsTr("Minimum drop length")
    }
    TextField {
        text: Matrix.minDropLength
        placeholderText: qsTr("Min. drop length")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtMinimumDropLength.text

        onTextChanged: Matrix.minDropLength = Number(text)
    }
    Label {
        id: txtMaximumDropLength
        text: qsTr("Maximum drop length")
    }
    TextField {
        text: Matrix.maxDropLength
        placeholderText: qsTr("Max. drop length")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtMaximumDropLength.text

        onTextChanged: Matrix.maxDropLength = Number(text)
    }
    Label {
        id: txtMinimumSpeed
        text: qsTr("Minimum speed")
    }
    TextField {
        text: Matrix.minSpeed
        placeholderText: qsTr("Min. speed")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /\d+\.\d+|\d+\.|\.\d+|\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtMinimumSpeed.text

        onTextChanged: Matrix.minSpeed = Number(text)
    }
    Label {
        id: txtMaximumSpeed
        text: qsTr("Maximum speed")
    }
    TextField {
        text: Matrix.maxSpeed
        placeholderText: qsTr("Max. speed")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /\d+\.\d+|\d+\.|\.\d+|\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtMaximumSpeed.text

        onTextChanged: Matrix.maxSpeed = Number(text)
    }

    Label {
        id: txtSmooth
        text: qsTr("Smooth scaling")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        Switch {
            checked: Matrix.smooth
            Accessible.name: txtSmooth.text

            onCheckedChanged: Matrix.smooth = checked
        }
    }

    Label {
        id: txtShowCursor
        text: qsTr("Show cursor")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        Switch {
            checked: Matrix.showCursor
            Accessible.name: txtShowCursor.text

            onCheckedChanged: Matrix.showCursor = checked
        }
    }

    Label {
        id: txtShowRain
        text: qsTr("Show rain")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        Switch {
            checked: Matrix.showRain
            Accessible.name: txtShowRain.text

            onCheckedChanged: Matrix.showRain = checked
        }
    }

    LABS.FontDialog {
        id: fontDialog
        title: qsTr("Please choose a font")
        font: Matrix.font

        onAccepted: Matrix.font = font
    }
}
