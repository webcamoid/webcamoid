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
import CharifyElement 1.0

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
        id: txtMode
        text: qsTr("Mode")
    }
    ComboBox {
        id: cbxMode
        textRole: "text"
        currentIndex: optionIndex(cbxMode, Charify.mode)
        Layout.fillWidth: true
        Accessible.description: txtMode.text
        model: ListModel {
            ListElement {
                text: qsTr("Natural")
                option: CharifyElement.ColorModeNatural
            }
            ListElement {
                text: qsTr("Fixed")
                option: CharifyElement.ColorModeFixed
            }
        }

        onCurrentIndexChanged: Charify.mode = cbxMode.model.get(currentIndex).option
    }

    Label {
        id: txtSymbols
        text: qsTr("Symbols")
    }
    TextField {
        text: Charify.charTable
        placeholderText: qsTr("Symbols")
        selectByMouse: true
        Layout.fillWidth: true
        Accessible.name: txtSymbols.text

        onTextChanged: Charify.charTable = text
    }

    Label {
        id: txtFont
        text: qsTr("Font")
    }
    RowLayout {
        TextField {
            id: txtTable
            text: Charify.font.family + " " + Charify.font.pointSize
            placeholderText: qsTr("Font")
            selectByMouse: true
            readOnly: true
            font: Charify.font
            Layout.fillWidth: true
            Accessible.name: txtFont.text
        }
        Button {
            text: qsTr("Search")
            icon.source: "image://icons/fonts"
            Accessible.description: qsTr("Search the font to be used")

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
        currentIndex: optionIndex(cbxHinting, Charify.hintingPreference)
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

        onCurrentIndexChanged: Charify.hintingPreference = cbxHinting.model.get(currentIndex).option
    }

    Label {
        id: txtStyle
        //: Different font rendering strategies
        text: qsTr("Style")
    }
    ComboBox {
        id: cbxStyle
        textRole: "text"
        currentIndex: optionIndex(cbxStyle, Charify.styleStrategy)
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

        onCurrentIndexChanged: Charify.styleStrategy = cbxStyle.model.get(currentIndex).option
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
            currentColor: AkUtils.fromRgba(Charify.foregroundColor)
            title: qsTr("Choose the foreground color")
            showAlphaChannel: true
            Accessible.description: txtForegroundColor.text

            onCurrentColorChanged: Charify.foregroundColor = AkUtils.toRgba(currentColor)
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
            currentColor: AkUtils.fromRgba(Charify.backgroundColor)
            title: qsTr("Choose the background color")
            showAlphaChannel: true
            Accessible.description: txtBackgroundColor.text

            onCurrentColorChanged: Charify.backgroundColor = AkUtils.toRgba(currentColor)
        }
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
            checked: Charify.smooth
            Accessible.name: txtSmooth.text

            onCheckedChanged: Charify.smooth = checked
        }
    }

    Label {
        id: txtReversed
        text: qsTr("Reversed")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        Switch {
            checked: Charify.reversed
            Accessible.name: txtReversed.text

            onCheckedChanged: Charify.reversed = checked
        }
    }

    LABS.FontDialog {
        id: fontDialog
        title: qsTr("Please choose a font")
        font: Charify.font

        onAccepted: Charify.font = font
    }
}
