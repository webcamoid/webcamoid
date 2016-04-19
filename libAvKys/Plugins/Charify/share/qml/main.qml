/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1

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

    function invert(color) {
        return Qt.rgba(1.0 - color.r, 1.0 - color.g, 1.0 - color.b, 1)
    }

    Label {
        text: qsTr("Mode")
    }
    ComboBox {
        id: cbxMode
        currentIndex: optionIndex(cbxMode, Charify.mode)

        model: ListModel {
            ListElement {
                text: qsTr("Natural")
                option: "natural"
            }
            ListElement {
                text: qsTr("Fixed")
                option: "fixed"
            }
        }

        onCurrentIndexChanged: Charify.mode = cbxMode.model.get(currentIndex).option
    }

    Label {
        text: qsTr("Symbols")
    }
    TextField {
        text: Charify.charTable
        onTextChanged: Charify.charTable = text
    }

    Label {
        text: qsTr("Font")
    }
    RowLayout {
        TextField {
            id: txtTable
            text: Charify.font.family + " " + Charify.font.pointSize
            readOnly: true
            font: Charify.font
            Layout.fillWidth: true
        }
        Button {
            text: qsTr("Search")
            iconName: "edit-find"

            onClicked: fontDialog.open()
        }
    }

    Label {
        text: qsTr("Hinting")
    }
    ComboBox {
        id: cbxHinting
        currentIndex: optionIndex(cbxHinting, Charify.hintingPreference)

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

        onCurrentIndexChanged: Charify.hintingPreference = cbxHinting.model.get(currentIndex).option
    }

    Label {
        text: qsTr("Style")
    }
    ComboBox {
        id: cbxStyle
        currentIndex: optionIndex(cbxStyle, Charify.styleStrategy)

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

        onCurrentIndexChanged: Charify.styleStrategy = cbxStyle.model.get(currentIndex).option
    }

    Label {
        text: qsTr("Foreground color")
    }
    Button {
        Layout.preferredWidth: 32
        Layout.preferredHeight: 32

        style: ButtonStyle {
            background: Rectangle {
                color: fromRgba(Charify.foregroundColor)
                border.color: invert(color)
                border.width: 1
            }
        }

        onClicked: foregroundColorDialog.open()
    }

    Label {
        text: qsTr("Background color")
    }
    Button {
        Layout.preferredWidth: 32
        Layout.preferredHeight: 32

        style: ButtonStyle {
            background: Rectangle {
                color: fromRgba(Charify.backgroundColor)
                border.color: invert(color)
                border.width: 1
            }
        }

        onClicked: backgroundColorDialog.open()
    }

    CheckBox {
        text: qsTr("Reversed")
        checked: Charify.reversed

        onCheckedChanged: Charify.reversed = checked
    }

    FontDialog {
        id: fontDialog
        title: qsTr("Please choose a font")
        font: Charify.font

        onAccepted: Charify.font = font
    }

    ColorDialog {
        id: foregroundColorDialog
        title: qsTr("Choose the foreground color")
        currentColor: fromRgba(Charify.foregroundColor)
        showAlphaChannel: true

        onAccepted: Charify.foregroundColor = toRgba(color)
    }

    ColorDialog {
        id: backgroundColorDialog
        title: qsTr("Choose the background color")
        currentColor: fromRgba(Charify.backgroundColor)
        showAlphaChannel: true

        onAccepted: Charify.backgroundColor = toRgba(color)
    }
}
