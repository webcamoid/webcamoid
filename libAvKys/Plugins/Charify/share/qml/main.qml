/* Webcamoid, camera capture application.
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

import QtQuick
import QtQuick.Controls
import Qt.labs.platform as LABS
import QtQuick.Layouts
import Ak
import AkControls as AK
import CharifyElement

ColumnLayout {
    id: root
    layoutDirection: rtl? Qt.RightToLeft: Qt.LeftToRight

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

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

    AK.LabeledComboBox {
        id: cbxMode
        label: qsTr("Mode")
        textRole: "text"
        currentIndex: optionIndex(cbxMode, Charify.mode)
        Layout.fillWidth: true
        Accessible.description: label
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
        font.bold: true
        Layout.fillWidth: true
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
        font.bold: true
        Layout.fillWidth: true
    }
    AK.ActionTextField {
        id: txtTable
        icon.source: "image://icons/fonts"
        labelText: Charify.font.family + " " + Charify.font.pointSize
        placeholderText: qsTr("Font")
        buttonText: qsTr("Search the font to be used")
        Layout.fillWidth: true

        onButtonClicked: fontDialog.open()
    }

    AK.LabeledComboBox {
        id: cbxHinting
        label: qsTr("Hinting")
        textRole: "text"
        currentIndex: optionIndex(cbxHinting, Charify.hintingPreference)
        Layout.fillWidth: true
        Accessible.description: label

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

    AK.LabeledComboBox {
        id: cbxStyle
        //: Different font rendering strategies
        label: qsTr("Style")
        textRole: "text"
        currentIndex: optionIndex(cbxStyle, Charify.styleStrategy)
        Layout.fillWidth: true
        Accessible.description: label

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

    GridLayout {
        columns: 2
        layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight

        Label {
            id: txtForegroundColor
            text: qsTr("Foreground color")
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(Charify.foregroundColor)
            title: qsTr("Choose the foreground color")
            showAlphaChannel: true
            Accessible.description: txtForegroundColor.text

            onCurrentColorChanged: Charify.foregroundColor = AkUtils.toRgba(currentColor)
        }
        Label {
            id: txtBackgroundColor
            text: qsTr("Background color")
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(Charify.backgroundColor)
            title: qsTr("Choose the background color")
            showAlphaChannel: true
            Accessible.description: txtBackgroundColor.text

            onCurrentColorChanged: Charify.backgroundColor = AkUtils.toRgba(currentColor)
        }
    }

    Switch {
        text: qsTr("Smooth scaling")
        checked: Charify.smooth
        Layout.fillWidth: true
        Accessible.name: text

        onCheckedChanged: Charify.smooth = checked
    }

    Switch {
        text: qsTr("Reversed")
        checked: Charify.reversed
        Accessible.name: text
        Layout.fillWidth: true

        onCheckedChanged: Charify.reversed = checked
    }

    LABS.FontDialog {
        id: fontDialog
        title: qsTr("Please choose a font")
        font: Charify.font

        onAccepted: Charify.font = font
    }
}
